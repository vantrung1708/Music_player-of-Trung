#pragma once
#include <cstdint>
#include <string>

namespace core {

class IMediaPlayer {
public:
  virtual ~IMediaPlayer() = default;
  virtual bool init() = 0;
  virtual bool open(const std::string& path) = 0;
  virtual void play() = 0;
  virtual void pause() = 0;
  virtual void stop() = 0;
  virtual bool seek(int64_t targetMs) = 0;

  virtual void setVolume(float v01) = 0;
  virtual float volume() const = 0;

  virtual int64_t positionMs() const = 0;
  virtual int64_t durationMs() const = 0;
};

} // namespace core
