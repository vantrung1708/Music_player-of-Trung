#include "model/MockSerialTransceiver.hpp"
#include <chrono>
#include <cctype>

namespace model {

MockSerialTransceiver::MockSerialTransceiver() = default;

MockSerialTransceiver::~MockSerialTransceiver() {
  stop();
}

bool MockSerialTransceiver::start() {
  if (running_) {
    return true;
  }
  running_ = true;
  worker_ = std::thread(&MockSerialTransceiver::workerMain_, this);
  return true;
}

void MockSerialTransceiver::stop() {
  running_ = false;
  cv_.notify_all();
  if (worker_.joinable()) {
    worker_.join();
  }
}

void MockSerialTransceiver::setEventCallback(EventCallback cb) {
  cb_ = std::move(cb);
}

void MockSerialTransceiver::displayText(const std::string& line1, const std::string& line2) {
  (void)line1;
  (void)line2;
}

void MockSerialTransceiver::enqueueCommand(const std::string& cmd) {
  {
    std::lock_guard<std::mutex> lock(mtx_);
    queue_.push(cmd);
  }
  cv_.notify_one();
}

void MockSerialTransceiver::workerMain_() {
  while (running_) {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [this]() { return !queue_.empty() || !running_; });
    if (!running_) {
      break;
    }
    auto cmd = queue_.front();
    queue_.pop();
    lock.unlock();

    handleCommand_(cmd);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

static int parseInt_(const std::string& s) {
  try {
    return std::stoi(s);
  } catch (...) {
    return 0;
  }
}

void MockSerialTransceiver::handleCommand_(const std::string& cmd) {
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

} // namespace model
