#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

#include "controller/HardwareController.hpp"
#include "controller/LibraryController.hpp"
#include "controller/PlaybackController.hpp"
#include "controller/SettingsController.hpp"
#include "controller/VideoController.hpp"
#include "model/MockSerialTransceiver.hpp"
#include "model/PlaylistManager.hpp"
#include "model/SerialTransceiver.hpp"
#include "view/VideoFrameProvider.hpp"

int main(int argc, char* argv[]) {
  QGuiApplication app(argc, argv);
  QQmlApplicationEngine engine;

  auto playlist = std::make_shared<model::PlaylistManager>();
  auto* playback = new controller::PlaybackController(playlist, &app);
  auto* library = new controller::LibraryController(playlist, &app);
  auto* settings = new controller::SettingsController(&app);
  settings->load();
  playback->setVolume(settings->volume());

  std::shared_ptr<core::IHardwareDevice> device;
#ifdef S32_ENABLE_HARDWARE
  auto serial = std::make_shared<model::SerialTransceiver>();
  serial->setConfig(settings->serialPort().toStdString(), settings->serialBaud());
  device = serial;
#else
  device = std::make_shared<model::MockSerialTransceiver>();
#endif

  auto* hardware = new controller::HardwareController(device, playback, &app);
  hardware->start();

  auto* provider = new VideoFrameProvider();
  engine.addImageProvider("video", provider);
  auto* video = new controller::VideoController(provider, &app);

  engine.rootContext()->setContextProperty("playbackController", playback);
  engine.rootContext()->setContextProperty("libraryController", library);
  engine.rootContext()->setContextProperty("settingsController", settings);
  engine.rootContext()->setContextProperty("hardwareController", hardware);
  engine.rootContext()->setContextProperty("videoController", video);

  const QUrl url(QStringLiteral("qrc:/main.qml"));
  QObject::connect(
    &engine,
    &QQmlApplicationEngine::objectCreated,
    &app,
    [url](QObject* obj, const QUrl& objUrl) {
      if (!obj && url == objUrl) {
        QCoreApplication::exit(-1);
      }
    },
    Qt::QueuedConnection);

  engine.load(url);
  return app.exec();
}
