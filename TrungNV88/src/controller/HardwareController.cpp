#include "controller/HardwareController.hpp"
#include "controller/PlaybackController.hpp"
#include "model/MockSerialTransceiver.hpp"
#include "model/SerialTransceiver.hpp"
#include <QMetaObject>
#include <algorithm>

namespace controller {

HardwareController::HardwareController(std::shared_ptr<core::IHardwareDevice> device,
                                       PlaybackController* playback,
                                       QObject* parent)
  : QObject(parent), device_(std::move(device)), playback_(playback) {
  if (device_) {
    device_->setEventCallback([this](const core::HardwareEvent& ev) {
      if (!playback_) return;
      switch (ev.type) {
        case core::HardwareEvent::Type::PlayToggle:
          QMetaObject::invokeMethod(playback_, "playPause", Qt::QueuedConnection);
          break;
        case core::HardwareEvent::Type::Next:
          QMetaObject::invokeMethod(playback_, "next", Qt::QueuedConnection);
          break;
        case core::HardwareEvent::Type::Volume: {
          const float v = std::min(1.0f, std::max(0.0f, ev.value / 100.0f));
          QMetaObject::invokeMethod(playback_, [this, v]() {
            playback_->setVolume(v);
          }, Qt::QueuedConnection);
          break;
        }
      }
    });
  }
}

void HardwareController::start() {
  if (device_) {
    device_->start();
  }
}

void HardwareController::stop() {
  if (device_) {
    device_->stop();
  }
}

void HardwareController::injectCommand(const QString& cmd) {
  auto mock = std::dynamic_pointer_cast<model::MockSerialTransceiver>(device_);
  if (mock) {
    mock->enqueueCommand(cmd.toStdString());
  }
}

void HardwareController::displayText(const QString& line1, const QString& line2) {
  if (device_) {
    device_->displayText(line1.toStdString(), line2.toStdString());
  }
}

void HardwareController::setSerialConfig(const QString& port, int baud) {
  auto serial = std::dynamic_pointer_cast<model::SerialTransceiver>(device_);
  if (!serial) {
    return;
  }
  serial->setConfig(port.toStdString(), baud);
  serial->stop();
  serial->start();
}

} // namespace controller
