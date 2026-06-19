// Tipos e estruturas compartilhados do núcleo de simulação.
#pragma once
#include <cstdint>
#include <vector>

namespace core {

constexpr int PAGE_SIZE_MB = 10; // tamanho de cada página/frame em MB

enum class Sched { RR = 0, SJF_P = 1, PRIO_P = 2 };
enum class Repl { FIFO = 0, LRU = 1, OPT = 2 };

struct Proc {
  int pid = 0;
  int chegada = 0;
  int burst = 0;
  int prioridade = 0;
  int memoria = 0;
  std::vector<int> paginas; // páginas referenciadas (sequência)
};

struct Slice {
  int pid;
  int inicio;
  int fim;
};

struct ProcMetrics {
  int pid = 0;
  int chegada = 0;
  int burst = 0;
  int primeiraExec = -1;
  int conclusao = -1;
  int resposta = 0;
  int espera = 0;
  int retorno = 0;
};

struct EntradaTabela {
  int pid;
  int pagina;
};

struct Result {
  std::vector<Slice> gantt;
  std::vector<ProcMetrics> metrics;
  double tempoMedioResposta = 0.0;
  double tempoMedioEspera = 0.0;
  int pageFaults = 0;
  int numFrames = 0;        // frames de memória física
  int numFramesVirtual = 0; // capacidade da memória virtual (em páginas)
  int paginasDistintas = 0; // páginas distintas exigidas pelos processos
  bool estouroVirtual = false; // demanda excede a memória virtual
  int totalReferencias = 0;
  std::vector<EntradaTabela> tabelaPaginas; // frames residentes ao final
};

} // namespace core
