/**
 ******************************************************************************
 * @file    SafeFile.cpp
 * @brief   Crash-safe whole-file saves (see the header).
 ******************************************************************************
 */

#include "SafeFile.hpp"

#include <cstdio>

namespace Streak::SafeFile
{

namespace
{
constexpr size_t kPath = SDK::Interface::IFileSystem::skMaxPathLen + 8;

bool sibling(char* out, const char* path, const char* suffix)
{
    const int n = snprintf(out, kPath, "%s%s", path, suffix);
    return n > 0 && static_cast<size_t>(n) < kPath;
}
} // namespace

bool write(SDK::Interface::IFileSystem& fs, const char* path, const char* data, size_t len)
{
    char tmp[kPath];
    char bak[kPath];
    if (!sibling(tmp, path, ".tmp") || !sibling(bak, path, ".bak")) {
        return false;
    }

    // 1) Stage: only the temp is ever truncated.
    auto file = fs.file(tmp);
    if (!file || !file->open(true, true)) {
        return false;
    }
    size_t written = 0;
    bool   ok      = file->write(data, len, written) && written == len;
    ok             = file->flush() && ok;
    file->close();
    if (!ok) {
        fs.remove(tmp);
        return false;
    }

    // 2) Publish, keeping a good copy at every point.
    if (fs.exist(path)) {
        if (fs.exist(bak)) {
            fs.remove(bak);
        }
        fs.rename(path, bak);
    }
    return fs.rename(tmp, path);
}

size_t read(SDK::Interface::IFileSystem& fs, const char* path, bool backup, char* out, size_t cap)
{
    if (cap == 0) {
        return 0;
    }
    out[0] = '\0';
    char name[kPath];
    if (!sibling(name, path, backup ? ".bak" : "")) {
        return 0;
    }
    auto file = fs.file(name);
    if (!file || !file->open(false, false)) {
        return 0;
    }
    size_t total = 0;
    size_t got   = 0;
    while (total < cap - 1 && file->read(out + total, cap - 1 - total, got) && got > 0) {
        total += got;
    }
    file->close();
    out[total] = '\0';
    return total;
}

} // namespace Streak::SafeFile
