# AAD_P5 — Resumo do que foi feito

Neste guião volta-se a dividir um cálculo por **vários processos**, mas agora com **MPI** (*Message Passing Interface*), a forma "fácil" e padrão de o fazer (é o que se usa nos supercomputadores).

A grande diferença para o P4:
- no P4 tínhamos de arrancar o servidor e os clientes à mão, dar-lhes o endereço IP, e escrever todo o código de sockets;
- com MPI, o comando `mpirun` arranca todos os processos de uma vez e o MPI trata da comunicação entre eles.

### Ideias base
- `mpirun -np 4 ./programa` corre **4 cópias do mesmo programa**.
- Cada cópia recebe um número diferente, o **rank** (de 0 a 3). É com ele que cada processo decide que parte do trabalho faz.
- Os processos **não partilham memória** (ao contrário das threads do P2 e P3). Para trocarem dados têm de enviar **mensagens**.
- Todos os programas MPI começam com `MPI_Init` e acabam com `MPI_Finalize`.

---

## 1. Hello world (secção 3)

Instalou-se o OpenMPI:

```bash
sudo apt install openmpi-bin openmpi-doc libopenmpi-dev
```

Os programas MPI compilam-se com `mpicc` (em vez de `cc`) e correm-se com `mpirun`.

```bash
make run_hello
```

**Resultado:** apareceram 4 linhas `Hello world from X/4 [ASUS-PEDROVENDA]`, uma por processo. A ordem muda de cada vez que se corre, porque os processos correm ao mesmo tempo e cada um escreve quando calha.

---

## 2. Mensagens entre dois processos e deadlock (secção 4)

O programa `one_to_one_communication_mpi.c` tem três partes:

1. **O rank 0 envia 10 números ao rank 1** com `MPI_Send`, e o rank 1 recebe-os com `MPI_Recv`. Os outros ranks não fazem nada.
2. **Barreira** (`MPI_Barrier`): ninguém passa daqui até todos lá chegarem.
3. **Anel**: cada rank envia o seu número ao seguinte e recebe o do anterior (o último envia ao 0).

### O problema da parte 3

Na parte 3, **todos enviam primeiro e só depois recebem**.
- Com `MPI_Send` funciona, porque mensagens pequenas ficam guardadas numa espécie de "caixa de correio" e o envio termina logo.
- Trocando por **`MPI_Ssend`** (envio síncrono, que só termina quando o outro lado recebe), o programa **fica parado para sempre**: todos estão à espera de que alguém receba, mas ninguém chega a receber. Isto é um **deadlock**.

Foi testado: depois da barreira o programa não avança e é preciso fazer Ctrl+C.

### As duas correções

Para comparar, a parte 3 ficou com três versões, escolhidas na compilação:

| Versão | Executável | Como funciona |
|---|---|---|
| Deadlock | `one_to_one_deadlock` | Todos enviam e depois recebem → bloqueia |
| Par/ímpar | `one_to_one_evenodd` | Os ranks pares enviam e depois recebem; os ímpares recebem e depois enviam. Assim há sempre alguém a receber |
| `MPI_Sendrecv` | `one_to_one_sendrecv` | Uma só função que envia e recebe ao mesmo tempo, sem risco de bloquear |

```bash
make section4_all
mpirun -np 4 ./one_to_one_deadlock    # fica parado (Ctrl+C para sair)
mpirun -np 4 ./one_to_one_evenodd     # funciona
mpirun -np 4 ./one_to_one_sendrecv    # funciona
```

**Resultado:** as duas correções funcionam, cada rank recebe o número do anterior (o 0 recebe o 3, o 1 recebe o 0, etc.). Também funcionam com 3 processos.

Pormenor: na versão par/ímpar e na do `Sendrecv`, o número a enviar tem de ser guardado numa variável à parte, porque a receção escreve por cima de `data[0]`.

---

## 3. Enviar uma estrutura a todos (secção 5)

O programa `one_to_all_communication_mpi.c` envia uma `struct` (2 inteiros e 3 doubles) do rank 0 para todos os outros.

- O MPI só conhece tipos simples (`MPI_INT`, `MPI_DOUBLE`...). Para enviar uma `struct` é preciso **descrevê-la**: que tipos tem, quantos de cada, e em que posição estão dentro da `struct` (`offsetof`). A posição é importante porque o compilador pode deixar espaços vazios entre campos.
- Só o rank 0 preenche os dados.
- **`MPI_Bcast`** envia os dados do rank 0 para todos. A mesma linha de código corre em todos os ranks: no rank 0 envia, nos outros recebe.

```bash
make one_to_all && mpirun -np 4 ./one_to_all
```

**Resultado:** os 4 ranks mostram os mesmos valores (`i[0]=3 i[1]=-1 d[0]=1.234 d[1]=5.678 d[2]=-2`).

---

## 4. Mandelbrot com MPI (secção 6)

Foram feitas duas versões no mesmo programa (`MandelbrotCountMPI.c`):

### Versão `cyclic` (por defeito) — divisão fixa
- Cada rank faz as linhas `rank`, `rank + N`, `rank + 2N`, ... (linhas intercaladas, como no P2).
- Como as linhas mais pesadas (no centro do Mandelbrot) ficam bem espalhadas, todos os ranks acabam quase ao mesmo tempo.
- No fim, **`MPI_Reduce`** soma as contagens de todos os ranks no rank 0, numa só linha.

### Versão `dynamic` — mestre e trabalhadores
- É a mesma ideia do P4, mas com MPI: o rank 0 é o "mestre" e vai entregando blocos de 64 linhas aos outros ranks; cada um calcula, devolve a contagem e recebe outro bloco.
- As mensagens são distinguidas pela *tag*: trabalho, parar, ou resultado.
- Quem apanha blocos mais leves acaba mais cedo e pede mais, por isso o trabalho fica equilibrado sozinho.
- Desvantagem: o rank 0 só distribui, não calcula.

No fim, o rank 0 junta a informação de todos (`MPI_Gather`) e mostra-a por ordem, porque se cada rank escrevesse a sua linha elas apareciam baralhadas.

```bash
make section6_all
./MandelbrotCountSeq
mpirun -np 4 ./MandelbrotCountMPI                    # cyclic
mpirun -np 5 ./MandelbrotCountMPI dynamic            # 1 mestre + 4 trabalhadores
mpirun --oversubscribe -np 16 ./MandelbrotCountMPI   # 16 processos
```

### Resultados

Todas as versões dão a **mesma contagem: 52013413**.

| Teste | Tempo | Quantas vezes mais rápido |
|---|---|---|
| Sequencial | 23.39 s | 1× |
| cyclic, 4 processos | 6.00 s | 3.9× |
| dynamic, 5 processos (4 a calcular) | 5.94 s | 3.9× |
| cyclic, 16 processos | 1.65 s | 14.2× |

O que se tira daqui:
- Com 4 processos fica quase **4 vezes mais rápido**, o ideal.
- Na versão `cyclic` todos os ranks demoram praticamente o mesmo (com 16: entre 1.58 s e 1.65 s) → a divisão intercalada equilibra bem o trabalho.
- Na versão `dynamic` os ranks fazem números de blocos diferentes (35, 35, 30, 29) mas acabam todos ao mesmo tempo → equilíbrio automático.
- Com 16 processos não chega a 16× porque o computador tem 16 threads mas (provavelmente) só 8 núcleos físicos; duas threads no mesmo núcleo não rendem o dobro.
- Comparado com o P4, o resultado é o mesmo mas com **muito menos código**: não há sockets, endereços IP, conversão de bytes nem arranque manual de processos.

Nota: neste dia o sequencial demorou 23 s (no P4 tinha demorado 10.5 s). O computador estava mais lento (provavelmente a bateria / modo de poupança), mas as comparações continuam válidas porque todas as medições foram feitas na mesma altura.

---

## 5. Opcional, não feito: correr processos noutro computador

A secção 3.1 do guião explica como usar o `mpirun` para lançar processos também no computador de um colega (por SSH, com chaves sem password):

```bash
mpirun -np 8 --host localhost:4,amigo:4 ./hello
```

Isto lançaria 4 processos neste computador e 4 no do colega. No fim é preciso desfazer as alterações no computador do colega (chaves SSH, programa copiado e servidor SSH).
