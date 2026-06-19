// Algoritmos de escalonamento: Round-Robin, SJF preemptivo e Prioridade
// preemptiva. Produz a linha do tempo (Gantt) e as métricas por processo.
#pragma once
#include "tipos.hpp"
#include <cstddef>
#include <deque>
#include <vector>

namespace core {

// Une fatias consecutivas do mesmo processo na linha do tempo.
inline std::vector<Slice> compactarGantt(const std::vector<int> &execTick) {
  std::vector<Slice> g;
  for (size_t t = 0; t < execTick.size(); ++t) {
    int pid = execTick[t];
    if (!g.empty() && g.back().pid == pid && g.back().fim == static_cast<int>(t))
      g.back().fim = static_cast<int>(t) + 1;
    else
      g.push_back({pid, static_cast<int>(t), static_cast<int>(t) + 1});
  }
  return g;
}

// Executa o escalonamento e devolve, por unidade de tempo, o pid em execução
// (-1 para CPU ociosa). Preenche também as métricas por processo.
inline std::vector<int> escalonar(const std::vector<Proc> &procs, Sched pol,
                                  int quantum, std::vector<ProcMetrics> &mt) {
  const int N = static_cast<int>(procs.size());
  std::vector<int> restante(N), primeira(N, -1), conclusao(N, -1);
  std::vector<bool> chegou(N, false);
  for (int i = 0; i < N; ++i)
    restante[i] = procs[i].burst;

  std::vector<int> execTick;
  int finalizados = 0, t = 0;
  if (quantum < 1)
    quantum = 1;

  std::deque<int> fila; // usada apenas no Round-Robin
  int atual = -1, usoQuantum = 0;

  auto enfileirarChegadas = [&](int tempo) {
    for (int i = 0; i < N; ++i)
      if (!chegou[i] && procs[i].chegada <= tempo) {
        chegou[i] = true;
        fila.push_back(i);
      }
  };

  const int LIMITE = 1000000; // proteção contra laço infinito
  while (finalizados < N && t < LIMITE) {
    int sel = -1;

    if (pol == Sched::RR) {
      enfileirarChegadas(t);
      if (atual == -1 && !fila.empty()) {
        atual = fila.front();
        fila.pop_front();
        usoQuantum = 0;
      }
      sel = atual;
    } else {
      // SJF preemptivo ou Prioridade preemptiva: reavalia a cada tick.
      for (int i = 0; i < N; ++i) {
        if (procs[i].chegada > t || restante[i] <= 0)
          continue;
        if (sel == -1) {
          sel = i;
          continue;
        }
        bool melhor = false;
        if (pol == Sched::SJF_P)
          melhor = restante[i] < restante[sel];
        else // PRIO_P: menor valor numérico = maior prioridade
          melhor = procs[i].prioridade < procs[sel].prioridade;
        if (melhor)
          sel = i;
      }
    }

    if (sel == -1) {
      execTick.push_back(-1); // CPU ociosa
      ++t;
      continue;
    }

    if (primeira[sel] < 0)
      primeira[sel] = t;
    execTick.push_back(procs[sel].pid);
    --restante[sel];
    ++t;

    if (pol == Sched::RR) {
      ++usoQuantum;
      if (restante[sel] == 0) {
        conclusao[sel] = t;
        ++finalizados;
        atual = -1;
        usoQuantum = 0;
      } else if (usoQuantum == quantum) {
        enfileirarChegadas(t);
        fila.push_back(sel);
        atual = -1;
        usoQuantum = 0;
      }
    } else {
      if (restante[sel] == 0) {
        conclusao[sel] = t;
        ++finalizados;
      }
    }
  }

  mt.clear();
  for (int i = 0; i < N; ++i) {
    ProcMetrics m;
    m.pid = procs[i].pid;
    m.chegada = procs[i].chegada;
    m.burst = procs[i].burst;
    m.primeiraExec = primeira[i];
    m.conclusao = conclusao[i];
    m.resposta = (primeira[i] >= 0) ? primeira[i] - procs[i].chegada : 0;
    m.retorno = (conclusao[i] >= 0) ? conclusao[i] - procs[i].chegada : 0;
    m.espera = m.retorno - procs[i].burst;
    if (m.espera < 0)
      m.espera = 0;
    mt.push_back(m);
  }
  return execTick;
}

} // namespace core
