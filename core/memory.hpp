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

// Lista efetiva de páginas de um processo. Usa a coluna "paginas" do CSV quando
// informada; caso contrário deriva da memória: teto(memoria / PAGE_SIZE_MB)
// páginas, numeradas de 0 a n-1 (no mínimo 1).
inline std::vector<int> paginasEfetivas(const Proc &p) {
  if (!p.paginas.empty())
    return p.paginas;
  int n = (p.memoria + PAGE_SIZE_MB - 1) / PAGE_SIZE_MB; // divisão com teto
  if (n < 1)
    n = 1;
  std::vector<int> v;
  v.reserve(n);
  for (int i = 0; i < n; ++i)
    v.push_back(i);
  return v;
}

// Identificador global único de página: combina pid e número da página para que
// a página X do processo 1 não colida com a página X do processo 2.
inline int64_t pageId(int pid, int pagina) {
  return static_cast<int64_t>(pid) * 100000 + pagina; // assume pagina < 100000
}

// Constrói a string de referência de páginas a partir da ordem de execução:
// cada unidade de CPU executada gera UMA referência (a "próxima" página do
// processo, ciclicamente). É essa string completa que viabiliza o algoritmo
// Ótimo, que precisa conhecer os acessos futuros.
inline std::vector<int64_t> stringReferencia(const std::vector<Proc> &procs,
                                             const std::vector<int> &execTick) {
  // Pré-calcula as páginas de cada processo e um cursor cíclico por processo.
  std::vector<std::vector<int>> pags(procs.size());
  std::vector<int> idx(procs.size(), 0); // cursor da lista de páginas
  std::vector<int> pidPos(procs.size()); // pid de cada posição
  for (size_t i = 0; i < procs.size(); ++i) {
    pags[i] = paginasEfetivas(procs[i]);
    pidPos[i] = procs[i].pid;
  }
  // Localiza a posição (índice) de um processo a partir do seu pid.
  auto posPorPid = [&](int pid) -> int {
    for (size_t i = 0; i < procs.size(); ++i)
      if (pidPos[i] == pid)
        return static_cast<int>(i);
    return -1;
  };

  std::vector<int64_t> ref;
  for (int pid : execTick) {
    if (pid < 0)
      continue; // instante ocioso não gera referência de página
    int i = posPorPid(pid);
    if (i < 0 || pags[i].empty())
      continue;
    int pagina = pags[i][idx[i] % pags[i].size()]; // próxima página (cíclica)
    ++idx[i];
    ref.push_back(pageId(pid, pagina));
  }
  return ref;
}

// Simula a substituição de páginas sobre a string de referência. Devolve o
// total de page faults e preenche residenteFinal com o estado final dos frames
// (a tabela de páginas residentes em memória física ao término).
inline int simularMemoria(const std::vector<int64_t> &ref, int numFrames,
                          Repl pol, std::vector<int64_t> &residenteFinal) {
  if (numFrames < 1)
    numFrames = 1;
  std::vector<int64_t> residente; // páginas atualmente nos frames
  std::deque<int64_t> ordemFifo;  // ordem de carga das páginas (para o FIFO)
  std::vector<int> ultimoUso(0);  // último instante de uso por frame (para o LRU)
  int faults = 0;
  const int n = static_cast<int>(ref.size());

  // Procura uma página entre os frames; devolve o índice em "residente" ou -1.
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
      // Acerto: a página já está na memória física.
      if (pol == Repl::LRU)
        ultimoUso[pos] = i; // LRU: renova o instante de uso
      continue;
    }

    // Falta de página (page fault).
    ++faults;
    if (static_cast<int>(residente.size()) < numFrames) {
      // Há frame livre: apenas carrega a página, sem substituição.
      residente.push_back(pg);
      ultimoUso.push_back(i);
      ordemFifo.push_back(pg);
      continue;
    }

    // Frames cheios: escolhe a página vítima conforme a política.
    int vitima = -1; // índice em "residente"
    if (pol == Repl::FIFO) {
      // FIFO: remove a página que entrou há mais tempo (frente da fila).
      int64_t alvo = ordemFifo.front();
      ordemFifo.pop_front();
      vitima = indiceResidente(alvo);
    } else if (pol == Repl::LRU) {
      // LRU: remove a de menor "ultimoUso" (usada há mais tempo).
      int menor = INT_MAX;
      for (size_t k = 0; k < residente.size(); ++k)
        if (ultimoUso[k] < menor) {
          menor = ultimoUso[k];
          vitima = static_cast<int>(k);
        }
    } else {
      // Ótimo: remove a página cujo próximo uso está mais distante no futuro
      // (ou que não será mais usada). Exige conhecer a string de referência
      // inteira, por isso o escalonamento roda antes da memória.
      int maisLonge = -1;
      for (size_t k = 0; k < residente.size(); ++k) {
        int prox = INT_MAX; // se nada for encontrado, a página não é mais usada
        for (int j = i + 1; j < n; ++j)
          if (ref[j] == residente[k]) {
            prox = j; // índice do próximo uso desta página
            break;
          }
        if (prox > maisLonge) {
          maisLonge = prox;
          vitima = static_cast<int>(k);
        }
      }
    }
    if (vitima < 0)
      vitima = 0; // salvaguarda (não deve ocorrer com frames cheios)

    // Nas políticas que não usam a fila FIFO para escolher a vítima, ainda
    // removemos a página substituída da fila para mantê-la coerente com os
    // frames.
    if (pol != Repl::FIFO) {
      for (auto it = ordemFifo.begin(); it != ordemFifo.end(); ++it)
        if (*it == residente[vitima]) {
          ordemFifo.erase(it);
          break;
        }
    }
    // Efetiva a troca: a vítima dá lugar à nova página.
    residente[vitima] = pg;
    ultimoUso[vitima] = i;
    ordemFifo.push_back(pg);
  }
  residenteFinal = residente; // estado final dos frames
  return faults;
}

} // namespace core
