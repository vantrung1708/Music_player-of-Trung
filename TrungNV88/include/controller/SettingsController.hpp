#pragma once
#include <QObject>
#include <QString>

namespace controller {

class SettingsController : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString lastLibraryPath READ lastLibraryPath WRITE setLastLibraryPath NOTIFY lastLibraryPathChanged)
  Q_PROPERTY(QString lastTrackPath READ lastTrackPath WRITE setLastTrackPath NOTIFY lastTrackPathChanged)
  Q_PROPERTY(float volume READ volume WRITE setVolume NOTIFY volumeChanged)
  Q_PROPERTY(QString serialPort READ serialPort WRITE setSerialPort NOTIFY serialPortChanged)
  Q_PROPERTY(int serialBaud READ serialBaud WRITE setSerialBaud NOTIFY serialBaudChanged)

public:
  explicit SettingsController(QObject* parent = nullptr);

  QString lastLibraryPath() const;
  QString lastTrackPath() const;
  float volume() const;
  QString serialPort() const;
  int serialBaud() const;

  void setLastLibraryPath(const QString& path);
  void setLastTrackPath(const QString& path);
  void setVolume(float v);
  void setSerialPort(const QString& port);
  void setSerialBaud(int baud);

  Q_INVOKABLE void load();
  Q_INVOKABLE void save() const;

signals:
  void lastLibraryPathChanged();
  void lastTrackPathChanged();
  void volumeChanged();
  void serialPortChanged();
  void serialBaudChanged();

private:
  QString lastLibraryPath_;
  QString lastTrackPath_;
  float volume_ = 1.0f;
  QString serialPort_ = "/dev/ttyUSB0";
  int serialBaud_ = 115200;
};

} // namespace controller
