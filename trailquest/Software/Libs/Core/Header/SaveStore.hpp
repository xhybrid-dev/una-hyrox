/**
 ******************************************************************************
 * @file    SaveStore.hpp
 * @brief   The game's save file on the watch: save at a save point, load at
 *          start.
 *
 * save.txt in the app's own folder (Apps/TrailQuest/), written whole through
 * SafeFile, which keeps save.txt.bak as the previous good copy. A load tries
 * the file, then the backup, then reports nothing usable (the caller starts a
 * new game), so a file that is missing, torn or edited into nonsense costs at
 * most the last save. Knitting's StateStore is the pattern.
 *
 * On SDK::Interface::IFileSystem, so the host tests run it on an in-memory tree.
 ******************************************************************************
 */

#ifndef TRAILQUEST_SAVE_STORE_HPP
#define TRAILQUEST_SAVE_STORE_HPP

#include "SDK/Interfaces/IFileSystem.hpp"

#include "RunState.hpp"

namespace TrailQuest::SaveStore
{

constexpr const char* kFile = "save.txt";

/// Where a loaded game came from.
enum class Source : uint8_t {
    None,       ///< Nothing usable: start a new game.
    File,       ///< save.txt.
    Backup,     ///< save.txt.bak, because save.txt was missing or unusable.
};

struct Loaded {
    RunState state;
    Source   source = Source::None;
};

/// Save @p state. False if it is not a valid save or the write failed (the
/// previous copy stands).
bool save(SDK::Interface::IFileSystem& fs, const RunState& state);

/// Load the saved game, if there is a usable one.
Loaded load(SDK::Interface::IFileSystem& fs);

} // namespace TrailQuest::SaveStore

#endif // TRAILQUEST_SAVE_STORE_HPP
