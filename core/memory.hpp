// Gerência de memória virtual paginada: geração da string de referência e
// substituição de páginas (FIFO, LRU e Ótimo).
#pragma once
#include "tipos.hpp"
#include <climits>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

namespace core {

// Gera a lista efetiva de páginas de um processo. Usa a coluna "paginas" quando
// informada; caso contrário deriva a partir da memória necessária.
inline std::vector<int> paginasEfetivas(const Proc &p) {
  if (!p.paginas.empty())
    return p.paginas;
  int n = (p.memoria + PAGE_SIZE_MB - 1) / PAGE_SIZE_MB;
  if (n < 1)
    n = 1;
  std::vector<int> v;
  v.reserve(n);
  for (int i = 0; i < n; ++i)
    v.push_back(i);
  return v;
}

// Identificador global de página (combina pid e número da página).
inline int64_t pageId(int pid, int pagina) {
  return static_cast<int64_t>(pid) * 100000 + pagina;
}

// Constrói a string de referência de páginas a partir da ordem de execução.
inline std::vector<int64_t> stringReferencia(const std::vector<Proc> &procs,
                                             const std::vector<int> &execTick) {
  std::vector<std::vector<int>> pags(procs.size());
  std::vector<int> idx(procs.size(), 0);
  std::vector<int> pidPos(procs.size());
  for (size_t i = 0; i < procs.size(); ++i) {
    pags[i] = paginasEfetivas(procs[i]);
    pidPos[i] = procs[i].pid;
  }
  auto posPorPid = [&](int pid) -> int {
    for (size_t i = 0; i < procs.size(); ++i)
      if (pidPos[i] == pid)
        return static_cast<int>(i);
    return -1;
  };

  std::vector<int64_t> ref;
  for (int pid : execTick) {
    if (pid < 0)
      continue; // ociosa não gera referência
    int i = posPorPid(pid);
    if (i < 0 || pags[i].empty())
      continue;
    int pagina = pags[i][idx[i] % pags[i].size()];
    ++idx[i];
    ref.push_back(pageId(pid, pagina));
  }
  return ref;
}

// Simula a substituição de páginas sobre a string de referência. Devolve o
// total de page faults e preenche o estado final dos frames (residenteFinal),
// que representa a tabela de páginas residentes em memória física ao término.
inline int simularMemoria(const std::vector<int64_t> &ref, int numFrames,
                          Repl pol, std::vector<int64_t> &residenteFinal) {
  if (numFrames < 1)
    numFrames = 1;
  std::vector<int64_t> residente; // páginas atualmente em memória
  std::deque<int64_t> ordemFifo;  // ordem de carga (FIFO)
  std::vector<int> ultimoUso(0);  // alinhado a residente (LRU)
  int faults = 0;
  const int n = static_cast<int>(ref.size());

  auto indiceResidente = [&](int64_t pg) -> int {
    for (size_t k = 0; k < residente.size(); ++k)
      if (residente[k] == pg)
        return static_cast<int>(k);
    return -1;
  };

  for (int i = 0; i < n; ++i) {
    int64_t pg = ref[i];
    int pos = indiceResidente(pg);
    if (pos >= 0) {
      if (pol == Repl::LRU)
        ultimoUso[pos] = i;
      continue; // acerto
    }

    ++faults;
    if (static_cast<int>(residente.size()) < numFrames) {
      residente.push_back(pg);
      ultimoUso.push_back(i);
      ordemFifo.push_back(pg);
      continue;
    }

    int vitima = -1; // índice em residente
    if (pol == Repl::FIFO) {
      int64_t alvo = ordemFifo.front();
      ordemFifo.pop_front();
      vitima = indiceResidente(alvo);
    } else if (pol == Repl::LRU) {
      int menor = INT_MAX;
      for (size_t k = 0; k < residente.size(); ++k)
        if (ultimoUso[k] < menor) {
          menor = ultimoUso[k];
          vitima = static_cast<int>(k);
        }
    } else { // OPT: substitui a página com uso mais distante no futuro
      int maisLonge = -1;
      for (size_t k = 0; k < residente.size(); ++k) {
        int prox = INT_MAX;
        for (int j = i + 1; j < n; ++j)
          if (ref[j] == residente[k]) {
            prox = j;
            break;
          }
        if (prox > maisLonge) {
          maisLonge = prox;
          vitima = static_cast<int>(k);
        }
      }
    }
    if (vitima < 0)
      vitima = 0;

    if (pol != Repl::FIFO) {
      // mantém a fila FIFO coerente removendo a página substituída
      for (auto it = ordemFifo.begin(); it != ordemFifo.end(); ++it)
        if (*it == residente[vitima]) {
          ordemFifo.erase(it);
          break;
        }
    }
    residente[vitima] = pg;
    ultimoUso[vitima] = i;
    ordemFifo.push_back(pg);
  }
  residenteFinal = residente;
  return faults;
}

} // namespace core
