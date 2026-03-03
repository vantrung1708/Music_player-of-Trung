#pragma once
#include <functional>
#include <string>

namespace core {

struct HardwareEvent {
  enum class Type { PlayToggle, Next, Volume };
  Type type = Type::PlayToggle;
  int value = 0; // used for Volume
};

class IHardwareDevice {
public:
  using EventCallback = std::function<void(const HardwareEvent&)>;
  virtual ~IHardwareDevice() = default;

  virtual bool start() = 0;
  virtual void stop() = 0;
  virtual void setEventCallback(EventCallback cb) = 0;
  virtual void displayText(const std::string& line1, const std::string& line2) = 0;
};

} // namespace core
