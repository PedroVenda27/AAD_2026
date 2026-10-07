# AAD_P4 — Resumo do que foi feito

Neste guião o objetivo é dividir um cálculo por **vários programas que comunicam pela rede**, usando *sockets*. Um programa é o **servidor** (reparte o trabalho) e os outros são **clientes** (fazem o trabalho e devolvem o resultado). Como comunicam pela rede, os clientes podem estar no mesmo computador ou noutros computadores.

---

## 1. Estudar o código de exemplo (`server_clients.c`)

O professor deu um único ficheiro com o código do servidor e do cliente. A escolha entre um e outro é feita na compilação:
- `-DSERVER=1` → compila o servidor
- `-DSERVER=0` → compila o cliente

### Como funciona a conversa entre servidor e cliente

Todas as mensagens têm o mesmo formato: um número com o **tamanho** e depois uma lista de até 7 números inteiros.

Cada vez que o cliente se liga ao servidor, acontece **apenas uma troca de mensagens** e a ligação é fechada logo a seguir:

1. O cliente envia uma de duas coisas:
   - mensagem vazia (tamanho 0) → "dá-me trabalho";
   - mensagem com 3 números `[índice, work_id, resultado]` → "aqui está o resultado do trabalho que me deste".
2. O servidor responde:
   - `[índice, work_id]` → "faz este trabalho";
   - mensagem vazia → "já não há trabalho, podes sair".
3. A ligação fecha. O cliente faz o trabalho **sem estar ligado** e só volta a ligar-se quando acabar.

O `work_id` serve para o servidor confirmar que o resultado que recebe corresponde mesmo ao trabalho que tinha dado.

### Ordem dos bytes (endianness)

Computadores diferentes podem guardar os números na memória com os bytes em ordem diferente. Para não haver confusão, antes de enviar cada número converte-se para a "ordem de rede" com `htonl()`, e quem recebe converte de volta com `ntohl()`. Assim funciona entre quaisquer dois computadores.

### Outros pormenores
- O servidor só atende **um cliente de cada vez**, mas como cada ligação é muito rápida, os clientes conseguem trabalhar todos em paralelo.
- O servidor tem um **alarme**: termina sozinho ao fim de 15 minutos, para não ficar esquecido a correr.
- As funções de leitura e escrita desistem se ficarem **10 segundos** sem conseguir enviar/receber.

### Erro encontrado no código do professor

Quando já não há trabalho, a variável `i` fica igual ao número de itens, e mesmo assim o código faz:

```c
work_items[i].status = assigned;
```

Isto escreve **fora do array** (uma posição a seguir ao fim). Foi corrigido na versão separada (`x_server.c`) para só fazer isto quando `i` é um índice válido.

---

## 2. Compilar e testar em local

```bash
make server client
./server 50000                  # num terminal
./client 127.0.0.1 50000        # noutro(s) terminal(is)
```

- `50000` é a porta (tem de estar entre 49152 e 65535).
- `127.0.0.1` é o endereço do próprio computador.

**Resultado:** com 3 clientes, os 10 trabalhos foram repartidos entre eles (3, 4 e 3) e o servidor mostrou no fim a tabela com todos os resultados.

---

## 3. Separar o código em vários ficheiros

O ficheiro único foi dividido em três partes:

| Ficheiros | O que contêm |
|---|---|
| `x_common.h` / `x_common.c` | O que é usado pelos dois lados: formato das mensagens, enviar/receber dados, enviar/receber mensagens, fechar o socket |
| `x_server.h` / `x_server.c` | Preparar o servidor, aceitar ligações, o alarme e o `main` do servidor |
| `x_client.h` / `x_client.c` | Ligar ao servidor e o `main` do cliente |

O que foi preciso mudar:
- **Tirar o `static`** das funções partilhadas. Uma função `static` só pode ser usada dentro do seu próprio ficheiro, e agora estas funções são chamadas a partir de outros ficheiros.
- **Remover os `#if SERVER`**, porque agora é o próprio ficheiro que decide se é servidor ou cliente.
- Pôr **proteções nos `.h`** (`#ifndef ... #define ... #endif`) para não serem incluídos duas vezes.

```bash
make x_server x_client
./x_server 50000
./x_client 127.0.0.1 50000
```

**Resultado:** compila sem avisos e dá **exatamente a mesma tabela** que a versão original.

---

## 4. Mandelbrot distribuído

### A ideia

O cálculo do Mandelbrot percorre 8193 linhas. Essas linhas foram divididas em **blocos** (por defeito 64 linhas por bloco, o que dá 129 blocos). Cada bloco é um "trabalho":

- o servidor dá um bloco a cada cliente que pede;
- o cliente conta os pontos do Mandelbrot nessas linhas e devolve o número;
- o servidor soma tudo no fim.

Como cada cliente pede um bloco novo assim que acaba o anterior, os clientes mais rápidos acabam por fazer mais blocos. É a mesma ideia do `schedule(dynamic)` do P3, mas agora entre programas diferentes em vez de threads.

### As mensagens

| Quem envia | Conteúdo |
|---|---|
| Servidor → cliente | `[índice, work_id, linha_inicial, linha_final]` |
| Cliente → servidor | `[índice, work_id, contagem]` |

### Melhorias em relação ao exemplo
- **Cliente que morre ou é lento:** quando já não há blocos novos, o servidor volta a dar blocos que ainda não tiveram resposta. Assim, se um cliente morrer, outro faz o trabalho dele e o cálculo termina na mesma. No exemplo original, o servidor ficava à espera para sempre.
- **Fim limpo dos clientes:** quando o servidor termina, os clientes que tentam ligar-se percebem que acabou e saem sem erro.
- **Cliente com threads:** o cliente pode usar várias threads OpenMP (como no P3) para calcular o seu bloco mais depressa.

### Ficheiros
- `mandel_server.c` — servidor
- `mandel_client.c` — cliente
- `MandelbrotCount.c` — versão sequencial, para comparar

### Como correr

```bash
make section4_all
./mandel_server 50000 64                     # terminal 1 (64 = linhas por bloco)
for k in 1 2 3 4; do ./mandel_client 127.0.0.1 50000 & done; wait   # terminal 2 (4 clientes)
./mandel_client 127.0.0.1 50000 8            # 1 cliente com 8 threads
```

Atenção: o servidor termina quando o trabalho acaba, por isso tem de ser arrancado de novo antes de cada teste.

### Resultados (no mesmo computador)

Todas as versões dão a **mesma contagem: 52013413**.

| Teste | Tempo | Quantas vezes mais rápido |
|---|---|---|
| Sequencial | 10.49 s | 1× |
| 1 cliente | 10.47 s | 1× |
| 4 clientes | ≈ 2.7 s | ≈ 3.9× |

O que se tira daqui:
- Com 1 cliente o tempo é praticamente igual ao sequencial: as 129 ligações gastam só uns 0.03 s, ou seja, **a comunicação quase não pesa**.
- Com 4 clientes fica quase **4 vezes mais rápido**, que é o ideal.
- Os 4 clientes fizeram 30 + 35 + 35 + 32 = 132 blocos, mais 3 do que os 129. São os blocos que foram **dados outra vez no fim**, a clientes que estavam livres. Gasta-se um pouco de trabalho repetido, mas evita-se ficar à espera de um cliente lento.

---

## 5. Por fazer: usar outro computador

Falta a última parte do guião: correr o servidor num computador e clientes noutro (ou ao contrário).

1. Descobrir o endereço IP do computador do servidor com `ip a` ou `ifconfig` (linha `inet`).
2. No outro computador, correr o cliente com esse endereço em vez de `127.0.0.1`:
   ```bash
   ./mandel_client 192.168.x.x 50000
   ```
3. Se a ligação for recusada, pode ser a firewall; usar um hotspot do telemóvel costuma resolver.

No fim, o servidor mostra quantos blocos fez cada computador (aparecem os endereços IP de cada um).
