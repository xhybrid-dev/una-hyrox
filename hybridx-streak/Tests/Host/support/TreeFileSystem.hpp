/**
 ******************************************************************************
 * @file    TreeFileSystem.hpp
 * @brief   An in-memory directory tree behind SDK::Interface::IFileSystem.
 *
 * The SDK's own test file systems cannot list a directory
 * (InMemoryFileSystem's directories are always empty; FakeFileSystem has
 * none), and the streak's whole job is listing other apps' Activity folders.
 * So this models the watch's volume as a tree, with the app's sandbox as the
 * current directory:
 *
 *   - relative paths resolve against the sandbox, e.g. "/Apps/HybridXStreak/";
 *   - ".." and "." are resolved, so "../Running/Activity" reaches a sibling
 *     app, as the SDK's own "../SharedData/" does (hybridx-streak NOTES E.3);
 *   - absolute paths ("/Apps/...") work, and "2:/..." drive prefixes are
 *     stripped, so a probe of either form behaves as on a single volume;
 *   - blockParentAccess() makes anything outside the sandbox (other than
 *     SharedData) look absent, to test the "firmware forbids it" outcome;
 *   - rename refuses an existing destination, as FatFs's f_rename does.
 ******************************************************************************
 */

#ifndef STREAK_TREE_FILE_SYSTEM_HPP
#define STREAK_TREE_FILE_SYSTEM_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "SDK/Interfaces/IFileSystem.hpp"

class TreeFileSystem : public SDK::Interface::IFileSystem
{
public:
    explicit TreeFileSystem(std::string sandbox = "/Apps/HybridXStreak");

    // -- Test setup ---------------------------------------------------------------
    void addFile(const std::string& path, std::string content, time_t utc = 0);
    void addDir(const std::string& path);
    bool hasFile(const std::string& path) const;
    std::string content(const std::string& path) const;
    /// Pretend the firmware confines an app to its sandbox (and SharedData).
    void blockParentAccess(bool block) { mBlockParent = block; }
    /// Normalise a path the way every operation does (public for tests).
    std::string resolve(const std::string& path) const;

    // -- IFileSystem ----------------------------------------------------------------
    bool mkdir(const char* path) override;
    std::unique_ptr<SDK::Interface::IFile> file(const char* path) override;
    std::unique_ptr<SDK::Interface::IDirectory> dir(const char* path) override;
    bool exist(const char* path) const override;
    bool remove(const char* path) override;
    bool rename(const char* oldPath, const char* newPath) override;
    bool copy(const char* oldPath, const char* newPath) override;
    bool objectInfo(const char* path, ObjectInfo& item) const override;

    struct Node {
        bool        isDir = false;
        std::string data;
        time_t      utc = 0;
    };

    /// Visible to the file and directory objects.
    bool allowed(const std::string& abs) const;
    Node* find(const std::string& abs);
    const Node* find(const std::string& abs) const;
    std::vector<std::string> children(const std::string& abs) const;
    void ensureParents(const std::string& abs);

private:
    std::string                 mSandbox;
    std::map<std::string, Node> mNodes;   ///< absolute path -> node; "/" is the root
    bool                        mBlockParent = false;
};

#endif // STREAK_TREE_FILE_SYSTEM_HPP
