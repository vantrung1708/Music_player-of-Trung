#pragma once
#include <QObject>
#include <QString>
#include <memory>
#include "core/IHardwareDevice.hpp"

namespace controller {

class PlaybackController;

class HardwareController : public QObject {
  Q_OBJECT

public:
  explicit HardwareController(std::shared_ptr<core::IHardwareDevice> device,
                              PlaybackController* playback,
                              QObject* parent = nullptr);

  Q_INVOKABLE void start();
  Q_INVOKABLE void stop();
  Q_INVOKABLE void injectCommand(const QString& cmd);
  Q_INVOKABLE void displayText(const QString& line1, const QString& line2);
  Q_INVOKABLE void setSerialConfig(const QString& port, int baud);

private:
  std::shared_ptr<core::IHardwareDevice> device_;
  PlaybackController* playback_ = nullptr;
};

} // namespace controller
