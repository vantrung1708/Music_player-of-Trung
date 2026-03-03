#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "core/IMediaPlayer.hpp"

struct SDL_AudioDeviceID; // forward (SDL)
typedef unsigned int SDL_AudioDeviceID;

struct AVFormatContext;
struct AVCodecContext;
struct SwrContext;
struct AVPacket;
struct AVFrame;

namespace playback {

struct AudioSpec {
  int sampleRate = 48000;
  int channels   = 2;
  int formatS16  = 1;     // placeholder
};

class AudioEngine : public core::IMediaPlayer {
public:
  using StateCallback = std::function<void(bool playing)>;
  using TimeCallback  = std::function<void(int64_t posMs, int64_t durMs)>;
  using EndCallback   = std::function<void()>;

  AudioEngine();
  ~AudioEngine();

  bool init() override;
  bool open(const std::string& path) override;   // audio/video file ok, audio stream used here
  void play() override;
  void pause() override;
  void stop() override;
  bool seek(int64_t targetMs) override;

  void setVolume(float v01) override;            // 0..1
  float volume() const override;

  int64_t positionMs() const override;
  int64_t durationMs() const override;

  void onStateChanged(StateCallback cb) { stateCb_ = std::move(cb); }
  void onTime(TimeCallback cb)          { timeCb_  = std::move(cb); }
  void onEnd(EndCallback cb)            { endCb_   = std::move(cb); }

private:
  // ---- SDL callback ----
  static void sdlAudioCallback(void* userdata, uint8_t* stream, int len);
  void fillAudio(uint8_t* stream, int len);

  // ---- FFmpeg decode loop ----
  void decodeThreadMain();
  bool openFFmpeg_(const std::string& path);
  void closeFFmpeg_();
  bool initSwr_(int outRate, int outCh);

  // Ring buffer (simple; can be upgraded to lock-free)
  void pushPcm_(const uint8_t* data, size_t bytes);
  size_t popPcm_(uint8_t* out, size_t bytes);

private:
  std::atomic<bool> running_{false};
  std::atomic<bool> playing_{false};
  std::atomic<float> volume_{1.0f};

  std::thread decodeThread_;
  std::mutex decodeMtx_;

  // Buffer
  std::mutex bufMtx_;
  std::vector<uint8_t> pcmBuf_;     // linear buffer
  size_t readPos_ = 0;

  // Timing
  std::atomic<int64_t> posMs_{0};
  std::atomic<int64_t> durMs_{0};
  std::atomic<int64_t> playedBytes_{0};
  std::atomic<int64_t> lastTimeCbMs_{0};

  // SDL
  SDL_AudioDeviceID sdlDev_ = 0;
  AudioSpec outSpec_;

  // FFmpeg
  AVFormatContext* fmt_ = nullptr;
  AVCodecContext*  dec_ = nullptr;
  SwrContext*      swr_ = nullptr;
  int audioStreamIndex_ = -1;
  double timeBase_ = 0.0;

  // callbacks
  StateCallback stateCb_;
  TimeCallback  timeCb_;
  EndCallback   endCb_;
};

} // namespace playback
