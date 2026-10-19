/**
 ******************************************************************************
 * @file    StateStore.cpp
 * @brief   The counter's file on the watch (see the header).
 ******************************************************************************
 */

#include "StateStore.hpp"

#include "SafeFile.hpp"
#include "StateCodec.hpp"

namespace Knitting::StateStore
{

bool save(SDK::Interface::IFileSystem& fs, const CounterState& state)
{
    char         buf[StateCodec::kMaxBytes];
    const size_t len = StateCodec::encode(state, buf, sizeof(buf));
    return len > 0 && SafeFile::write(fs, kFile, buf, len);
}

Loaded load(SDK::Interface::IFileSystem& fs)
{
    Loaded result;
    char   buf[StateCodec::kMaxBytes];

    for (int pass = 0; pass < 2; ++pass) {
        const bool   backup = (pass == 1);
        const size_t len    = SafeFile::read(fs, kFile, backup, buf, sizeof(buf));
        CounterState s;
        if (len > 0 && StateCodec::decode(buf, len, s)) {
            result.state  = s;
            result.source = backup ? Source::Backup : Source::File;
            return result;
        }
    }
    return result;
}

} // namespace Knitting::StateStore
