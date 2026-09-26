/**
 ******************************************************************************
 * @file    FlatFileSystem.hpp
 * @brief   A minimal in-memory SDK::Interface::IFileSystem, for ProbeRunner's
 *          host test.
 *
 * The SDK's own test fakes can't list a directory (each returns nothing from
 * dir()), and ProbeRunner's whole job is listing one, so a tiny fake is
 * written here instead -- unlike hybridx-streak's TreeFileSystem, this one
 * needs no ".." traversal, no drive prefixes and no rename semantics: the
 * probe only ever touches its own sandbox root and one "Routes" subfolder.
 * Paths are plain relative strings ("probe.txt", "Routes", "Routes/x.gpx");
 * leading "./" is stripped, nothing else is normalised.
 ******************************************************************************
 */

#ifndef TRAIL_FLAT_FILE_SYSTEM_HPP
#define TRAIL_FLAT_FILE_SYSTEM_HPP

#include <cstring>
#include <ctime>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "SDK/Interfaces/IFileSystem.hpp"

class FlatFileSystem : public SDK::Interface::IFileSystem
{
public:
    struct Node {
        bool        isDir = false;
        std::string data;
        time_t      utc = 0;
    };

    // -- Test setup -----------------------------------------------------------------
    void addFile(const std::string& path, std::string content, time_t utc = 0)
    {
        mNodes[norm(path)] = Node { false, std::move(content), utc };
    }
    void addDir(const std::string& path) { mNodes[norm(path)] = Node { true, "", 0 }; }
    bool hasFile(const std::string& path) const
    {
        auto it = mNodes.find(norm(path));
        return it != mNodes.end() && !it->second.isDir;
    }
    std::string content(const std::string& path) const
    {
        auto it = mNodes.find(norm(path));
        return it != mNodes.end() ? it->second.data : std::string();
    }

    // -- IFileSystem ------------------------------------------------------------------
    bool mkdir(const char* path) override
    {
        const std::string p  = norm(path ? path : "");
        auto               it = mNodes.find(p);
        if (it != mNodes.end()) {
            return it->second.isDir;   // "successfully created or existed" (IFileSystem.hpp:56-58)
        }
        mNodes[p] = Node { true, "", 0 };
        return true;
    }

    std::unique_ptr<SDK::Interface::IFile> file(const char* path) override;
    std::unique_ptr<SDK::Interface::IDirectory> dir(const char* path) override;

    bool exist(const char* path) const override { return mNodes.count(norm(path ? path : "")) > 0; }

    bool remove(const char* path) override
    {
        const std::string p = norm(path ? path : "");
        return mNodes.erase(p) > 0;
    }

    bool rename(const char*, const char*) override { return false; }   // unused by the probe
    bool copy(const char*, const char*) override { return false; }     // unused by the probe

    bool objectInfo(const char* path, ObjectInfo& item) const override
    {
        const std::string p  = norm(path ? path : "");
        auto               it = mNodes.find(p);
        if (it == mNodes.end()) {
            return false;
        }
        std::memset(&item, 0, sizeof(item));
        const std::string name = baseName(p);
        std::strncpy(item.name, name.c_str(), sizeof(item.name) - 1);
        item.isDir = it->second.isDir;
        item.size  = it->second.data.size();
        item.utc   = it->second.utc;
        return true;
    }

    // Visible to the file/directory objects.
    Node* find(const std::string& path)
    {
        auto it = mNodes.find(path);
        return it == mNodes.end() ? nullptr : &it->second;
    }
    std::vector<std::string> children(const std::string& dirPath) const
    {
        std::vector<std::string> out;
        const std::string        prefix = dirPath.empty() ? "" : dirPath + "/";
        for (const auto& kv : mNodes) {
            if (kv.first.rfind(prefix, 0) == 0 && kv.first != dirPath) {
                const std::string rest = kv.first.substr(prefix.size());
                if (rest.find('/') == std::string::npos) {   // direct child only
                    out.push_back(rest);
                }
            }
        }
        return out;
    }
    static std::string norm(std::string p)
    {
        if (p.rfind("./", 0) == 0) {
            p = p.substr(2);
        }
        while (!p.empty() && p.back() == '/') {
            p.pop_back();
        }
        return p;
    }
    static std::string baseName(const std::string& p)
    {
        const size_t slash = p.find_last_of('/');
        return slash == std::string::npos ? p : p.substr(slash + 1);
    }

private:
    std::map<std::string, Node> mNodes;
};

#endif // TRAIL_FLAT_FILE_SYSTEM_HPP
