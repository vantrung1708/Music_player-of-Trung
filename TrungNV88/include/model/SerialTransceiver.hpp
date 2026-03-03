#pragma once
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include "core/IHardwareDevice.hpp"

#if defined(S32_HAVE_SERIALPORT)
struct sp_port;
#endif

namespace model {

// Real UART implementation (Phase 2)
class SerialTransceiver : public core::IHardwareDevice {
public:
  SerialTransceiver();
  ~SerialTransceiver() override;

  void setConfig(const std::string& portName, int baud);

  bool start() override;
  void stop() override;
  void setEventCallback(EventCallback cb) override;
  void displayText(const std::string& line1, const std::string& line2) override;

private:
  void readerLoop_();
  void handleCommand_(const std::string& cmd);
  bool openPort_();
  void closePort_();

  std::atomic<bool> running_{false};
  EventCallback cb_;

  std::mutex cfgMtx_;
  std::string portName_;
  int baud_ = 115200;

  std::mutex portMtx_;
#if defined(S32_HAVE_SERIALPORT)
  sp_port* port_ = nullptr;
#endif
  std::thread reader_;
  std::string rxBuffer_;
};

} // namespace model
