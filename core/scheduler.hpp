// Algoritmos de escalonamento: Round-Robin, SJF preemptivo e Prioridade
// preemptiva. Produz a linha do tempo (Gantt) e as métricas por processo.
#pragma once
#include "tipos.hpp"
#include <cstddef>
#include <deque>
#include <vector>

namespace core {

// Une fatias consecutivas do mesmo processo na linha do tempo. Recebe o vetor
// "um pid por unidade de tempo" e agrupa instantes contíguos do mesmo processo
// em blocos (Slice), formando o Gantt.
inline std::vector<Slice> compactarGantt(const std::vector<int> &execTick) {
  std::vector<Slice> g;
  for (size_t t = 0; t < execTick.size(); ++t) {
    int pid = execTick[t];
    // Se a última fatia é do mesmo processo e termina exatamente neste instante,
    // apenas a estende; caso contrário, abre uma nova fatia.
    if (!g.empty() && g.back().pid == pid && g.back().fim == static_cast<int>(t))
      g.back().fim = static_cast<int>(t) + 1;
    else
      g.push_back({pid, static_cast<int>(t), static_cast<int>(t) + 1});
  }
  return g;
}

// Executa o escalonamento e devolve, por unidade de tempo, o pid em execução
// (-1 para CPU ociosa). Preenche também as métricas por processo (mt).
inline std::vector<int> escalonar(const std::vector<Proc> &procs, Sched pol,
                                  int quantum, std::vector<ProcMetrics> &mt) {
  const int N = static_cast<int>(procs.size());
  // Estado por processo (indexado pela posição i na entrada, não pelo pid):
  //   restante = CPU que ainda falta; primeira = instante da 1a execução;
  //   conclusao = instante de término. -1 significa "ainda não ocorreu".
  std::vector<int> restante(N), primeira(N, -1), conclusao(N, -1);
  std::vector<bool> chegou(N, false); // se o processo já entrou na fila (RR)
  for (int i = 0; i < N; ++i)
    restante[i] = procs[i].burst;

  std::vector<int> execTick;      // saída: pid em execução a cada instante
  int finalizados = 0, t = 0;     // processos concluídos e relógio atual
  if (quantum < 1)
    quantum = 1;                  // proteção: quantum mínimo de 1

  std::deque<int> fila;           // fila circular do Round-Robin
  int atual = -1, usoQuantum = 0; // processo na CPU e ticks já usados (RR)

  // Coloca na fila todo processo que já chegou e ainda não foi enfileirado.
  auto enfileirarChegadas = [&](int tempo) {
    for (int i = 0; i < N; ++i)
      if (!chegou[i] && procs[i].chegada <= tempo) {
        chegou[i] = true;
        fila.push_back(i);
      }
  };

  const int LIMITE = 1000000; // proteção contra laço infinito
  // Laço principal: cada iteração corresponde a uma unidade de tempo.
  while (finalizados < N && t < LIMITE) {
    int sel = -1; // índice do processo escolhido para executar neste tick

    if (pol == Sched::RR) {
      // Round-Robin: escolhe pela fila; só troca de processo quando a CPU fica
      // livre (o atual terminou ou estourou o quantum).
      enfileirarChegadas(t);
      if (atual == -1 && !fila.empty()) {
        atual = fila.front();
        fila.pop_front();
        usoQuantum = 0;
      }
      sel = atual;
    } else {
      // SJF preemptivo / Prioridade preemptiva: reavalia o melhor a cada tick
      // (é essa reavaliação que gera a preempção). Busca do tipo argmin entre
      // os processos prontos.
      for (int i = 0; i < N; ++i) {
        if (procs[i].chegada > t || restante[i] <= 0)
          continue; // ignora quem não chegou ainda ou já terminou
        if (sel == -1) {
          sel = i; // primeiro elegível vira o candidato inicial
          continue;
        }
        bool melhor = false;
        if (pol == Sched::SJF_P)
          melhor = restante[i] < restante[sel]; // menor tempo restante
        else // PRIO_P: menor valor numérico = maior prioridade
          melhor = procs[i].prioridade < procs[sel].prioridade;
        if (melhor) // comparação com "<": em empate mantém o de menor índice
          sel = i;
      }
    }

    // Nenhum processo pronto: CPU ociosa neste instante.
    if (sel == -1) {
      execTick.push_back(-1);
      ++t;
      continue;
    }

    // --- Executa 1 unidade de tempo do processo escolhido ---
    if (primeira[sel] < 0)
      primeira[sel] = t; // 1a execução: registra para o tempo de resposta
    execTick.push_back(procs[sel].pid);
    --restante[sel]; // consumiu uma unidade de CPU
    ++t;             // após isto, t vale o FIM deste tick

    // --- Contabilidade pós-tick: término e preempção ---
    if (pol == Sched::RR) {
      ++usoQuantum;
      if (restante[sel] == 0) {
        conclusao[sel] = t; // terminou
        ++finalizados;
        atual = -1; // libera a CPU
        usoQuantum = 0;
      } else if (usoQuantum == quantum) {
        // Estourou o quantum: as chegadas deste instante entram na fila ANTES
        // de o processo preemptado voltar ao fim (convenção do Round-Robin).
        enfileirarChegadas(t);
        fila.push_back(sel);
        atual = -1;
        usoQuantum = 0;
      }
      // Se não terminou nem estourou o quantum, o mesmo processo segue no
      // próximo tick.
    } else {
      // SJF/Prioridade: só verifica o término; a próxima seleção acontece
      // naturalmente no início do próximo tick.
      if (restante[sel] == 0) {
        conclusao[sel] = t;
        ++finalizados;
      }
    }
  }

  // --- Consolida as métricas por processo (na ordem original da entrada) ---
  mt.clear();
  for (int i = 0; i < N; ++i) {
    ProcMetrics m;
    m.pid = procs[i].pid;
    m.chegada = procs[i].chegada;
    m.burst = procs[i].burst;
    m.primeiraExec = primeira[i];
    m.conclusao = conclusao[i];
    // resposta = 1a execução - chegada; retorno (turnaround) = conclusão -
    // chegada; espera = retorno - burst (limitada a >= 0).
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
