# AAD_P6 — Resumo do que foi feito

Neste guião o cálculo passa a ser feito na **placa gráfica (GPU)**, usando **CUDA** (só para placas NVIDIA) e **OpenCL** (para qualquer tipo de dispositivo).

Máquina usada: portátil com **NVIDIA GeForce RTX 3050 Laptop GPU** e processador **AMD Ryzen 7 5800HS**, através do WSL (Ubuntu).

---

## 1. Como funciona uma GPU (secção 2, teoria)

- Uma GPU tem muitos núcleos, e cada núcleo executa a mesma instrução em **32 "faixas" ao mesmo tempo** (como as instruções SIMD do processador, mas muito mais largas).
- Em CUDA escreve-se o código para **uma só thread** (o *kernel*), e a GPU corre-o em milhares de threads ao mesmo tempo.
- As threads organizam-se assim:
  - **warp** — grupo de 32 threads que executam sempre a mesma instrução, em conjunto;
  - **bloco** — grupo de warps que corre no mesmo núcleo (até 1024 threads);
  - **grid** — conjunto de todos os blocos.
- Cada thread sabe quem é através de `threadIdx` (posição no bloco) e `blockIdx` (posição do bloco na grid). O número global de uma thread é `threadIdx.x + blockDim.x * blockIdx.x`.

Conselhos do guião para o código ser rápido:
- as 32 threads de um warp devem aceder a **posições de memória seguidas**;
- evitar `if`/`switch` em que threads do mesmo warp vão por caminhos diferentes (*divergência*): a GPU tem de executar os dois caminhos;
- fazer muitas contas por cada acesso à memória e **reduzir as transferências** entre o processador e a GPU.

---

## 2. Preparação do ambiente

Instalou-se no Ubuntu o compilador CUDA (`nvcc`) e o OpenCL:

```bash
sudo apt install nvidia-cuda-toolkit pocl-opencl-icd ocl-icd-opencl-dev clinfo
```

A `makefile` do professor estava feita para outra máquina, por isso foi adaptada:

| O quê | Professor | Esta máquina |
|---|---|---|
| Arquitetura da GPU | `sm_75` (GTX 1660 Ti) | `sm_86` (RTX 3050) |
| Pasta do CUDA | `/usr/local/cuda` | `/usr` (pacote do Ubuntu) |
| Biblioteca `libcuda` | `/usr/local/cuda/lib64` | `/usr/lib/wsl/lib` (a do driver do Windows) |
| OpenCL | o da NVIDIA | o **PoCL**, que corre no processador |

Limitações do WSL encontradas:
- a NVIDIA **não disponibiliza OpenCL dentro do WSL**, só CUDA → o OpenCL correu no processador;
- o `nvprof` **não funciona em placas RTX 30** → os tempos foram medidos dentro dos programas.

---

## 3. "Hello world" em CUDA, a forma fácil (secção 3)

O `hello.cu` usa a **runtime API** (funções `cuda...`). As funções que correm na GPU têm `__global__` e chamam-se com `<<<blocos, threads>>>`:

```bash
make hello && ./hello | less
```

**Resultado:**
- primeira grid: 1 bloco de 32 threads → aparecem as threads 0 a 31 por ordem (um só warp);
- segunda grid: 8 blocos (4×2) de 32 threads (2×16) → os blocos aparecem por ordem aleatória, porque correm em núcleos diferentes da GPU.

---

## 4. "Hello world" em CUDA, a forma difícil (secção 4)

O `hello_cuda_driver_api` usa a **driver API** (funções `cu...`). O código da GPU (`.cu`) é compilado à parte para um ficheiro `.cubin`, e o programa do processador (`.c`) tem de fazer tudo à mão: iniciar o CUDA, escolher a placa, criar um contexto, carregar o `.cubin`, reservar memória na GPU, lançar o kernel, copiar o resultado de volta e libertar tudo.

```bash
make hello_cuda_driver_api && ./hello_cuda_driver_api
```

**Resultado:** `CUDA code running on a NVIDIA GeForce RTX 3050 Laptop GPU`, e depois "Hello, world!" letra a letra — cada letra escrita por uma thread diferente; as threads 14 a 127 escrevem `X`.

---

## 5. "Hello world" em OpenCL (secção 5)

Faz o mesmo que a secção 4, mas em OpenCL: `__kernel` em vez de `__global__`, `get_global_id()` para saber o número da thread, e o código da GPU é lido de um ficheiro `.cl` e compilado **durante a execução**.

```bash
make hello_open_cl && ./hello_open_cl
```

**Resultado:** o mesmo "Hello, world!", mas a correr no **processador AMD Ryzen** (através do PoCL). Mostra a vantagem do OpenCL: o mesmo código corre em qualquer tipo de dispositivo.

---

## 6. Filtro de mediana (secção 6)

O programa pega na imagem do Mandelbrot do P1 (2049 × 2049 pixels) e substitui cada pixel pela **mediana dos 9 pixels à sua volta**. Cada thread trata de um pixel. Para ordenar os 9 valores usa uma *sorting network* feita só com `min` e `max`, sem `if`, para não haver divergência.

### Correções necessárias
1. **Crash (segmentation fault):** o programa lançava o kernel e começava logo a escrever a imagem, sem esperar que a GPU acabasse. No WSL o processador não pode mexer na memória "managed" enquanto a GPU trabalha, por isso rebentava. Acrescentou-se `cudaDeviceSynchronize()` depois do kernel. (Mesmo em Linux normal era um erro: podia ler pixels ainda não calculados.)
2. **Medição de tempos:** como o `nvprof` não funciona nesta placa, o tempo do kernel é medido com eventos CUDA.
3. O tamanho do bloco passou a poder ser dado na linha de comandos (`./median_filter_cuda 16 16`).

```bash
make median_filter_cuda
./median_filter_cuda            # blocos 32x32
./median_filter_cuda 16 16      # outro tamanho de bloco
```

### Resultados — tamanho do bloco

| Bloco | Tempo |
|---|---|
| **32×32** | **4.2 ms** (o melhor) |
| 32×8 | 4.7 ms |
| 16×16 | 5.4 ms |
| 32×2 | 7.1 ms |
| 8×8 | 8.6 ms |
| 128×1, 1024×1 | ≈ 8.3–8.6 ms |
| **1×32** | **37 ms** (o pior) |

O que se tira daqui:
- **A largura do bloco deve ser 32**: assim as 32 threads de um warp leem 32 pixels seguidos da mesma linha, e a memória entrega-os de uma vez. Com **1×32**, cada thread do warp lê numa linha diferente → 32 acessos separados → **9 vezes mais lento**.
- Blocos de uma só linha (128×1, 1024×1) são piores do que blocos "quadrados", porque cada pixel precisa dos vizinhos de cima e de baixo, que num bloco quadrado são partilhados.

### A memória "managed" tem um custo
- A memória "managed" pode ser usada tanto pelo processador como pela GPU, o que é cómodo.
- Mas no WSL os dados passam pela ligação entre o processador e a GPU sempre que o kernel corre. Num teste à parte, com memória normal da GPU (`cudaMalloc`), o mesmo kernel demorou só **0.43 ms em vez de 4.2 ms** (10 vezes menos).
- O guião sugere `cudaMemPrefetchAsync()` (mudar `#if 0` para `#if 1`) para mover os dados para a GPU antes do kernel, mas **no WSL isto não é suportado**.

---

## 7. Mandelbrot em CUDA (secção 7)

Ficheiro: `MandelbrotCountCUDA.cu`.

### Como funciona
- **Cada thread calcula um só ponto** dos 8193 × 8193 (cerca de 67 milhões de threads).
- Threads seguidas de um warp ficam com pontos vizinhos, que costumam precisar de um número parecido de iterações → menos divergência.
- **Contagem:** dentro de cada bloco, `__syncthreads_count()` conta quantas threads encontraram um ponto do conjunto; depois **cada bloco faz um só `atomicAdd`** para somar ao total. (Se cada uma das 67 milhões de threads fizesse `atomicAdd`, ficariam todas em fila.)
- O mesmo código serve para `float` e `double`, e pode também correr no processador (opção `check`) para comparar.
- O guião pede `float`, porque as placas gráficas são muito lentas em `double`.

```bash
make section7_all
./MandelbrotCountCUDA float check
./MandelbrotCountCUDA double check
```

### Resultados

| Versão | Tempo | Contagem |
|---|---|---|
| CPU, double, 1 núcleo (sequencial) | 10.29 s | 52013413 |
| CPU, float, 1 núcleo | 10.36 s | 52013483 |
| **GPU, float** | **0.028 s** | 52013483 |
| GPU, double | 0.947 s | 52013413 |

O que se tira daqui:
- Em `float`, **a GPU é cerca de 370 vezes mais rápida** do que um núcleo do processador. (Com OpenMP e 16 threads, nos guiões anteriores, demorava cerca de 1.5 s.)
- Em `double`, a GPU é **35 vezes mais lenta** do que em `float` — como o guião avisa, estas placas têm muito pouca capacidade para contas em `double`.
- A contagem em `float` é um pouco diferente da de `double`: com menos precisão, alguns pontos na fronteira do Mandelbrot mudam de lado. Mas a **GPU e o processador dão exatamente o mesmo resultado** quando usam a mesma precisão, o que mostra que o programa está certo.
- O tamanho do bloco quase não muda nada aqui (0.027–0.038 s), ao contrário do filtro de mediana: este cálculo quase não usa memória, é praticamente só contas.
- Compila-se com `-fmad=false` para a GPU não juntar multiplicações e somas numa só instrução (FMA), que arredonda de forma ligeiramente diferente. Sem esta opção fica um pouco mais rápido (0.025 s), mas a contagem deixa de ser igual à do processador (52013591).

---

## 8. Limpeza (pedida no fim da secção 6)

```bash
make clean
rm MandelbrotSet.pgm
```
