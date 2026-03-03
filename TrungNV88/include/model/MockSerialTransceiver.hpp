#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include "core/IHardwareDevice.hpp"

namespace model {

// Simple mock device that consumes queued commands on a worker thread.
class MockSerialTransceiver : public core::IHardwareDevice {
public:
  MockSerialTransceiver();
  ~MockSerialTransceiver() override;

  bool start() override;
  void stop() override;
  void setEventCallback(EventCallback cb) override;
  void displayText(const std::string& line1, const std::string& line2) override;

  // Inject a raw command (e.g. "BTN_PLAY", "BTN_NEXT", "VOL:42").
  void enqueueCommand(const std::string& cmd);

private:
  void workerMain_();
  void handleCommand_(const std::string& cmd);

  std::atomic<bool> running_{false};
  EventCallback cb_;

  std::mutex mtx_;
  std::condition_variable cv_;
  std::queue<std::string> queue_;
  std::thread worker_;
};

} // namespace model
