#pragma once
#include <QObject>
#include <QString>
#include <QVariantList>
#include <memory>
#include "model/LibraryScanner.hpp"
#include "model/MetadataExtractor.hpp"
#include "model/PlaylistManager.hpp"

namespace controller {

class LibraryController : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantList tracks READ tracks NOTIFY tracksChanged)
  Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
  Q_PROPERTY(int trackCount READ trackCount NOTIFY tracksChanged)

public:
  explicit LibraryController(std::shared_ptr<model::PlaylistManager> playlist,
                             QObject* parent = nullptr);

  QVariantList tracks() const;
  QString filterText() const;
  int trackCount() const;

  void setFilterText(const QString& text);

  Q_INVOKABLE void scan(const QString& path);
  Q_INVOKABLE bool loadSaved();

signals:
  void tracksChanged();
  void filterTextChanged();

private:
  void applyFilter_();
  bool matchesFilter_(const QVariantMap& item) const;

  QVariantList tracks_;
  QVariantList allTracks_;
  QString filterText_;
  std::shared_ptr<model::PlaylistManager> playlist_;
  model::LibraryScanner scanner_;
  model::MetadataExtractor metadata_;
};

} // namespace controller
