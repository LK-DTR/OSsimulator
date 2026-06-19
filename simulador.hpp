#pragma once
#include "core/simulacao.hpp"
#include <QDebug>
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
    qDebug() << "--- INICIANDO SIMULAÇÃO NO C++ ---";
    qDebug() << "Arquivo CSV:" << caminhoCSV;
    qDebug() << "Memoria Fisica:" << memFisica << "MB | Virtual:" << memVirtual
             << "MB";
    qDebug() << "Escalonamento (ID):" << escalonamento
             << "| Quantum:" << quantum;
    qDebug() << "Política de Substituição (ID):" << politicaMemoria;

    if (caminhoCSV.isEmpty()) {
      qDebug() << "Nenhum arquivo CSV selecionado.";
      return;
    }

    QFile file(caminhoCSV);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      qDebug() << "Falha ao abrir arquivo:" << caminhoCSV << "->"
               << file.errorString();
      return;
    }

    QTextStream in(&file);

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
      // Remove surrounding quotes if present
      for (QString &f : fields) {
        if (f.size() >= 2 && f.startsWith('"') && f.endsWith('"')) {
          f = f.mid(1, f.size() - 2);
        }
        f.replace("\"\"", "\"");
      }
      return fields;
    };

    QList<QStringList> rows;
    while (!in.atEnd()) {
      QString line = in.readLine();
      if (line.trimmed().isEmpty())
        continue;
      QString t = line.trimmed();
      if (t.startsWith('#') || t.startsWith("//"))
        continue;
      rows.append(splitCSVLine(line));
    }
    file.close();

    if (rows.isEmpty()) {
      qDebug() << "Arquivo CSV vazio ou sem linhas válidas.";
      return;
    }

    // Detect header
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
    // compostos (ex.: "Tempo de Chegada"), por substring usando palavras-chave
    // específicas e não ambíguas.
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
      // PID: usa a coluna informada; se ausente (ex.: formato do enunciado),
      // atribui sequencialmente. Não há fallback posicional para evitar
      // confundir o PID com a coluna de chegada.
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

      processos.append(p);
    }

    qDebug() << "Processos carregados:" << processos.size();
    if (processos.isEmpty()) {
      qDebug() << "Nenhum processo válido encontrado.";
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

    construirResultado(res, escalonamento, repl, memFisica, memVirtual,
                       quantum);
    emit resultadoPronto();
  }

private:
  QString m_relatorio;
  QVariantList m_gantt;

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
    r += QString("Páginas distintas exigidas: %1\n")
             .arg(res.paginasDistintas);
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
