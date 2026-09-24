/**
 ******************************************************************************
 * @file    TreeFileSystem.cpp
 * @brief   An in-memory directory tree behind IFileSystem (see the header).
 ******************************************************************************
 */

#include "TreeFileSystem.hpp"

#include <cstring>

using SDK::Interface::IDirectory;
using SDK::Interface::IFile;
using SDK::Interface::IFileSystem;

namespace
{

std::string parentOf(const std::string& abs)
{
    const size_t slash = abs.find_last_of('/');
    return (slash == 0 || slash == std::string::npos) ? "/" : abs.substr(0, slash);
}

std::string nameOf(const std::string& abs)
{
    const size_t slash = abs.find_last_of('/');
    return slash == std::string::npos ? abs : abs.substr(slash + 1);
}

class TreeFile : public IFile
{
public:
    TreeFile(TreeFileSystem& fs, std::string path) : mFs(fs), mPath(std::move(path)) {}

    void setPath(const char* path) override { mPath = path ? path : ""; }
    const char* getPath() const override { return mPath.c_str(); }
    bool exist() const override { return mFs.exist(mPath.c_str()); }
    bool rename(const char* newPath) override { return mFs.rename(mPath.c_str(), newPath); }
    bool remove() override { return mFs.remove(mPath.c_str()); }

    size_t size() const override
    {
        const auto* n = mFs.find(mFs.resolve(mPath));
        return (n && !n->isDir) ? n->data.size() : 0;
    }

    bool open(bool wMode, bool override) override
    {
        const std::string abs = mFs.resolve(mPath);
        if (!mFs.allowed(abs)) {
            return false;
        }
        auto* n = mFs.find(abs);
        if (n && n->isDir) {
            return false;
        }
        if (!n) {
            if (!wMode) {
                return false;
            }
            const auto* parent = mFs.find(parentOf(abs));
            if (!parent || !parent->isDir) {
                return false;   // FatFs: no such path
            }
            mFs.addFile(abs, "");
        } else if (wMode && override) {
            n->data.clear();
        }
        mAbs  = abs;
        mOpen = true;
        mPos  = 0;
        mWrite = wMode;
        return true;
    }

    bool isOpen() const override { return mOpen; }
    bool close() override { mOpen = false; return true; }

    bool read(char* buff, size_t btr, size_t& br) override
    {
        br = 0;
        auto* n = mOpen ? mFs.find(mAbs) : nullptr;
        if (!n) {
            return false;
        }
        const size_t avail = mPos < n->data.size() ? n->data.size() - mPos : 0;
        br = btr < avail ? btr : avail;
        std::memcpy(buff, n->data.data() + mPos, br);
        mPos += br;
        return true;
    }

    bool write(const char* buff, size_t btw, size_t& bw) override
    {
        bw = 0;
        auto* n = (mOpen && mWrite) ? mFs.find(mAbs) : nullptr;
        if (!n) {
            return false;
        }
        if (mPos > n->data.size()) {
            n->data.resize(mPos);
        }
        n->data.replace(mPos, btw, buff, btw);
        mPos += btw;
        bw = btw;
        return true;
    }

    bool seek(size_t offset) override { mPos = offset; return mOpen; }
    bool truncate(size_t offset) override
    {
        auto* n = mOpen ? mFs.find(mAbs) : nullptr;
        if (!n) {
            return false;
        }
        n->data.resize(offset);
        return true;
    }
    bool flush() override { return mOpen; }
    size_t getPosition() const override { return mPos; }

private:
    TreeFileSystem& mFs;
    std::string     mPath;
    std::string     mAbs;
    bool            mOpen  = false;
    bool            mWrite = false;
    size_t          mPos   = 0;
};

class TreeDir : public IDirectory
{
public:
    TreeDir(TreeFileSystem& fs, std::string path) : mFs(fs), mPath(std::move(path)) {}

    void setPath(const char* path) override { mPath = path ? path : ""; }
    const char* getPath() const override { return mPath.c_str(); }
    bool exist() const override { return mFs.exist(mPath.c_str()); }
    bool rename(const char* newPath) override { return mFs.rename(mPath.c_str(), newPath); }
    bool remove() override { return mFs.remove(mPath.c_str()); }
    bool create() override { return mFs.mkdir(mPath.c_str()); }

    bool open() override
    {
        const std::string abs = mFs.resolve(mPath);
        const auto*       n   = mFs.find(abs);
        if (!mFs.allowed(abs) || !n || !n->isDir) {
            return false;
        }
        mAbs     = abs;
        mEntries = mFs.children(abs);
        mNext    = 0;
        mOpen    = true;
        return true;
    }

    bool isOpen() const override { return mOpen; }

    bool readNext(IFileSystem::ObjectInfo& item, bool reset) override
    {
        if (!mOpen) {
            return false;
        }
        if (reset) {
            mNext = 0;
            return true;
        }
        while (mNext < mEntries.size()) {
            const std::string child = (mAbs == "/" ? "" : mAbs) + "/" + mEntries[mNext++];
            if (mFs.objectInfo(child.c_str(), item)) {
                return true;
            }
        }
        return false;
    }

    bool close() override { mOpen = false; return true; }

private:
    TreeFileSystem&          mFs;
    std::string              mPath;
    std::string              mAbs;
    std::vector<std::string> mEntries;
    size_t                   mNext = 0;
    bool                     mOpen = false;
};

} // namespace

TreeFileSystem::TreeFileSystem(std::string sandbox)
    : mSandbox(std::move(sandbox))
{
    mNodes["/"].isDir = true;
    addDir(mSandbox);
}

std::string TreeFileSystem::resolve(const std::string& path) const
{
    std::string p = path;
    // A drive prefix ("2:/Apps") names the one volume this models.
    if (p.size() >= 2 && p[1] == ':') {
        p = p.substr(2);
    }
    std::string full = (!p.empty() && p[0] == '/') ? p : mSandbox + "/" + p;

    std::vector<std::string> parts;
    size_t start = 0;
    while (start <= full.size()) {
        const size_t end = full.find('/', start);
        const std::string part = full.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (part == "..") {
            if (!parts.empty()) {
                parts.pop_back();
            }
        } else if (!part.empty() && part != ".") {
            parts.push_back(part);
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    std::string out;
    for (const auto& part : parts) {
        out += "/" + part;
    }
    return out.empty() ? "/" : out;
}

bool TreeFileSystem::allowed(const std::string& abs) const
{
    if (!mBlockParent) {
        return true;
    }
    auto under = [&abs](const std::string& root) {
        return abs == root || abs.compare(0, root.size() + 1, root + "/") == 0;
    };
    return under(mSandbox) || under("/Apps/SharedData");
}

TreeFileSystem::Node* TreeFileSystem::find(const std::string& abs)
{
    auto it = mNodes.find(abs);
    return it == mNodes.end() ? nullptr : &it->second;
}

const TreeFileSystem::Node* TreeFileSystem::find(const std::string& abs) const
{
    auto it = mNodes.find(abs);
    return it == mNodes.end() ? nullptr : &it->second;
}

std::vector<std::string> TreeFileSystem::children(const std::string& abs) const
{
    std::vector<std::string> out;
    for (const auto& kv : mNodes) {
        if (kv.first != "/" && kv.first != abs && parentOf(kv.first) == abs) {
            out.push_back(nameOf(kv.first));
        }
    }
    return out;
}

void TreeFileSystem::ensureParents(const std::string& abs)
{
    const std::string parent = parentOf(abs);
    if (parent != abs && !find(parent)) {
        ensureParents(parent);
        mNodes[parent].isDir = true;
    }
}

void TreeFileSystem::addFile(const std::string& path, std::string content, time_t utc)
{
    const std::string abs = resolve(path);
    ensureParents(abs);
    Node& n = mNodes[abs];
    n.isDir = false;
    n.data  = std::move(content);
    n.utc   = utc;
}

void TreeFileSystem::addDir(const std::string& path)
{
    const std::string abs = resolve(path);
    ensureParents(abs);
    mNodes[abs].isDir = true;
}

bool TreeFileSystem::hasFile(const std::string& path) const
{
    const auto* n = find(resolve(path));
    return n && !n->isDir;
}

std::string TreeFileSystem::content(const std::string& path) const
{
    const auto* n = find(resolve(path));
    return n ? n->data : std::string();
}

bool TreeFileSystem::mkdir(const char* path)
{
    const std::string abs = resolve(path ? path : "");
    if (!allowed(abs)) {
        return false;
    }
    if (const auto* n = find(abs)) {
        return n->isDir;
    }
    ensureParents(abs);
    mNodes[abs].isDir = true;
    return true;
}

std::unique_ptr<IFile> TreeFileSystem::file(const char* path)
{
    return std::make_unique<TreeFile>(*this, path ? path : "");
}

std::unique_ptr<IDirectory> TreeFileSystem::dir(const char* path)
{
    return std::make_unique<TreeDir>(*this, path ? path : "");
}

bool TreeFileSystem::exist(const char* path) const
{
    const std::string abs = resolve(path ? path : "");
    return allowed(abs) && find(abs) != nullptr;
}

bool TreeFileSystem::remove(const char* path)
{
    const std::string abs = resolve(path ? path : "");
    if (!allowed(abs) || !find(abs) || abs == "/") {
        return false;
    }
    if (find(abs)->isDir && !children(abs).empty()) {
        return false;   // FatFs: a directory must be empty
    }
    mNodes.erase(abs);
    return true;
}

bool TreeFileSystem::rename(const char* oldPath, const char* newPath)
{
    const std::string from = resolve(oldPath ? oldPath : "");
    const std::string to   = resolve(newPath ? newPath : "");
    if (!allowed(from) || !allowed(to) || !find(from) || find(to) || find(from)->isDir) {
        return false;   // FatFs f_rename: FR_EXIST when the destination exists
    }
    mNodes[to] = mNodes[from];
    mNodes.erase(from);
    return true;
}

bool TreeFileSystem::copy(const char* oldPath, const char* newPath)
{
    const std::string from = resolve(oldPath ? oldPath : "");
    const std::string to   = resolve(newPath ? newPath : "");
    if (!allowed(from) || !allowed(to) || !find(from) || find(from)->isDir) {
        return false;
    }
    mNodes[to] = mNodes[from];
    return true;
}

bool TreeFileSystem::objectInfo(const char* path, ObjectInfo& item) const
{
    const std::string abs = resolve(path ? path : "");
    const auto*       n   = find(abs);
    if (!allowed(abs) || !n) {
        return false;
    }
    std::memset(&item, 0, sizeof(item));
    const std::string name = nameOf(abs);
    std::strncpy(item.name, name.c_str(), sizeof(item.name) - 1);
    item.isDir = n->isDir;
    item.size  = n->data.size();
    item.utc   = n->utc;
    return true;
}
