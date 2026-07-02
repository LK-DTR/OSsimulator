// Testes do núcleo de simulação (C++ puro, sem Qt).
// Compilar e executar a partir da raiz do projeto:
//   g++ -std=c++17 tests/test_core.cpp -o test_core && ./test_core
#include "../core/simulacao.hpp"
#include <cmath>
#include <cstdio>
using namespace core;

static int falhas = 0;

static void check(bool ok, const char *nome) {
  std::printf("[%s] %s\n", ok ? "PASS" : "FALHA", nome);
  if (!ok)
    ++falhas;
}

static bool quase(double a, double b) { return std::fabs(a - b) < 0.01; }

int main() {
  // Processos do enunciado (PID, chegada, burst, prioridade, memória, páginas).
  std::vector<Proc> ps = {
      {1, 0, 5, 1, 100, {}}, {2, 2, 3, 2, 50, {}}, {3, 4, 7, 1, 200, {}}};

  // --- Escalonamento ---
  Result rr = simular(ps, Sched::RR, 2, Repl::FIFO, 1024, 4096);
  check(quase(rr.tempoMedioResposta, 0.67), "RR q=2: tempo medio de resposta = 0.67");
  check(quase(rr.tempoMedioEspera, 4.33), "RR q=2: tempo medio de espera = 4.33");
  check(rr.gantt.size() >= 1 && rr.gantt[0].pid == 1 && rr.gantt[0].inicio == 0 &&
            rr.gantt[0].fim == 2,
        "RR q=2: primeira fatia do Gantt = P1 [0,2)");

  Result sjf = simular(ps, Sched::SJF_P, 2, Repl::FIFO, 1024, 4096);
  check(sjf.metrics[0].conclusao == 5 && sjf.metrics[1].conclusao == 8 &&
            sjf.metrics[2].conclusao == 15,
        "SJF-P: conclusoes = 5, 8, 15");
  check(quase(sjf.tempoMedioEspera, 2.33), "SJF-P: tempo medio de espera = 2.33");

  Result pr = simular(ps, Sched::PRIO_P, 2, Repl::FIFO, 1024, 4096);
  check(pr.metrics[2].conclusao == 12, "Prioridade-P: P3 conclui em 12");

  // --- Substituição de páginas (string de referência clássica de Belady) ---
  std::vector<Proc> b = {{1, 0, 12, 1, 50, {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5}}};
  check(simular(b, Sched::SJF_P, 2, Repl::FIFO, 30, 4096).pageFaults == 9,
        "Memoria 3 frames FIFO: 9 page faults");
  check(simular(b, Sched::SJF_P, 2, Repl::LRU, 30, 4096).pageFaults == 10,
        "Memoria 3 frames LRU: 10 page faults");
  check(simular(b, Sched::SJF_P, 2, Repl::OPT, 30, 4096).pageFaults == 7,
        "Memoria 3 frames Otimo: 7 page faults");

  // --- Memória virtual: estouro só quando a demanda excede a capacidade ---
  check(simular(ps, Sched::SJF_P, 2, Repl::FIFO, 1024, 20).estouroVirtual,
        "Memoria virtual pequena (2 frames) sinaliza estouro");
  check(!simular(ps, Sched::SJF_P, 2, Repl::FIFO, 1024, 4096).estouroVirtual,
        "Memoria virtual ampla nao sinaliza estouro");

  // --- Cenarios adicionais de arguicao (valores validados a mao) ---
  // SJF-P: A(0,9) B(1,4) C(2,8) D(3,5)  [A=P1 B=P2 C=P3 D=P4]
  std::vector<Proc> sjf2 = {{1, 0, 9, 1, 100, {}},
                            {2, 1, 4, 1, 100, {}},
                            {3, 2, 8, 1, 100, {}},
                            {4, 3, 5, 1, 100, {}}};
  Result rs = simular(sjf2, Sched::SJF_P, 2, Repl::FIFO, 1024, 4096);
  check(rs.metrics[0].conclusao == 18 && rs.metrics[1].conclusao == 5 &&
            rs.metrics[2].conclusao == 26 && rs.metrics[3].conclusao == 10,
        "SJF-P (9,4,8,5): conclusoes 18, 5, 26, 10");
  check(quase(rs.tempoMedioResposta, 4.50) && quase(rs.tempoMedioEspera, 6.75),
        "SJF-P (9,4,8,5): resposta 4.50 / espera 6.75");

  // Round-Robin: A(0,8) B(4,5) C(9,6) D(14,7)
  std::vector<Proc> rr2 = {{1, 0, 8, 1, 100, {}},
                           {2, 4, 5, 1, 100, {}},
                           {3, 9, 6, 1, 100, {}},
                           {4, 14, 7, 1, 100, {}}};
  check(quase(simular(rr2, Sched::RR, 2, Repl::FIFO, 1024, 4096).tempoMedioEspera, 5.75),
        "RR q=2 (8,5,6,7): espera media 5.75");
  check(quase(simular(rr2, Sched::RR, 3, Repl::FIFO, 1024, 4096).tempoMedioEspera, 5.50),
        "RR q=3 (8,5,6,7): espera media 5.50");

  // Memoria: string classica (Silberschatz), 3 frames
  std::vector<Proc> mem = {
      {1, 0, 20, 1, 100, {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1}}};
  check(simular(mem, Sched::SJF_P, 2, Repl::FIFO, 30, 4096).pageFaults == 15,
        "Memoria 3 frames FIFO (string classica): 15 faults");
  check(simular(mem, Sched::SJF_P, 2, Repl::LRU, 30, 4096).pageFaults == 12,
        "Memoria 3 frames LRU (string classica): 12 faults");
  check(simular(mem, Sched::SJF_P, 2, Repl::OPT, 30, 4096).pageFaults == 9,
        "Memoria 3 frames Otimo (string classica): 9 faults");

  std::printf("\n%s (%d falha(s))\n", falhas == 0 ? "TODOS OS TESTES PASSARAM" : "HA FALHAS", falhas);
  return falhas == 0 ? 0 : 1;
}
