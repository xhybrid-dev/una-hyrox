/**
 ******************************************************************************
 * @file    FlatFileSystem.cpp
 * @brief   FlatFile / FlatDir (see the header).
 ******************************************************************************
 */

#include "FlatFileSystem.hpp"

using SDK::Interface::IDirectory;
using SDK::Interface::IFile;
using SDK::Interface::IFileSystem;

namespace
{

class FlatFile : public IFile
{
public:
    FlatFile(FlatFileSystem& fs, std::string path) : mFs(fs), mPath(FlatFileSystem::norm(std::move(path))) {}

    void setPath(const char* path) override { mPath = FlatFileSystem::norm(path ? path : ""); }
    const char* getPath() const override { return mPath.c_str(); }
    bool exist() const override { return mFs.find(mPath) != nullptr; }
    bool rename(const char*) override { return false; }
    bool remove() override { return mFs.remove(mPath.c_str()); }

    size_t size() const override
    {
        auto* n = mFs.find(mPath);
        return (n && !n->isDir) ? n->data.size() : 0;
    }

    bool open(bool wMode, bool overrideExisting) override
    {
        auto* n = mFs.find(mPath);
        if (n && n->isDir) {
            return false;
        }
        if (!n) {
            if (!wMode) {
                return false;
            }
            mFs.addFile(mPath, "");   // FA_OPEN_ALWAYS: a fresh write creates the file
            n = mFs.find(mPath);
        } else if (wMode && overrideExisting) {
            n->data.clear();
        }
        mOpen  = true;
        mWrite = wMode;
        mPos   = 0;
        return n != nullptr;
    }

    bool isOpen() const override { return mOpen; }
    bool close() override { mOpen = false; return true; }

    bool read(char* buff, size_t btr, size_t& br) override
    {
        br      = 0;
        auto* n = mOpen ? mFs.find(mPath) : nullptr;
        if (!n) {
            return false;
        }
        const size_t avail = mPos < n->data.size() ? n->data.size() - mPos : 0;
        br                 = btr < avail ? btr : avail;
        std::memcpy(buff, n->data.data() + mPos, br);
        mPos += br;
        return true;
    }

    bool write(const char* buff, size_t btw, size_t& bw) override
    {
        bw      = 0;
        auto* n = (mOpen && mWrite) ? mFs.find(mPath) : nullptr;
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
        auto* n = mOpen ? mFs.find(mPath) : nullptr;
        if (!n) {
            return false;
        }
        n->data.resize(offset);
        return true;
    }
    bool flush() override { return mOpen; }
    size_t getPosition() const override { return mPos; }

private:
    FlatFileSystem& mFs;
    std::string     mPath;
    bool            mOpen  = false;
    bool            mWrite = false;
    size_t          mPos   = 0;
};

class FlatDir : public IDirectory
{
public:
    FlatDir(FlatFileSystem& fs, std::string path) : mFs(fs), mPath(FlatFileSystem::norm(std::move(path))) {}

    void setPath(const char* path) override { mPath = FlatFileSystem::norm(path ? path : ""); }
    const char* getPath() const override { return mPath.c_str(); }
    bool exist() const override
    {
        auto* n = mFs.find(mPath);
        return n && n->isDir;
    }
    bool rename(const char*) override { return false; }
    bool remove() override { return mFs.remove(mPath.c_str()); }
    bool create() override { return mFs.mkdir(mPath.c_str()); }

    bool open() override
    {
        auto* n = mFs.find(mPath);
        if (!n || !n->isDir) {
            return false;
        }
        mEntries = mFs.children(mPath);
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
            const std::string childPath = mPath.empty() ? mEntries[mNext] : mPath + "/" + mEntries[mNext];
            ++mNext;
            if (mFs.objectInfo(childPath.c_str(), item)) {
                return true;
            }
        }
        return false;
    }

    bool close() override { mOpen = false; return true; }

private:
    FlatFileSystem&          mFs;
    std::string              mPath;
    std::vector<std::string> mEntries;
    size_t                   mNext = 0;
    bool                     mOpen = false;
};

} // namespace

std::unique_ptr<IFile> FlatFileSystem::file(const char* path)
{
    return std::make_unique<FlatFile>(*this, path ? path : "");
}

std::unique_ptr<IDirectory> FlatFileSystem::dir(const char* path)
{
    return std::make_unique<FlatDir>(*this, path ? path : "");
}
