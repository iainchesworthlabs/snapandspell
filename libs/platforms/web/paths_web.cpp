// SPDX-License-Identifier: GPL-3.0-or-later
#include <QByteArray>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <filesystem>

#include "sns/platform/paths.hpp"

namespace sns::platform {

namespace {

// The resource tree the data was compiled into (sns_platform_configure_app in
// platform.cmake, SNS_WEB_DATA_RESOURCE_PREFIX): dictionaries/en.txt and dice/3x3.txt to
// 6x6.txt, the files a desktop build copies beside its executable.
constexpr auto resource_root = ":/sns-web-data";

// Where the data is laid down: a directory of the in-memory file system, which is empty at every
// page load and so needs no check that a file is already the same. Not under the persistent
// directory: the program's own files are not what a person keeps, and they would otherwise be
// written to IndexedDB at every sync.
constexpr auto data_directory = "/sns-program-data";

// The name the "executable" has in that directory. Only its directory is ever used: the program
// looks for `dictionaries/` and `dice/` beside it.
constexpr auto executable_name = "SnapAndSpell";

// The directory the boot script (packaging/sns-boot.js) mounts as IndexedDB before main() runs,
// and the program's per-user data lives below it: sns-boot.js sets XDG_DATA_HOME to it, which is
// where Qt's AppLocalDataLocation (the store's directory, apps/app/src/main.cpp) comes from.
// tests/platform/check_web.cmake holds this, platform.cmake and sns-boot.js to one name.
constexpr auto persistent_directory = "/persist";

// Copies the resources to the in-memory file system and says where they are. The dictionary
// loader opens a file by path, and nothing in a resource tree is one. If a file cannot be
// written the game's setup says which one it could not open.
[[nodiscard]] std::filesystem::path unpack_data() {
    const QDir root{QString::fromLatin1(data_directory)};
    const QString resource_prefix = QString::fromLatin1(resource_root);

    QDirIterator packed_files{resource_prefix, QDir::Files, QDirIterator::Subdirectories};
    while (packed_files.hasNext()) {
        const QString packed_path = packed_files.next();
        const QString target = root.filePath(QDir{resource_prefix}.relativeFilePath(packed_path));
        root.mkpath(QFileInfo{target}.path());
        QFile::remove(target);
        QFile{packed_path}.copy(target);
    }
    return std::filesystem::path{root.filePath(QString::fromLatin1(executable_name)).toStdString()};
}

}  // namespace

std::filesystem::path executable_path() {
    // There is no executable to sit beside: the program is a .wasm the browser fetched. The
    // answer is where the program's data was unpacked, as if the executable were there: the
    // one use of this function is to find that data.
    static const std::filesystem::path path = unpack_data();
    return path;
}

std::filesystem::path user_data_directory() {
    // The person's own place: the persistent directory if the page mounted it, and the same
    // path in memory if it could not (IndexedDB is refused in some private windows), where the
    // game still works and keeps nothing past the page.
    return std::filesystem::path{persistent_directory} / "sns";
}

}  // namespace sns::platform
