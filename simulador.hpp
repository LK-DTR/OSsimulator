#pragma once
#include "core/simulacao.hpp"
#include <QDebug>
#include <cmath>
#include <QFile>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

class Simulador : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString relatorio READ relatorio NOTIFY resultadoPronto)
  Q_PROPERTY(QVariantList gantt READ gantt NOTIFY resultadoPronto)
public:
  explicit Simulador(QObject *parent = nullptr) : QObject(parent) {}

  QString relatorio() const { return m_relatorio; }
  QVariantList gantt() const { return m_gantt; }

signals:
  void resultadoPronto();

public:

  struct Processo {
    int pid = 0;
    int chegada = 0;
    int burst = 0;
    int prioridade = 0;
    int memoria = 0;
    QVector<int> paginas;
    QString raw;
  };

  Q_INVOKABLE void iniciarSimulacao(QString caminhoCSV, int memFisica,
                                    int memVirtual, int escalonamento,
                                    int quantum, int politicaMemoria) {
    qDebug() << "--- INICIANDO SIMULAÇÃO ---" << caminhoCSV;

    if (caminhoCSV.isEmpty()) {
      mostrarErro("Nenhum arquivo CSV selecionado. Use o botão Procurar.");
      return;
    }

    QFile file(caminhoCSV);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      mostrarErro("Falha ao abrir o arquivo:\n" + caminhoCSV + "\n" +
                  file.errorString());
      return;
    }
    const QString conteudo = QTextStream(&file).readAll();
    file.close();

    // Faz o parse (a leitura do arquivo e o parse ficam separados de propósito,
    // para que o parse possa ser testado pelo botão "Validar Testes").
    QString erro;
    QList<Processo> processos = lerProcessos(conteudo, erro);
    if (!erro.isEmpty()) {
      mostrarErro(erro);
      return;
    }

    // Converte os processos lidos para as estruturas do núcleo de simulação.
    std::vector<core::Proc> entrada;
    entrada.reserve(processos.size());
    for (const Processo &p : processos) {
      core::Proc cp;
      cp.pid = p.pid;
      cp.chegada = p.chegada;
      cp.burst = p.burst;
      cp.prioridade = p.prioridade;
      cp.memoria = p.memoria;
      cp.paginas = std::vector<int>(p.paginas.begin(), p.paginas.end());
      entrada.push_back(cp);
    }

    core::Sched sched = static_cast<core::Sched>(escalonamento);
    core::Repl repl = static_cast<core::Repl>(politicaMemoria);
    core::Result res =
        core::simular(entrada, sched, quantum, repl, memFisica, memVirtual);

    construirResultado(res, escalonamento, repl, memFisica, memVirtual, quantum);
    emit resultadoPronto();
  }

  // Roda os cenários de teste do núcleo e do parser (mesmos valores do
  // tests/test_core.cpp) e exibe o resultado na área de relatório.
  Q_INVOKABLE void validarTestes() {
    QString r = "=== VALIDAÇÃO DOS TESTES ===\n\n";
    int falhas = 0;
    auto quase = [](double a, double b) { return std::fabs(a - b) < 0.01; };
    auto check = [&](bool ok, const QString &nome) {
      r += (ok ? "[PASS]  " : "[FALHA] ") + nome + "\n";
      if (!ok)
        ++falhas;
    };

    using namespace core;
    // Processos do enunciado.
    std::vector<Proc> ps = {
        {1, 0, 5, 1, 100, {}}, {2, 2, 3, 2, 50, {}}, {3, 4, 7, 1, 200, {}}};

    Result rr = simular(ps, Sched::RR, 2, Repl::FIFO, 1024, 4096);
    check(quase(rr.tempoMedioResposta, 0.67) && quase(rr.tempoMedioEspera, 4.33),
          "RR q=2 (enunciado): resposta 0.67 / espera 4.33");
    check(rr.gantt.size() >= 1 && rr.gantt[0].pid == 1 &&
              rr.gantt[0].inicio == 0 && rr.gantt[0].fim == 2,
          "RR q=2 (enunciado): 1a fatia do Gantt = P1 [0,2)");

    Result sjf = simular(ps, Sched::SJF_P, 2, Repl::FIFO, 1024, 4096);
    check(sjf.metrics[0].conclusao == 5 && sjf.metrics[1].conclusao == 8 &&
              sjf.metrics[2].conclusao == 15,
          "SJF-P (enunciado): conclusoes 5, 8, 15");
    check(quase(sjf.tempoMedioEspera, 2.33),
          "SJF-P (enunciado): espera media 2.33");

    check(simular(ps, Sched::PRIO_P, 2, Repl::FIFO, 1024, 4096)
                  .metrics[2]
                  .conclusao == 12,
          "Prioridade-P (enunciado): P3 conclui em 12");

    // Substituição de páginas (string de Belady).
    std::vector<Proc> bel = {
        {1, 0, 12, 1, 50, {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5}}};
    check(simular(bel, Sched::SJF_P, 2, Repl::FIFO, 30, 4096).pageFaults == 9,
          "Memoria 3 frames FIFO (Belady): 9 page faults");
    check(simular(bel, Sched::SJF_P, 2, Repl::LRU, 30, 4096).pageFaults == 10,
          "Memoria 3 frames LRU (Belady): 10 page faults");
    check(simular(bel, Sched::SJF_P, 2, Repl::OPT, 30, 4096).pageFaults == 7,
          "Memoria 3 frames Otimo (Belady): 7 page faults");

    // Memória virtual: estouro só quando a demanda excede a capacidade.
    check(simular(ps, Sched::SJF_P, 2, Repl::FIFO, 1024, 20).estouroVirtual,
          "Memoria virtual pequena (2 frames): sinaliza estouro");
    check(!simular(ps, Sched::SJF_P, 2, Repl::FIFO, 1024, 4096).estouroVirtual,
          "Memoria virtual ampla: sem estouro");

    // Cenários adicionais: SJF A(0,9) B(1,4) C(2,8) D(3,5).
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

    // Round-Robin A(0,8) B(4,5) C(9,6) D(14,7).
    std::vector<Proc> rr2 = {{1, 0, 8, 1, 100, {}},
                             {2, 4, 5, 1, 100, {}},
                             {3, 9, 6, 1, 100, {}},
                             {4, 14, 7, 1, 100, {}}};
    check(quase(simular(rr2, Sched::RR, 2, Repl::FIFO, 1024, 4096).tempoMedioEspera,
                5.75),
          "RR q=2 (8,5,6,7): espera media 5.75");
    check(quase(simular(rr2, Sched::RR, 3, Repl::FIFO, 1024, 4096).tempoMedioEspera,
                5.50),
          "RR q=3 (8,5,6,7): espera media 5.50");

    // Memória: string clássica (Silberschatz), 3 frames.
    std::vector<Proc> mem = {{1, 0, 20, 1, 100, {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0,
                                                 3, 2, 1, 2, 0, 1, 7, 0, 1}}};
    check(simular(mem, Sched::SJF_P, 2, Repl::FIFO, 30, 4096).pageFaults == 15,
          "Memoria 3 frames FIFO (classica): 15 page faults");
    check(simular(mem, Sched::SJF_P, 2, Repl::LRU, 30, 4096).pageFaults == 12,
          "Memoria 3 frames LRU (classica): 12 page faults");
    check(simular(mem, Sched::SJF_P, 2, Repl::OPT, 30, 4096).pageFaults == 9,
          "Memoria 3 frames Otimo (classica): 9 page faults");

    // Tratamento de entradas inválidas (parser).
    QString e;
    check(lerProcessos("", e).isEmpty() && !e.isEmpty(),
          "Entrada vazia: detecta erro");
    check(lerProcessos("# comentario\n\n// nada\n", e).isEmpty() && !e.isEmpty(),
          "So comentarios/branco: detecta erro");
    QList<Processo> pInval = lerProcessos(
        "pid,chegada,burst,prioridade,memoria\n1,0,0,1,100\n2,1,4,1,100\n", e);
    check(pInval.size() == 1 && pInval[0].pid == 2,
          "Burst<=0: descarta linha invalida, mantem a valida");
    QList<Processo> pTexto = lerProcessos(
        "pid,chegada,burst,prioridade,memoria\n1,texto,texto,1,100\n", e);
    check(pTexto.size() == 1 && pTexto[0].chegada == 0 && pTexto[0].burst == 1,
          "Valores nao numericos: usa padrao (chegada 0, burst 1)");
    check(lerProcessos("pid,chegada,burst,prioridade,memoria\n1,0,0,1,100\n2,1,-2,"
                       "1,50\n",
                       e)
              .isEmpty(),
          "Todos com burst<=0: nenhum processo valido");

    r += QString("\n%1 (%2 falha(s))\n")
             .arg(falhas == 0 ? "TODOS OS TESTES PASSARAM" : "HÁ FALHAS")
             .arg(falhas);
    m_relatorio = r;
    m_gantt.clear();
    emit resultadoPronto();
  }

private:
  QString m_relatorio;
  QVariantList m_gantt;

  // Exibe uma mensagem de erro na área de relatório (e limpa o Gantt).
  void mostrarErro(const QString &msg) {
    m_relatorio = "=== ERRO ===\n" + msg + "\n";
    m_gantt.clear();
    emit resultadoPronto();
  }

  // Faz o parse do conteúdo CSV em uma lista de processos. Em caso de problema,
  // preenche "erro" com a mensagem e devolve lista vazia. Separado da leitura do
  // arquivo para poder ser testado com entradas em memória.
  QList<Processo> lerProcessos(const QString &conteudo, QString &erro) {
    erro.clear();

    auto normalizeName = [](const QString &s) -> QString {
      QString x = s.toLower().trimmed();
      x.remove(' ');
      x.remove('_');
      x.remove('-');
      x.remove('.');
      // Remove acentos comuns para casar cabeçalhos como "memória".
      x.replace(QChar(0x00E1), QChar('a')); // á
      x.replace(QChar(0x00E3), QChar('a')); // ã
      x.replace(QChar(0x00E2), QChar('a')); // â
      x.replace(QChar(0x00E0), QChar('a')); // à
      x.replace(QChar(0x00E9), QChar('e')); // é
      x.replace(QChar(0x00EA), QChar('e')); // ê
      x.replace(QChar(0x00ED), QChar('i')); // í
      x.replace(QChar(0x00F3), QChar('o')); // ó
      x.replace(QChar(0x00F5), QChar('o')); // õ
      x.replace(QChar(0x00F4), QChar('o')); // ô
      x.replace(QChar(0x00FA), QChar('u')); // ú
      x.replace(QChar(0x00E7), QChar('c')); // ç
      return x;
    };

    auto splitCSVLine = [&](const QString &str) -> QStringList {
      QStringList fields;
      QString cur;
      bool inQuotes = false;
      int n = str.length();
      for (int i = 0; i < n; ++i) {
        QChar c = str[i];
        if (c == '"') {
          if (inQuotes && i + 1 < n && str[i + 1] == '"') {
            cur.append('"');
            ++i;
          } else {
            inQuotes = !inQuotes;
          }
        } else if (c == ',' && !inQuotes) {
          fields.append(cur.trimmed());
          cur.clear();
        } else {
          cur.append(c);
        }
      }
      fields.append(cur.trimmed());
      // Remove aspas ao redor do campo, se houver.
      for (QString &f : fields) {
        if (f.size() >= 2 && f.startsWith('"') && f.endsWith('"')) {
          f = f.mid(1, f.size() - 2);
        }
        f.replace("\"\"", "\"");
      }
      return fields;
    };

    // Quebra o conteúdo em linhas úteis (ignora vazias e comentários).
    QList<QStringList> rows;
    const QStringList linhas = conteudo.split('\n');
    for (const QString &line : linhas) {
      QString t = line.trimmed();
      if (t.isEmpty())
        continue;
      if (t.startsWith('#') || t.startsWith("//"))
        continue;
      rows.append(splitCSVLine(line));
    }

    if (rows.isEmpty()) {
      erro = "Arquivo CSV vazio ou sem linhas válidas (apenas linhas em branco "
             "ou comentários).";
      return {};
    }

    // Detecta cabeçalho.
    QStringList firstRow = rows.first();
    bool firstIsHeader = false;
    QString headerJoined = firstRow.join(" ").toLower();
    QStringList knownKeys = {
        "pid",    "id",      "nome", "chegada",    "arrival",  "tempo",
        "burst",  "rafaga",  "cpu",  "prioridade", "priority", "memoria",
        "memory", "tamanho", "size", "paginas",    "pages"};
    for (const QString &k : knownKeys) {
      if (headerJoined.contains(k)) {
        firstIsHeader = true;
        break;
      }
    }
    if (!firstIsHeader) {
      bool allNumeric = true;
      for (const QString &f : firstRow) {
        bool ok;
        f.toDouble(&ok);
        if (!ok) {
          allNumeric = false;
          break;
        }
      }
      if (!allNumeric)
        firstIsHeader = true;
    }

    QMap<QString, int> colIndex;
    int startRow = 0;
    if (firstIsHeader) {
      for (int i = 0; i < firstRow.size(); ++i) {
        colIndex[normalizeName(firstRow[i])] = i;
      }
      startRow = 1;
    } else {
      // Sem cabeçalho: o mapeamento depende do número de colunas. O formato do
      // enunciado tem 4 colunas (chegada, burst, prioridade, memória) e não
      // inclui PID; nesse caso o PID é atribuído sequencialmente.
      QStringList defaults;
      int ncols = firstRow.size();
      if (ncols >= 6)
        defaults = {"pid",        "chegada", "burst",
                    "prioridade", "memoria", "paginas"};
      else if (ncols == 5)
        defaults = {"pid", "chegada", "burst", "prioridade", "memoria"};
      else if (ncols == 4)
        defaults = {"chegada", "burst", "prioridade", "memoria"};
      else if (ncols == 3)
        defaults = {"chegada", "burst", "prioridade"};
      else
        defaults = {"chegada", "burst"};
      for (int i = 0; i < defaults.size(); ++i) {
        colIndex[defaults[i]] = i;
      }
      startRow = 0;
    }

    // Busca uma coluna: primeiro por nome exato; depois, para cabeçalhos
    // compostos (ex.: "Tempo de Chegada"), por substring com palavras-chave.
    auto getCol = [&](const QStringList &exatos, const QStringList &subs,
                      const QStringList &cols) -> QString {
      for (const QString &cand : exatos) {
        QString k = normalizeName(cand);
        if (colIndex.contains(k)) {
          int idx = colIndex[k];
          if (idx >= 0 && idx < cols.size())
            return cols[idx];
        }
      }
      for (const QString &cand : subs) {
        QString k = normalizeName(cand);
        for (auto it = colIndex.constBegin(); it != colIndex.constEnd(); ++it) {
          if (it.key().contains(k)) {
            int idx = it.value();
            if (idx >= 0 && idx < cols.size())
              return cols[idx];
          }
        }
      }
      return QString();
    };

    QList<Processo> processos;
    for (int r = startRow; r < rows.size(); ++r) {
      QStringList cols = rows[r];
      Processo p;
      p.raw = cols.join(",");

      QString val;
      bool ok;
      // PID: usa a coluna informada; se ausente, atribui sequencialmente.
      val = getCol(QStringList{"pid", "id", "nome"}, QStringList{"pid"}, cols);
      p.pid = val.toInt(&ok);
      if (!ok)
        p.pid = processos.size() + 1;

      val = getCol(QStringList{"chegada", "arrival"},
                   QStringList{"chegada", "arrival"}, cols);
      p.chegada = val.toInt(&ok);
      if (!ok)
        p.chegada = 0;

      val = getCol(QStringList{"burst", "rafaga", "cpu"},
                   QStringList{"execucao", "burst", "rafaga", "surto"}, cols);
      p.burst = val.toInt(&ok);
      if (!ok)
        p.burst = 1;

      val = getCol(QStringList{"prioridade", "priority", "prio"},
                   QStringList{"prioridade", "priority"}, cols);
      p.prioridade = val.toInt(&ok);
      if (!ok)
        p.prioridade = 0;

      val = getCol(QStringList{"memoria", "memory", "mem"},
                   QStringList{"memoria", "memory", "necessaria"}, cols);
      p.memoria = val.toInt(&ok);
      if (!ok)
        p.memoria = 0;

      val = getCol(QStringList{"paginas", "pages"},
                   QStringList{"pagina", "pages"}, cols);
      if (!val.isEmpty()) {
        QString s = val;
        s.replace(';', ',');
        s.replace('|', ',');
        QStringList parts = s.split(',', Qt::SkipEmptyParts);
        for (QString part : parts) {
          part = part.trimmed();
          if (part.contains('-')) {
            QStringList range = part.split('-', Qt::SkipEmptyParts);
            if (range.size() == 2) {
              bool ok1, ok2;
              int a = range[0].toInt(&ok1);
              int b = range[1].toInt(&ok2);
              if (ok1 && ok2) {
                for (int x = a; x <= b; ++x)
                  p.paginas.append(x);
              }
            }
          } else {
            bool okp;
            int page = part.toInt(&okp);
            if (okp)
              p.paginas.append(page);
          }
        }
      }

      // Validação: ignora linhas inválidas (sem tempo de CPU) e normaliza a
      // chegada para não ser negativa.
      if (p.burst < 1)
        continue;
      if (p.chegada < 0)
        p.chegada = 0;

      processos.append(p);
    }

    if (processos.isEmpty()) {
      erro = "Nenhum processo válido encontrado no arquivo.";
      return {};
    }
    return processos;
  }

  // Monta o relatório textual e o modelo da linha do tempo para a interface.
  void construirResultado(const core::Result &res, int escalonamento,
                          core::Repl repl, int memFisica, int memVirtual,
                          int quantum) {
    static const char *nomesSched[] = {"Round-Robin", "SJF Preemptivo",
                                       "Prioridade Preemptiva"};
    static const char *nomesRepl[] = {"FIFO", "LRU", "Ótimo"};

    m_gantt.clear();
    for (const core::Slice &s : res.gantt) {
      QVariantMap m;
      m["pid"] = s.pid;
      m["inicio"] = s.inicio;
      m["fim"] = s.fim;
      m["duracao"] = s.fim - s.inicio;
      m_gantt.append(m);
    }

    QString r;
    r += "=== CONFIGURAÇÃO ===\n";
    r += QString("Escalonamento: %1").arg(nomesSched[escalonamento]);
    if (escalonamento == 0)
      r += QString(" (quantum = %1)").arg(quantum < 1 ? 1 : quantum);
    r += "\n";
    r += QString("Memória física: %1 MB | virtual: %2 MB | página: %3 MB\n")
             .arg(memFisica)
             .arg(memVirtual)
             .arg(core::PAGE_SIZE_MB);
    r += QString("Frames físicos: %1 | Frames virtuais: %2 | Substituição: %3\n")
             .arg(res.numFrames)
             .arg(res.numFramesVirtual)
             .arg(nomesRepl[static_cast<int>(repl)]);
    r += QString("Páginas distintas exigidas: %1\n").arg(res.paginasDistintas);
    if (res.estouroVirtual)
      r += "ATENÇÃO: a demanda de páginas excede a memória virtual "
           "disponível.\n";
    r += "\n";

    r += "=== MÉTRICAS POR PROCESSO ===\n";
    r += "PID  Chegada  Burst  1aExec  Conclusão  Resposta  Espera\n";
    for (const core::ProcMetrics &m : res.metrics) {
      r += QString("%1%2%3%4%5%6%7\n")
               .arg(m.pid, -5)
               .arg(m.chegada, -9)
               .arg(m.burst, -7)
               .arg(m.primeiraExec, -8)
               .arg(m.conclusao, -11)
               .arg(m.resposta, -10)
               .arg(m.espera, -6);
    }

    r += "\n=== RESULTADOS ===\n";
    r += QString("Tempo médio de resposta: %1\n")
             .arg(res.tempoMedioResposta, 0, 'f', 2);
    r += QString("Tempo médio de espera:   %1\n")
             .arg(res.tempoMedioEspera, 0, 'f', 2);
    r += QString("Total de referências de página: %1\n")
             .arg(res.totalReferencias);
    r += QString("Total de Page Faults: %1\n").arg(res.pageFaults);

    r += "\n=== TABELA DE PÁGINAS (estado final dos frames) ===\n";
    for (int i = 0; i < res.numFrames; ++i) {
      r += QString("Frame %1: ").arg(i, -3);
      if (i < static_cast<int>(res.tabelaPaginas.size()))
        r += QString("P%1 página %2\n")
                 .arg(res.tabelaPaginas[i].pid)
                 .arg(res.tabelaPaginas[i].pagina);
      else
        r += "livre\n";
    }

    m_relatorio = r;
  }
};
