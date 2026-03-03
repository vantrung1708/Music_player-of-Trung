#include "playback/AudioEngine.hpp"
#if __has_include(<SDL.h>)
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#include <algorithm>
#include <chrono>
#include <cstring>
#include <thread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

namespace playback {

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine() {
  stop();
  running_ = false;
  if (decodeThread_.joinable()) {
    decodeThread_.join();
  }
  closeFFmpeg_();
  if (sdlDev_ != 0) {
    SDL_CloseAudioDevice(sdlDev_);
    sdlDev_ = 0;
  }
  if (SDL_WasInit(SDL_INIT_AUDIO) != 0) {
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
  }
}

bool AudioEngine::init() {
  if (sdlDev_ != 0) {
    return true;
  }
  if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
      return false;
    }
  }

  SDL_AudioSpec desired{};
  desired.freq = outSpec_.sampleRate;
  desired.format = AUDIO_S16SYS;
  desired.channels = static_cast<Uint8>(outSpec_.channels);
  desired.samples = 1024;
  desired.callback = &AudioEngine::sdlAudioCallback;
  desired.userdata = this;

  SDL_AudioSpec obtained{};
  sdlDev_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
  if (sdlDev_ == 0) {
    return false;
  }

  outSpec_.sampleRate = obtained.freq;
  outSpec_.channels = obtained.channels;
  outSpec_.formatS16 = 1;
  SDL_PauseAudioDevice(sdlDev_, 1);
  return true;
}

bool AudioEngine::open(const std::string& path) {
  std::lock_guard<std::mutex> lock(decodeMtx_);
  closeFFmpeg_();
  if (!openFFmpeg_(path)) {
    return false;
  }

  {
    std::lock_guard<std::mutex> bufLock(bufMtx_);
    pcmBuf_.clear();
    readPos_ = 0;
  }
  playedBytes_ = 0;
  posMs_ = 0;
  lastTimeCbMs_ = 0;

  if (!running_) {
    running_ = true;
    decodeThread_ = std::thread(&AudioEngine::decodeThreadMain, this);
  }
  return true;
}

void AudioEngine::play() {
  if (sdlDev_ == 0) {
    return;
  }
  playing_ = true;
  SDL_PauseAudioDevice(sdlDev_, 0);
  if (stateCb_) {
    stateCb_(true);
  }
}

void AudioEngine::pause() {
  if (sdlDev_ == 0) {
    return;
  }
  playing_ = false;
  SDL_PauseAudioDevice(sdlDev_, 1);
  if (stateCb_) {
    stateCb_(false);
  }
}

void AudioEngine::stop() {
  if (sdlDev_ != 0) {
    SDL_PauseAudioDevice(sdlDev_, 1);
  }
  playing_ = false;
  {
    std::lock_guard<std::mutex> bufLock(bufMtx_);
    pcmBuf_.clear();
    readPos_ = 0;
  }
  playedBytes_ = 0;
  posMs_ = 0;
  lastTimeCbMs_ = 0;
  if (stateCb_) {
    stateCb_(false);
  }
}

bool AudioEngine::seek(int64_t targetMs) {
  std::lock_guard<std::mutex> lock(decodeMtx_);
  if (!fmt_ || audioStreamIndex_ < 0 || timeBase_ <= 0.0) {
    return false;
  }
  const int64_t ts = static_cast<int64_t>(targetMs / 1000.0 / timeBase_);
  if (av_seek_frame(fmt_, audioStreamIndex_, ts, AVSEEK_FLAG_BACKWARD) < 0) {
    return false;
  }
  if (dec_) {
    avcodec_flush_buffers(dec_);
  }
  {
    std::lock_guard<std::mutex> bufLock(bufMtx_);
    pcmBuf_.clear();
    readPos_ = 0;
  }
  posMs_ = targetMs;
  lastTimeCbMs_ = targetMs;
  const int64_t bytesPerSec = static_cast<int64_t>(outSpec_.sampleRate) * outSpec_.channels * 2;
  playedBytes_ = (targetMs * bytesPerSec) / 1000;
  return true;
}

void AudioEngine::setVolume(float v01) {
  if (v01 < 0.0f) v01 = 0.0f;
  if (v01 > 1.0f) v01 = 1.0f;
  volume_ = v01;
}

float AudioEngine::volume() const {
  return volume_.load();
}

int64_t AudioEngine::positionMs() const {
  return posMs_.load();
}

int64_t AudioEngine::durationMs() const {
  return durMs_.load();
}

void AudioEngine::sdlAudioCallback(void* userdata, uint8_t* stream, int len) {
  auto* self = static_cast<AudioEngine*>(userdata);
  if (self) {
    self->fillAudio(stream, len);
  }
}

void AudioEngine::fillAudio(uint8_t* stream, int len) {
  std::memset(stream, 0, static_cast<size_t>(len));
  if (!playing_ || !running_) {
    return;
  }

  const size_t got = popPcm_(stream, static_cast<size_t>(len));
  if (got == 0) {
    return;
  }

  const float v = volume_.load();
  if (v < 0.999f) {
    auto* samples = reinterpret_cast<int16_t*>(stream);
    const size_t count = got / sizeof(int16_t);
    for (size_t i = 0; i < count; ++i) {
      int sample = static_cast<int>(samples[i] * v);
      sample = std::min(32767, std::max(-32768, sample));
      samples[i] = static_cast<int16_t>(sample);
    }
  }

  const int64_t bytesPerSec = static_cast<int64_t>(outSpec_.sampleRate) * outSpec_.channels * 2;
  const int64_t newPlayedBytes = playedBytes_.fetch_add(static_cast<int64_t>(got)) + static_cast<int64_t>(got);
  const int64_t newPosMs = (newPlayedBytes * 1000) / bytesPerSec;
  posMs_ = newPosMs;

  if (timeCb_) {
    const int64_t last = lastTimeCbMs_.load();
    if (newPosMs - last >= 200 || (durMs_.load() > 0 && newPosMs >= durMs_.load())) {
      lastTimeCbMs_ = newPosMs;
      timeCb_(newPosMs, durMs_.load());
    }
  }
}

void AudioEngine::decodeThreadMain() {
  AVPacket* pkt = av_packet_alloc();
  AVFrame* frame = av_frame_alloc();

  const size_t maxBufferBytes = static_cast<size_t>(outSpec_.sampleRate) * outSpec_.channels * 2 * 5;

  while (running_) {
    if (!playing_) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    bool bufferFull = false;
    {
      std::lock_guard<std::mutex> bufLock(bufMtx_);
      const size_t available = (pcmBuf_.size() >= readPos_) ? (pcmBuf_.size() - readPos_) : 0;
      bufferFull = available > maxBufferBytes;
    }
    if (bufferFull) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    std::unique_lock<std::mutex> lock(decodeMtx_);
    if (!fmt_ || !dec_) {
      lock.unlock();
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    const int ret = av_read_frame(fmt_, pkt);
    if (ret == AVERROR_EOF) {
      lock.unlock();
      playing_ = false;
      if (stateCb_) {
        stateCb_(false);
      }
      if (endCb_) {
        endCb_();
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
      continue;
    }
    if (ret < 0) {
      lock.unlock();
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      continue;
    }

    if (pkt->stream_index != audioStreamIndex_) {
      av_packet_unref(pkt);
      lock.unlock();
      continue;
    }

    int sendRet = avcodec_send_packet(dec_, pkt);
    av_packet_unref(pkt);
    if (sendRet < 0) {
      lock.unlock();
      continue;
    }

    while (sendRet >= 0) {
      int recvRet = avcodec_receive_frame(dec_, frame);
      if (recvRet == AVERROR(EAGAIN) || recvRet == AVERROR_EOF) {
        break;
      }
      if (recvRet < 0) {
        break;
      }

      const int outSamples = swr_get_out_samples(swr_, frame->nb_samples);
      const int outChannels = outSpec_.channels;
      const int outBufSize = av_samples_get_buffer_size(nullptr, outChannels, outSamples,
                                                        AV_SAMPLE_FMT_S16, 0);
      if (outBufSize <= 0) {
        continue;
      }

      std::vector<uint8_t> out(static_cast<size_t>(outBufSize));
      uint8_t* outPlanes[1] = { out.data() };
      const int converted = swr_convert(swr_, outPlanes, outSamples,
                                        const_cast<const uint8_t**>(frame->data), frame->nb_samples);
      if (converted > 0) {
        const int bytes = av_samples_get_buffer_size(nullptr, outChannels, converted,
                                                     AV_SAMPLE_FMT_S16, 1);
        if (bytes > 0) {
          pushPcm_(out.data(), static_cast<size_t>(bytes));
        }
      }
      av_frame_unref(frame);
    }
    lock.unlock();
  }

  av_frame_free(&frame);
  av_packet_free(&pkt);
}

bool AudioEngine::openFFmpeg_(const std::string& path) {
  auto fail = [this]() {
    closeFFmpeg_();
    return false;
  };

  if (avformat_open_input(&fmt_, path.c_str(), nullptr, nullptr) < 0) {
    return fail();
  }
  if (avformat_find_stream_info(fmt_, nullptr) < 0) {
    return fail();
  }

  audioStreamIndex_ = -1;
  for (unsigned int i = 0; i < fmt_->nb_streams; ++i) {
    if (fmt_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      audioStreamIndex_ = static_cast<int>(i);
      break;
    }
  }
  if (audioStreamIndex_ < 0) {
    return fail();
  }

  AVStream* stream = fmt_->streams[audioStreamIndex_];
  const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
  if (!codec) {
    return fail();
  }

  dec_ = avcodec_alloc_context3(codec);
  if (!dec_) {
    return fail();
  }
  if (avcodec_parameters_to_context(dec_, stream->codecpar) < 0) {
    return fail();
  }
  if (avcodec_open2(dec_, codec, nullptr) < 0) {
    return fail();
  }

  timeBase_ = av_q2d(stream->time_base);
  if (stream->duration != AV_NOPTS_VALUE && timeBase_ > 0.0) {
    durMs_ = static_cast<int64_t>(stream->duration * timeBase_ * 1000.0);
  } else if (fmt_->duration != AV_NOPTS_VALUE) {
    durMs_ = static_cast<int64_t>(fmt_->duration / (AV_TIME_BASE / 1000.0));
  } else {
    durMs_ = 0;
  }

  if (!initSwr_(outSpec_.sampleRate, outSpec_.channels)) {
    return fail();
  }

  return true;
}

void AudioEngine::closeFFmpeg_() {
  if (dec_) {
    avcodec_free_context(&dec_);
    dec_ = nullptr;
  }
  if (fmt_) {
    avformat_close_input(&fmt_);
    fmt_ = nullptr;
  }
  if (swr_) {
    swr_free(&swr_);
    swr_ = nullptr;
  }
  audioStreamIndex_ = -1;
  timeBase_ = 0.0;
  durMs_ = 0;
}

bool AudioEngine::initSwr_(int outRate, int outCh) {
  if (!dec_) {
    return false;
  }
  if (swr_) {
    swr_free(&swr_);
  }

  const int64_t inLayout = dec_->channel_layout ? dec_->channel_layout
                                                : av_get_default_channel_layout(dec_->channels);
  const int64_t outLayout = (outCh == 1) ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;

  swr_ = swr_alloc_set_opts(nullptr,
                            outLayout, AV_SAMPLE_FMT_S16, outRate,
                            inLayout, dec_->sample_fmt, dec_->sample_rate,
                            0, nullptr);
  if (!swr_) {
    return false;
  }
  return swr_init(swr_) >= 0;
}

void AudioEngine::pushPcm_(const uint8_t* data, size_t bytes) {
  if (!data || bytes == 0) return;
  std::lock_guard<std::mutex> lock(bufMtx_);

  // Compact buffer if readPos_ is large.
  if (readPos_ > 0 && readPos_ >= pcmBuf_.size() / 2) {
    pcmBuf_.erase(pcmBuf_.begin(), pcmBuf_.begin() + static_cast<std::ptrdiff_t>(readPos_));
    readPos_ = 0;
  }

  const size_t oldSize = pcmBuf_.size();
  pcmBuf_.resize(oldSize + bytes);
  std::memcpy(pcmBuf_.data() + oldSize, data, bytes);
}

size_t AudioEngine::popPcm_(uint8_t* out, size_t bytes) {
  std::lock_guard<std::mutex> lock(bufMtx_);
  const size_t available = (pcmBuf_.size() >= readPos_) ? (pcmBuf_.size() - readPos_) : 0;
  const size_t toCopy = std::min(bytes, available);
  if (toCopy == 0) {
    return 0;
  }
  std::memcpy(out, pcmBuf_.data() + readPos_, toCopy);
  readPos_ += toCopy;
  return toCopy;
}

} // namespace playback
