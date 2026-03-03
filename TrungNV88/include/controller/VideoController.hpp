#pragma once
#include <QObject>
#include <QString>
#include "playback/VideoEngine.hpp"

class VideoFrameProvider;

namespace controller {

class VideoController : public QObject {
  Q_OBJECT
  Q_PROPERTY(int frameCounter READ frameCounter NOTIFY frameCounterChanged)
  Q_PROPERTY(bool active READ active NOTIFY activeChanged)
  Q_PROPERTY(double playbackRate READ playbackRate WRITE setPlaybackRate NOTIFY playbackRateChanged)

public:
  explicit VideoController(VideoFrameProvider* provider, QObject* parent = nullptr);

  int frameCounter() const;
  bool active() const;
  double playbackRate() const;

  Q_INVOKABLE void openAndPlay(const QString& path);
  Q_INVOKABLE void play();
  Q_INVOKABLE void pause();
  Q_INVOKABLE void seek(qint64 posMs);
  Q_INVOKABLE void stop();
  Q_INVOKABLE void setPlaybackRate(double rate);

signals:
  void frameCounterChanged();
  void activeChanged();
  void playbackRateChanged();

private:
  void onFrame_(const playback::VideoFrame& frame);

  VideoFrameProvider* provider_ = nullptr;
  playback::VideoEngine engine_;
  int frameCounter_ = 0;
  bool active_ = false;
  double playbackRate_ = 1.0;
};

} // namespace controller
