#include "controller/SettingsController.hpp"
#include "utils/Config.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

namespace controller {

SettingsController::SettingsController(QObject* parent)
  : QObject(parent) {}

QString SettingsController::lastLibraryPath() const {
  return lastLibraryPath_;
}

QString SettingsController::lastTrackPath() const {
  return lastTrackPath_;
}

float SettingsController::volume() const {
  return volume_;
}

QString SettingsController::serialPort() const {
  return serialPort_;
}

int SettingsController::serialBaud() const {
  return serialBaud_;
}

void SettingsController::setLastLibraryPath(const QString& path) {
  if (lastLibraryPath_ == path) {
    return;
  }
  lastLibraryPath_ = path;
  emit lastLibraryPathChanged();
}

void SettingsController::setLastTrackPath(const QString& path) {
  if (lastTrackPath_ == path) {
    return;
  }
  lastTrackPath_ = path;
  emit lastTrackPathChanged();
}

void SettingsController::setVolume(float v) {
  if (v < 0.0f) v = 0.0f;
  if (v > 1.0f) v = 1.0f;
  if (volume_ == v) {
    return;
  }
  volume_ = v;
  emit volumeChanged();
}

void SettingsController::setSerialPort(const QString& port) {
  if (serialPort_ == port) {
    return;
  }
  serialPort_ = port;
  emit serialPortChanged();
}

void SettingsController::setSerialBaud(int baud) {
  if (serialBaud_ == baud) {
    return;
  }
  serialBaud_ = baud;
  emit serialBaudChanged();
}

void SettingsController::load() {
  try {
    std::ifstream in(utils::settingsPath());
    if (!in) {
      return;
    }
    nlohmann::json j;
    in >> j;
    setLastLibraryPath(QString::fromStdString(j.value("lastLibraryPath", "")));
    setLastTrackPath(QString::fromStdString(j.value("lastTrackPath", "")));
    setVolume(j.value("volume", 1.0f));
    setSerialPort(QString::fromStdString(j.value("serialPort", "/dev/ttyUSB0")));
    setSerialBaud(j.value("serialBaud", 115200));
  } catch (...) {
    // ignore
  }
}

void SettingsController::save() const {
  try {
    nlohmann::json j;
    j["lastLibraryPath"] = lastLibraryPath_.toStdString();
    j["lastTrackPath"] = lastTrackPath_.toStdString();
    j["volume"] = volume_;
    j["serialPort"] = serialPort_.toStdString();
    j["serialBaud"] = serialBaud_;

    std::ofstream out(utils::settingsPath());
    if (!out) {
      return;
    }
    out << j.dump(2);
  } catch (...) {
    // ignore
  }
}

} // namespace controller
