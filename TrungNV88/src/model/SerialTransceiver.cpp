#include "model/SerialTransceiver.hpp"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <mutex>
#include <thread>
#include <vector>

#if defined(S32_HAVE_SERIALPORT)
#include <libserialport.h>
#endif

namespace model {

SerialTransceiver::SerialTransceiver() = default;

SerialTransceiver::~SerialTransceiver() {
  stop();
}

void SerialTransceiver::setConfig(const std::string& portName, int baud) {
  std::lock_guard<std::mutex> lock(cfgMtx_);
  portName_ = portName;
  baud_ = baud;
}

bool SerialTransceiver::start() {
#if !defined(S32_HAVE_SERIALPORT)
  return false;
#else
  if (running_) {
    return true;
  }
  running_ = true;
  reader_ = std::thread(&SerialTransceiver::readerLoop_, this);
  return true;
#endif
}

void SerialTransceiver::stop() {
  running_ = false;
  if (reader_.joinable()) {
    reader_.join();
  }
  closePort_();
}

void SerialTransceiver::setEventCallback(EventCallback cb) {
  cb_ = std::move(cb);
}

void SerialTransceiver::displayText(const std::string& line1, const std::string& line2) {
#if defined(S32_HAVE_SERIALPORT)
  std::string safe1 = line1;
  std::string safe2 = line2;
  safe1.erase(std::remove(safe1.begin(), safe1.end(), '\n'), safe1.end());
  safe1.erase(std::remove(safe1.begin(), safe1.end(), '\r'), safe1.end());
  safe2.erase(std::remove(safe2.begin(), safe2.end(), '\n'), safe2.end());
  safe2.erase(std::remove(safe2.begin(), safe2.end(), '\r'), safe2.end());

  const std::string msg = "DISP:" + safe1 + "|" + safe2 + "\n";
  std::lock_guard<std::mutex> lock(portMtx_);
  if (port_) {
    sp_blocking_write(port_, msg.data(), static_cast<int>(msg.size()), 200);
  }
#else
  (void)line1;
  (void)line2;
#endif
}

static int parseInt_(const std::string& s) {
  try {
    return std::stoi(s);
  } catch (...) {
    return 0;
  }
}

void SerialTransceiver::handleCommand_(const std::string& cmd) {
  if (!cb_) {
    return;
  }
  if (cmd == "BTN_PLAY") {
    cb_({ core::HardwareEvent::Type::PlayToggle, 0 });
    return;
  }
  if (cmd == "BTN_NEXT") {
    cb_({ core::HardwareEvent::Type::Next, 0 });
    return;
  }
  const std::string prefix = "VOL:";
  if (cmd.rfind(prefix, 0) == 0) {
    const int value = parseInt_(cmd.substr(prefix.size()));
    cb_({ core::HardwareEvent::Type::Volume, value });
  }
}

bool SerialTransceiver::openPort_() {
#if defined(S32_HAVE_SERIALPORT)
  std::string portName;
  int baud = 115200;
  {
    std::lock_guard<std::mutex> lock(cfgMtx_);
    portName = portName_;
    baud = baud_;
  }
  if (portName.empty()) {
    return false;
  }

  sp_port* port = nullptr;
  if (sp_get_port_by_name(portName.c_str(), &port) != SP_OK || !port) {
    return false;
  }
  if (sp_open(port, SP_MODE_READ_WRITE) != SP_OK) {
    sp_free_port(port);
    return false;
  }

  sp_set_baudrate(port, baud);
  sp_set_bits(port, 8);
  sp_set_parity(port, SP_PARITY_NONE);
  sp_set_stopbits(port, 1);
  sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE);

  {
    std::lock_guard<std::mutex> lock(portMtx_);
    port_ = port;
  }
  return true;
#else
  return false;
#endif
}

void SerialTransceiver::closePort_() {
#if defined(S32_HAVE_SERIALPORT)
  std::lock_guard<std::mutex> lock(portMtx_);
  if (port_) {
    sp_close(port_);
    sp_free_port(port_);
    port_ = nullptr;
  }
#endif
}

void SerialTransceiver::readerLoop_() {
#if defined(S32_HAVE_SERIALPORT)
  std::vector<char> buf(256);
  while (running_) {
    sp_port* port = nullptr;
    {
      std::lock_guard<std::mutex> lock(portMtx_);
      port = port_;
    }

    if (!port) {
      if (!openPort_()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        continue;
      }
      {
        std::lock_guard<std::mutex> lock(portMtx_);
        port = port_;
      }
    }

    if (!port) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      continue;
    }

    int n = sp_blocking_read(port, buf.data(), static_cast<int>(buf.size()), 100);
    if (n < 0) {
      closePort_();
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      continue;
    }
    if (n == 0) {
      continue;
    }

    rxBuffer_.append(buf.data(), static_cast<size_t>(n));

    size_t pos = 0;
    while ((pos = rxBuffer_.find_first_of("\r\n")) != std::string::npos) {
      std::string line = rxBuffer_.substr(0, pos);
      const size_t next = rxBuffer_.find_first_not_of("\r\n", pos);
      if (next == std::string::npos) {
        rxBuffer_.clear();
      } else {
        rxBuffer_.erase(0, next);
      }
      const auto lastNonWs = line.find_last_not_of(" \t");
      if (lastNonWs == std::string::npos) {
        line.clear();
      } else {
        line.erase(lastNonWs + 1);
      }
      if (!line.empty()) {
        handleCommand_(line);
      }
    }
  }
#endif
}

} // namespace model
