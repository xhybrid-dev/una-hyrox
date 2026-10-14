/**
 ******************************************************************************
 * @file    LogStore.cpp
 * @brief   The weigh-in log on the watch (see the header).
 ******************************************************************************
 */

#include "LogStore.hpp"

#include <cstdio>
#include <cstring>

namespace XWeighIn::LogStore
{

namespace
{

using SDK::Interface::IFileSystem;

constexpr size_t kPath  = IFileSystem::skMaxPathLen + 8;
constexpr size_t kChunk = 256;

bool writeAll(SDK::Interface::IFile& file, const char* data, size_t len)
{
    size_t written = 0;
    return file.write(data, len, written) && written == len;
}

/// Size of @p path in bytes, or false if it is not a file.
bool fileSize(IFileSystem& fs, const char* path, size_t& size)
{
    IFileSystem::ObjectInfo info {};
    if (!fs.objectInfo(path, info) || info.isDir) {
        return false;
    }
    size = info.size;
    return true;
}

/// The file's last byte, read through a read-only handle (a write handle may
/// not be readable on the watch).
bool lastByte(IFileSystem& fs, const char* path, size_t size, char& out)
{
    auto file = fs.file(path);
    if (!file || !file->open(false, false)) {
        return false;
    }
    size_t got = 0;
    const bool ok = file->seek(size - 1) && file->read(&out, 1, got) && got == 1;
    file->close();
    return ok;
}

bool isHeader(const char* line, size_t len)
{
    if (len > 0 && line[len - 1] == '\r') {
        --len;
    }
    const size_t n = std::strlen(kLogHeader);
    return len == n && std::memcmp(line, kLogHeader, n) == 0;
}

}  // namespace

bool append(IFileSystem& fs, const char* path, const Entry& e)
{
    // The line, with room for a leading newline and the trailing one.
    char line[kMaxLineBytes + 2];
    const size_t len = formatEntry(e, line + 1, sizeof(line) - 2);
    if (len == 0) {
        return false;
    }
    line[1 + len] = '\n';

    size_t size = 0;
    const bool exists = fileSize(fs, path, size);
    bool needsHeader  = !exists || size == 0;
    bool needsNewline = false;
    if (!needsHeader) {
        char last = '\n';
        if (!lastByte(fs, path, size, last)) {
            return false;
        }
        needsNewline = last != '\n';
    }

    // Open for writing without truncating (open-or-create), then append at the end.
    auto file = fs.file(path);
    if (!file || !file->open(true, false)) {
        return false;
    }
    bool ok = file->seek(needsHeader ? 0 : size);
    if (ok && needsHeader) {
        ok = writeAll(*file, kLogHeader, std::strlen(kLogHeader)) && writeAll(*file, "\n", 1);
    }
    if (ok) {
        const char*  start = needsNewline ? line : line + 1;
        const size_t count = len + 1 + (needsNewline ? 1 : 0);
        if (needsNewline) {
            line[0] = '\n';
        }
        ok = writeAll(*file, start, count);
    }
    ok = file->flush() && ok;
    file->close();
    return ok;
}

bool appendBoth(IFileSystem& fs, const Entry& e)
{
    const bool own = append(fs, kLogFile, e);
    if (own && fs.mkdir(kSharedDir)) {
        append(fs, kSharedFile, e);
    }
    return own;
}

ScanResult scan(IFileSystem& fs, const char* path, EntryVisitor& visitor)
{
    ScanResult result;
    auto file = fs.file(path);
    if (!file || !file->open(false, false)) {
        return result;
    }
    result.opened = true;

    char   chunk[kChunk];
    char   line[kMaxLineBytes];
    size_t lineLen  = 0;
    bool   overlong = false;

    auto finishLine = [&]() {
        if (overlong) {
            ++result.rejected;
        } else if (lineLen > 0 && !(lineLen == 1 && line[0] == '\r') && !isHeader(line, lineLen)) {
            Entry e;
            if (parseLine(line, lineLen, e)) {
                ++result.entries;
                visitor.onEntry(e);
            } else {
                ++result.rejected;
            }
        }
        lineLen  = 0;
        overlong = false;
    };

    size_t got = 0;
    while (file->read(chunk, sizeof(chunk), got) && got > 0) {
        for (size_t i = 0; i < got; ++i) {
            const char c = chunk[i];
            if (c == '\n') {
                finishLine();
            } else if (lineLen < sizeof(line) - 1) {
                line[lineLen++] = c;
            } else {
                overlong = true;
            }
        }
    }
    finishLine();   // a last line with no newline: a hand edit, or a torn write
    file->close();
    return result;
}

bool copyFile(IFileSystem& fs, const char* from, const char* to)
{
    char tmp[kPath];
    const int n = std::snprintf(tmp, sizeof(tmp), "%s.tmp", to);
    if (n <= 0 || static_cast<size_t>(n) >= sizeof(tmp)) {
        return false;
    }

    auto in = fs.file(from);
    if (!in || !in->open(false, false)) {
        return false;
    }
    auto out = fs.file(tmp);
    if (!out || !out->open(true, true)) {
        in->close();
        return false;
    }
    char   chunk[kChunk];
    size_t got = 0;
    bool   ok  = true;
    while (ok && in->read(chunk, sizeof(chunk), got) && got > 0) {
        ok = writeAll(*out, chunk, got);
    }
    ok = out->flush() && ok;
    out->close();
    in->close();
    if (!ok) {
        fs.remove(tmp);
        return false;
    }
    if (fs.exist(to) && !fs.remove(to)) {
        fs.remove(tmp);
        return false;
    }
    return fs.rename(tmp, to);
}

Reconcile reconcile(IFileSystem& fs, const char* own, const char* shared, const char* sharedDir)
{
    size_t ownSize = 0;
    size_t sharedSize = 0;
    const bool haveOwn    = fileSize(fs, own, ownSize);
    const bool haveShared = fileSize(fs, shared, sharedSize);

    if (!haveOwn) {
        if (!haveShared) {
            return Reconcile::Nothing;
        }
        return copyFile(fs, shared, own) ? Reconcile::Restored : Reconcile::Failed;
    }
    if (haveShared && sharedSize == ownSize) {
        return Reconcile::Nothing;
    }
    if (!fs.mkdir(sharedDir)) {
        return Reconcile::Failed;
    }
    return copyFile(fs, own, shared) ? Reconcile::Refreshed : Reconcile::Failed;
}

}  // namespace XWeighIn::LogStore
