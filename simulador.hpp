#pragma once
#include <QDebug>
#include <QFile>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVector>

class Simulador : public QObject {
  Q_OBJECT
public:
  explicit Simulador(QObject *parent = nullptr) : QObject(parent) {}

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
      // default mapping if there is no header
      QStringList defaults = {"pid",        "chegada", "burst",
                              "prioridade", "memoria", "paginas"};
      for (int i = 0; i < defaults.size(); ++i) {
        colIndex[defaults[i]] = i;
      }
      startRow = 0;
    }

    auto getCol = [&](const QStringList &candidates,
                      const QStringList &cols) -> QString {
      for (const QString &cand : candidates) {
        QString k = normalizeName(cand);
        if (colIndex.contains(k)) {
          int idx = colIndex[k];
          if (idx >= 0 && idx < cols.size())
            return cols[idx];
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
      val = getCol(QStringList{"pid", "id", "nome"}, cols);
      if (val.isEmpty() && cols.size() > 0)
        val = cols[0];
      bool ok;
      p.pid = val.toInt(&ok);
      if (!ok)
        p.pid = processos.size() + 1;

      val = getCol(
          QStringList{"chegada", "arrival", "tempo_chegada", "tchegada"}, cols);
      if (val.isEmpty() && cols.size() > 1)
        val = cols[1];
      p.chegada = val.toInt(&ok);
      if (!ok)
        p.chegada = 0;

      val = getCol(QStringList{"burst", "tempo", "rafaga", "cpu", "bursttime"},
                   cols);
      if (val.isEmpty() && cols.size() > 2)
        val = cols[2];
      p.burst = val.toInt(&ok);
      if (!ok)
        p.burst = 1;

      val = getCol(QStringList{"prioridade", "priority", "prio"}, cols);
      if (val.isEmpty() && cols.size() > 3)
        val = cols[3];
      p.prioridade = val.toInt(&ok);
      if (!ok)
        p.prioridade = 0;

      val = getCol(
          QStringList{"memoria", "memory", "mem", "tamanho", "size", "memreq"},
          cols);
      if (val.isEmpty() && cols.size() > 4)
        val = cols[4];
      p.memoria = val.toInt(&ok);
      if (!ok)
        p.memoria = 0;

      val = getCol(QStringList{"paginas", "pages"}, cols);
      if (val.isEmpty() && cols.size() > 5)
        val = cols[5];
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
    for (const Processo &p : processos) {
      QStringList pagStr;
      for (int pg : p.paginas)
        pagStr.append(QString::number(pg));
      qDebug() << "PID:" << p.pid << "| chegada:" << p.chegada
               << "| burst:" << p.burst << "| prioridade:" << p.prioridade
               << "| memoria:" << p.memoria << "| paginas:"
               << (pagStr.isEmpty() ? QString("[]") : pagStr.join(","));
    }

    // TODO: implementar a lógica de escalonamento e gerenciamento de memória
  }
};
