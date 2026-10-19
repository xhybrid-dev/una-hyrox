/**
 ******************************************************************************
 * @file    SaveStore.cpp
 * @brief   The game's save file on the watch (see the header).
 ******************************************************************************
 */

#include "SaveStore.hpp"

#include "SafeFile.hpp"
#include "SaveCodec.hpp"

namespace TrailQuest::SaveStore
{

bool save(SDK::Interface::IFileSystem& fs, const RunState& state)
{
    if (!SaveCodec::valid(state)) {
        return false;   // never overwrite a good save with a bad one
    }
    char         buf[SaveCodec::kMaxBytes];
    const size_t len = SaveCodec::encode(state, buf, sizeof(buf));
    return len > 0 && SafeFile::write(fs, kFile, buf, len);
}

Loaded load(SDK::Interface::IFileSystem& fs)
{
    Loaded result;
    char   buf[SaveCodec::kMaxBytes];
    for (int pass = 0; pass < 2; ++pass) {
        const bool   backup = (pass == 1);
        const size_t len    = SafeFile::read(fs, kFile, backup, buf, sizeof(buf));
        RunState     s;
        if (len > 0 && SaveCodec::decode(buf, len, s)) {
            result.state  = s;
            result.source = backup ? Source::Backup : Source::File;
            return result;
        }
    }
    return result;
}

bool saveSettings(SDK::Interface::IFileSystem& fs, const Settings& settings)
{
    const char* text = settings.beat ? "beat=1\n" : "beat=0\n";
    return SafeFile::write(fs, kSettingsFile, text, 7);
}

Settings loadSettings(SDK::Interface::IFileSystem& fs)
{
    Settings s;
    char     buf[16];
    for (int pass = 0; pass < 2; ++pass) {
        const size_t len = SafeFile::read(fs, kSettingsFile, pass == 1, buf, sizeof(buf));
        if (len >= 6 && buf[0] == 'b' && buf[1] == 'e' && buf[2] == 'a' && buf[3] == 't' && buf[4] == '='
            && (buf[5] == '0' || buf[5] == '1')) {
            s.beat = buf[5] == '1';
            return s;
        }
    }
    return s;
}

} // namespace TrailQuest::SaveStore
