/**
 ******************************************************************************
 * @file    StateStore.hpp
 * @brief   The counter's file on the watch: save after a change, load at start.
 *
 * state.txt in the app's own folder (Apps/Knitting/), written whole through
 * SafeFile, which keeps state.txt.bak as the previous good copy. A load tries
 * the file, then the backup, then gives a fresh counter, so a file that is
 * missing, torn or edited into nonsense costs at most the last change.
 *
 * On SDK::Interface::IFileSystem, so the host tests run it on an in-memory tree.
 ******************************************************************************
 */

#ifndef KNITTING_STATE_STORE_HPP
#define KNITTING_STATE_STORE_HPP

#include "SDK/Interfaces/IFileSystem.hpp"

#include "RowCounter.hpp"

namespace Knitting::StateStore
{

constexpr const char* kFile = "state.txt";

/// Where a loaded state came from.
enum class Source : uint8_t {
    Fresh,      ///< Nothing usable: a new counter.
    File,       ///< state.txt.
    Backup,     ///< state.txt.bak, because state.txt was missing or unusable.
};

struct Loaded {
    CounterState state;
    Source       source = Source::Fresh;
};

/// Save @p state. False if the write failed (the previous copy stands).
bool save(SDK::Interface::IFileSystem& fs, const CounterState& state);

/// Load the saved state, or a fresh counter.
Loaded load(SDK::Interface::IFileSystem& fs);

} // namespace Knitting::StateStore

#endif // KNITTING_STATE_STORE_HPP
