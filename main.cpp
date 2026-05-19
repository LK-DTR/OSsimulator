#include "simulador.hpp"
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

int main(int argc, char *argv[]) {
  QQuickStyle::setStyle("FluentWinUI3");
  QGuiApplication app(argc, argv);

  Simulador backend;

  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("backend", &backend);

  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      [](const QUrl &url) {
        qDebug() << "\nERRO!" << url;
        QCoreApplication::exit(-1);
      },
      Qt::QueuedConnection);

  const QUrl url(QStringLiteral("qrc:/qt/qml/SimuladorQt/Main.qml"));
  engine.load(url);

  return app.exec();
}
