#pragma once
#include <QObject>
#include <QString>
#include <memory>
#include <string>
#include "model/PlaylistManager.hpp"
#include "playback/AudioEngine.hpp"

namespace controller {

class PlaybackController : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
  Q_PROPERTY(qint64 positionMs READ positionMs NOTIFY positionChanged)
  Q_PROPERTY(qint64 durationMs READ durationMs NOTIFY durationChanged)
  Q_PROPERTY(float volume READ volume WRITE setVolume NOTIFY volumeChanged)
  Q_PROPERTY(QString currentPath READ currentPath NOTIFY currentPathChanged)
  Q_PROPERTY(QString currentTitle READ currentTitle NOTIFY currentTitleChanged)
  Q_PROPERTY(QString currentArtist READ currentArtist NOTIFY currentArtistChanged)

public:
  explicit PlaybackController(std::shared_ptr<model::PlaylistManager> playlist,
                              QObject* parent = nullptr);

  bool playing() const;
  qint64 positionMs() const;
  qint64 durationMs() const;
  float volume() const;
  QString currentPath() const;
  QString currentTitle() const;
  QString currentArtist() const;

  Q_INVOKABLE void playPause();
  Q_INVOKABLE void openAndPlay(const QString& path);
  Q_INVOKABLE void stop();
  Q_INVOKABLE void seek(qint64 posMs);
  Q_INVOKABLE void next();
  Q_INVOKABLE void shuffle();

public slots:
  void setVolume(float v01);

signals:
  void playingChanged();
  void positionChanged();
  void durationChanged();
  void volumeChanged();
  void currentPathChanged();
  void currentTitleChanged();
  void currentArtistChanged();

private:
  void bindEngineCallbacks_();
  void updateDuration_();
  void updateCurrentMeta_();

  std::shared_ptr<model::PlaylistManager> playlist_;
  playback::AudioEngine engine_;

  bool playing_ = false;
  qint64 positionMs_ = 0;
  qint64 durationMs_ = 0;
  float volume_ = 1.0f;
  QString currentPath_;
  QString currentTitle_;
  QString currentArtist_;
};

} // namespace controller
