/**
 ******************************************************************************
 * @file    ProbeRunner.hpp
 * @brief   The Trail Probe's route check, over IFileSystem, with no kernel.
 *
 * Answers the route half of Gate T0: after Jon copies a GPX into this app's
 * "Routes/" folder over USB, can the app find it and read it into a route?
 *
 *   1. ensure "Routes/" exists (IFileSystem::mkdir succeeds whether it
 *      creates the folder or finds it there), so Jon has a folder to copy into
 *      after the first run;
 *   2. list it, bounded, counting .gpx files and skipping everything else,
 *      including the "._name.gpx" files a Mac leaves on a FAT drive;
 *   3. stream the newest .gpx through Trail::GpxReader into a
 *      Trail::RouteBuilder in 512-byte reads.
 *
 * Pure C++ over SDK::Interface::IFileSystem, so the host tests run it against
 * an in-memory fake rather than needing a watch.
 ******************************************************************************
 */

#ifndef TRAIL_PROBE_RUNNER_HPP
#define TRAIL_PROBE_RUNNER_HPP

#include <cstddef>
#include <cstdint>

#include "SDK/Interfaces/IFileSystem.hpp"

#include "GpxReader.hpp"
#include "ProbeResult.hpp"
#include "RouteBuilder.hpp"

namespace Probe
{

/// Where the report's narration goes.
class Host
{
public:
    /// One line of the report, without its newline.
    virtual void line(const char* text) = 0;

protected:
    ~Host() = default;
};

class Runner
{
public:
    static constexpr uint16_t    kMaxEntries = 500;       ///< bounds the directory read
    static constexpr size_t      kChunk      = 512;
    static constexpr uint32_t    kMaxBytes   = 16u << 20; ///< stop reading past 16 MB
    static constexpr const char* kRoutesDir  = "Routes";

    Runner(SDK::Interface::IFileSystem& fs, Host& host, Trail::RouteBuilder& route);

    /// Run the check, reporting through the host; fills the Routes/ fields of @p r.
    void run(Result& r);

    /// Does @p name look like a GPX we should read? ".gpx" in any case, and not
    /// a hidden file (a leading '.', which covers macOS's "._" companions).
    static bool isRouteFile(const char* name);

    /// Copy @p in to @p out as printable ASCII: each UTF-8 sequence becomes one
    /// '?', control characters become '.'. The watch fonts are ASCII only.
    static void asciiFold(const char* in, char* out, size_t outCap);

private:
    SDK::Interface::IFileSystem& mFs;
    Host&                        mHost;
    Trail::RouteBuilder&         mRoute;
    Trail::GpxReader             mReader;
    SDK::Interface::IFileSystem::ObjectInfo mInfo {};
    char mNewest[SDK::Interface::IFileSystem::skMaxPathLen] {};
    char mPath[SDK::Interface::IFileSystem::skMaxPathLen] {};
    char mLine[256] {};
    char mChunk[kChunk] {};
};

} // namespace Probe

#endif // TRAIL_PROBE_RUNNER_HPP
