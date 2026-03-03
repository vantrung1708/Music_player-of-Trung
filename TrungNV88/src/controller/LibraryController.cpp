#include "controller/LibraryController.hpp"
#include "utils/Config.hpp"
#include <QFileInfo>
#include <QVariantMap>

namespace controller {

LibraryController::LibraryController(std::shared_ptr<model::PlaylistManager> playlist,
                                     QObject* parent)
  : QObject(parent), playlist_(std::move(playlist)) {}

QVariantList LibraryController::tracks() const {
  return tracks_;
}

QString LibraryController::filterText() const {
  return filterText_;
}

int LibraryController::trackCount() const {
  return tracks_.size();
}

void LibraryController::setFilterText(const QString& text) {
  if (filterText_ == text) {
    return;
  }
  filterText_ = text;
  applyFilter_();
  emit filterTextChanged();
}

void LibraryController::scan(const QString& path) {
  const std::filesystem::path root = path.toStdString();
  auto scanned = scanner_.scan(root);

  QVariantList list;
  list.reserve(static_cast<int>(scanned.size()));
  for (auto& t : scanned) {
    t.meta = metadata_.extract(t.filePath);
    t.duration = metadata_.durationMs(t.filePath);

    const QString display = !t.meta.title.empty()
      ? QString::fromStdString(t.meta.title)
      : QFileInfo(QString::fromStdString(t.filePath)).fileName();

    QVariantMap item;
    item["path"] = QString::fromStdString(t.filePath);
    item["title"] = QString::fromStdString(t.meta.title);
    item["artist"] = QString::fromStdString(t.meta.artist);
    item["album"] = QString::fromStdString(t.meta.album);
    item["display"] = display;
    item["durationMs"] = static_cast<qlonglong>(t.duration);
    list.push_back(item);
  }

  allTracks_ = list;
  if (playlist_) {
    playlist_->setTracks(std::move(scanned));
    playlist_->save(utils::playlistPath());
  }

  applyFilter_();
  emit tracksChanged();
}

bool LibraryController::loadSaved() {
  if (!playlist_) {
    return false;
  }
  if (!playlist_->load(utils::playlistPath())) {
    return false;
  }

  QVariantList list;
  const auto& tracks = playlist_->tracks();
  list.reserve(static_cast<int>(tracks.size()));
  for (const auto& t : tracks) {
    const QString display = !t.meta.title.empty()
      ? QString::fromStdString(t.meta.title)
      : QFileInfo(QString::fromStdString(t.filePath)).fileName();

    QVariantMap item;
    item["path"] = QString::fromStdString(t.filePath);
    item["title"] = QString::fromStdString(t.meta.title);
    item["artist"] = QString::fromStdString(t.meta.artist);
    item["album"] = QString::fromStdString(t.meta.album);
    item["display"] = display;
    item["durationMs"] = static_cast<qlonglong>(t.duration);
    list.push_back(item);
  }

  allTracks_ = list;
  applyFilter_();
  emit tracksChanged();
  return true;
}

static bool containsCI_(const QString& haystack, const QString& needle) {
  if (needle.isEmpty()) {
    return true;
  }
  return haystack.toLower().contains(needle.toLower());
}

bool LibraryController::matchesFilter_(const QVariantMap& item) const {
  if (filterText_.trimmed().isEmpty()) {
    return true;
  }
  const QString needle = filterText_.trimmed();
  const QString title = item.value("display").toString();
  const QString artist = item.value("artist").toString();
  const QString album = item.value("album").toString();
  const QString path = item.value("path").toString();

  return containsCI_(title, needle) || containsCI_(artist, needle) ||
         containsCI_(album, needle) || containsCI_(path, needle);
}

void LibraryController::applyFilter_() {
  if (filterText_.trimmed().isEmpty()) {
    tracks_ = allTracks_;
    return;
  }

  QVariantList filtered;
  for (const auto& v : allTracks_) {
    const auto item = v.toMap();
    if (matchesFilter_(item)) {
      filtered.push_back(item);
    }
  }
  tracks_ = filtered;
}

} // namespace controller
