/**
 ******************************************************************************
 * @file    FileProbe.cpp
 * @brief   See the header.
 ******************************************************************************
 */

#include "FileProbe.hpp"

#include <cstring>

using SDK::Interface::IFileSystem;

namespace ClockProbe
{

namespace
{
uint32_t clampSize(size_t v)
{
    return v > 0xFFFFFFFFu ? 0xFFFFFFFFu : static_cast<uint32_t>(v);
}

uint32_t clampUtc(time_t t)
{
    return t > 0 ? static_cast<uint32_t>(t) : 0u;
}
} // namespace

FileResult probeFile(IFileSystem& fs, const char* path, char* buf, size_t cap, bool readContent)
{
    FileResult r;
    if (buf != nullptr && cap > 0) {
        buf[0] = '\0';
    }
    if (path == nullptr) {
        return r;
    }
    r.exists = fs.exist(path);
    if (!r.exists) {
        return r;
    }
    IFileSystem::ObjectInfo info {};
    if (fs.objectInfo(path, info)) {
        r.infoOk = true;
        r.size   = clampSize(info.size);
        r.utc    = clampUtc(info.utc);
    }
    if (!readContent || buf == nullptr || cap < 2) {
        return r;
    }
    auto file = fs.file(path);
    if (!file || !file->open(false, false)) {
        return r;
    }
    r.opened = true;
    size_t got = 0;
    const bool ok = file->read(buf, cap - 1, got);
    file->close();
    if (!ok || got > cap - 1) {
        buf[0] = '\0';
        return r;
    }
    buf[got]  = '\0';
    r.got     = static_cast<uint16_t>(got);
    r.readOk  = true;
    if (!r.infoOk) {
        r.size = static_cast<uint32_t>(got);
    }
    r.truncated = r.infoOk && r.size > got;
    return r;
}

ListResult listDir(IFileSystem& fs, const char* path, ListVisitor visit, void* ctx, uint16_t maxEntries)
{
    ListResult r;
    if (path == nullptr) {
        return r;
    }
    r.exists = fs.exist(path);
    auto dir = fs.dir(path);
    if (!dir || !dir->open()) {
        return r;
    }
    if (!r.exists) {
        // Open said yes to a folder exist() denied: Gate 0's suspicion. Nothing is read from it.
        r.openedWithoutExist = true;
        dir->close();
        return r;
    }
    r.opened = true;
    IFileSystem::ObjectInfo item {};
    while (r.entries < maxEntries) {
        if (!dir->readNext(item)) {
            break;
        }
        if (std::strcmp(item.name, ".") == 0 || std::strcmp(item.name, "..") == 0) {
            continue;
        }
        ++r.entries;
        if (item.isDir) {
            ++r.dirs;
        } else {
            ++r.files;
        }
        if (visit != nullptr) {
            ListEntry e;
            std::strncpy(e.name, item.name, sizeof(e.name) - 1);
            e.name[sizeof(e.name) - 1] = '\0';
            e.isDir = item.isDir;
            e.size  = clampSize(item.size);
            e.utc   = clampUtc(item.utc);
            visit(ctx, e);
        }
    }
    r.full = (r.entries >= maxEntries);
    dir->close();
    return r;
}

} // namespace ClockProbe
