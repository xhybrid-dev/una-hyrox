/**
 ******************************************************************************
 * @file    FitSessionReader.hpp
 * @brief   Reads the one `session` message out of a .fit file, and nothing else.
 *
 * The SDK writes FIT but cannot read it (NOTES E.3), so this is a small,
 * bounded, streaming parser (PLAN 5.2):
 *   - one fixed 512-byte buffer, no heap, one pass over the file;
 *   - checks the header (12 or 14 bytes, ".FIT", a non-zero data size) and
 *     that the file holds header + data + the 2-byte CRC;
 *   - keeps a definition table for the 16 local message types, honouring
 *     each definition's byte order, developer fields and compressed-timestamp
 *     record headers;
 *   - decodes only `session` (global 18), and only the fields the streak needs;
 *   - checks the whole-file CRC with the SDK's fitCrcUpdate().
 *
 * Every number comes from una-sdk Libs/Header/SDK/Fit/FitProfile.hpp:
 * MesgNum::Session = 18; session StartTime 2, Sport 5, SubSport 6,
 * TotalElapsedTime 7, TotalTimerTime 8 (scale 1000, s).
 *
 * A file that fails any check is REJECTED, not counted, and is looked at again
 * on the next scan in case it was being written (PLAN 5.2).
 ******************************************************************************
 */

#ifndef STREAK_FIT_SESSION_READER_HPP
#define STREAK_FIT_SESSION_READER_HPP

#include <cstddef>
#include <cstdint>

#include "SDK/Interfaces/IFileSystem.hpp"

namespace Streak
{

/// Why a file was not read.
enum class FitReject : uint8_t {
    None,
    Open,        ///< could not be opened
    Header,      ///< not a FIT header
    Truncated,   ///< shorter than its header says, or never finalised
    Format,      ///< a record that does not parse
    Crc,         ///< the file CRC does not match
    NoSession,   ///< a valid file with no session message
};

struct FitSession {
    FitReject reject       = FitReject::Open;
    uint32_t  startFit     = 0;      ///< session start_time, s since the FIT epoch (UTC)
    uint8_t   sport        = 0xFF;   ///< FIT sport enum; 0xFF when absent
    uint8_t   subSport     = 0xFF;
    uint32_t  timerSeconds = 0;      ///< total_timer_time, or total_elapsed_time if absent

    bool ok() const { return reject == FitReject::None; }
};

/// Seconds between the Unix epoch and the FIT epoch (1989-12-31 00:00 UTC),
/// as the SDK's ActivityWriter uses (skFitEpochOffset).
constexpr uint32_t kFitEpochOffset = 631065600u;

class FitSessionReader
{
public:
    static constexpr size_t kBufferBytes = 512;

    /// Read @p path's first session. The reader keeps its buffers between
    /// calls, so keep one instance (a static one on the watch).
    FitSession read(SDK::Interface::IFileSystem& fs, const char* path);

private:
    struct Definition {
        bool     defined   = false;
        bool     bigEndian = false;
        uint16_t global    = 0;
        uint16_t size      = 0;       ///< bytes in one data message, dev fields included
        // Offsets of the session fields in a data message, -1 when absent.
        int16_t  offStart = -1, offSport = -1, offSubSport = -1, offElapsed = -1, offTimer = -1;
        uint8_t  sizeStart = 0, sizeSport = 0, sizeSubSport = 0, sizeElapsed = 0, sizeTimer = 0;
    };

    bool     refill();
    bool     take(uint8_t& out);
    bool     take(uint8_t* out, size_t n);
    bool     skip(size_t n);
    uint32_t value(const uint8_t* p, uint8_t size, bool bigEndian) const;

    SDK::Interface::IFile* mFile = nullptr;
    uint8_t    mBuf[kBufferBytes] {};
    size_t     mLen      = 0;      ///< valid bytes in mBuf
    size_t     mPos      = 0;      ///< next byte in mBuf
    uint32_t   mOffset   = 0;      ///< file offset of mBuf[0]
    uint32_t   mCrcEnd   = 0;      ///< bytes covered by the file CRC
    uint16_t   mCrc      = 0;
    Definition mDefs[16] {};
    uint8_t    mRecord[256] {};    ///< one session data message (sizes beyond this are skipped)
};

} // namespace Streak

#endif // STREAK_FIT_SESSION_READER_HPP
