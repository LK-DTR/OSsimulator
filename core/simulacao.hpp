// Coordenação da simulação. Inclui os módulos de tipos, escalonamento e
// memória e executa a simulação completa (escalonamento + paginação).
#pragma once
#include "memory.hpp"
#include "scheduler.hpp"
#include "tipos.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>

namespace core {

// Executa a simulação completa.
inline Result simular(const std::vector<Proc> &procs, Sched sched, int quantum,
                      Repl repl, int memFisicaMB, int memVirtualMB) {
  Result r;
  std::vector<int> execTick = escalonar(procs, sched, quantum, r.metrics);
  r.gantt = compactarGantt(execTick);

  double somaResp = 0, somaEsp = 0;
  for (const auto &m : r.metrics) {
    somaResp += m.resposta;
    somaEsp += m.espera;
  }
  if (!r.metrics.empty()) {
    r.tempoMedioResposta = somaResp / r.metrics.size();
    r.tempoMedioEspera = somaEsp / r.metrics.size();
  }

  std::vector<int64_t> ref = stringReferencia(procs, execTick);
  r.totalReferencias = static_cast<int>(ref.size());

  r.numFrames = memFisicaMB / PAGE_SIZE_MB;
  if (r.numFrames < 1)
    r.numFrames = 1;
  r.numFramesVirtual = memVirtualMB / PAGE_SIZE_MB;
  if (r.numFramesVirtual < 1)
    r.numFramesVirtual = 1;

  // Páginas distintas exigidas: limitadas pela capacidade da memória virtual.
  std::vector<int64_t> distintas = ref;
  std::sort(distintas.begin(), distintas.end());
  distintas.erase(std::unique(distintas.begin(), distintas.end()),
                  distintas.end());
  r.paginasDistintas = static_cast<int>(distintas.size());
  r.estouroVirtual = r.paginasDistintas > r.numFramesVirtual;

  std::vector<int64_t> residente;
  r.pageFaults = simularMemoria(ref, r.numFrames, repl, residente);
  for (int64_t id : residente)
    r.tabelaPaginas.push_back(
        {static_cast<int>(id / 100000), static_cast<int>(id % 100000)});
  return r;
}

} // namespace core
