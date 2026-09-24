/**
 ******************************************************************************
 * @file    FitSessionReader.cpp
 * @brief   Streaming FIT `session` reader (see the header).
 ******************************************************************************
 */

#include "FitSessionReader.hpp"

#include <cstring>

#include "SDK/Fit/FitCrc.hpp"

namespace Streak
{

namespace
{
// una-sdk FitProfile.hpp: MesgNum::Session and field::Session numbers.
constexpr uint16_t kMesgSession      = 18;
constexpr uint8_t  kFieldStartTime   = 2;
constexpr uint8_t  kFieldSport       = 5;
constexpr uint8_t  kFieldSubSport    = 6;
constexpr uint8_t  kFieldElapsed     = 7;
constexpr uint8_t  kFieldTimer       = 8;

// Record header bits (FIT protocol).
constexpr uint8_t kCompressedTimestamp = 0x80;
constexpr uint8_t kDefinition          = 0x40;
constexpr uint8_t kDeveloperData       = 0x20;
constexpr uint8_t kLocalTypeMask       = 0x0F;

constexpr uint32_t kInvalid32 = 0xFFFFFFFFu;
} // namespace

bool FitSessionReader::refill()
{
    mOffset += static_cast<uint32_t>(mLen);
    mLen = 0;
    mPos = 0;
    size_t got = 0;
    if (!mFile->read(reinterpret_cast<char*>(mBuf), sizeof(mBuf), got) || got == 0) {
        return false;
    }
    mLen = got;
    // Fold the part of this chunk that the file CRC covers.
    if (mOffset < mCrcEnd) {
        const uint32_t covered = (mCrcEnd - mOffset) < got ? (mCrcEnd - mOffset) : static_cast<uint32_t>(got);
        mCrc = SDK::Fit::fitCrcUpdate(mCrc, mBuf, covered);
    }
    return true;
}

bool FitSessionReader::take(uint8_t& out)
{
    if (mPos >= mLen && !refill()) {
        return false;
    }
    out = mBuf[mPos++];
    return true;
}

bool FitSessionReader::take(uint8_t* out, size_t n)
{
    while (n > 0) {
        if (mPos >= mLen && !refill()) {
            return false;
        }
        const size_t chunk = (mLen - mPos) < n ? (mLen - mPos) : n;
        std::memcpy(out, mBuf + mPos, chunk);
        mPos += chunk;
        out += chunk;
        n -= chunk;
    }
    return true;
}

bool FitSessionReader::skip(size_t n)
{
    while (n > 0) {
        if (mPos >= mLen && !refill()) {
            return false;
        }
        const size_t chunk = (mLen - mPos) < n ? (mLen - mPos) : n;
        mPos += chunk;
        n -= chunk;
    }
    return true;
}

uint32_t FitSessionReader::value(const uint8_t* p, uint8_t size, bool bigEndian) const
{
    uint32_t v = 0;
    for (uint8_t i = 0; i < size && i < 4; ++i) {
        const uint8_t b = bigEndian ? p[i] : p[size - 1 - i];
        v = (v << 8) | b;
    }
    return v;
}

FitSession FitSessionReader::read(SDK::Interface::IFileSystem& fs, const char* path)
{
    FitSession result;
    auto file = fs.file(path);
    if (!file || !file->open(false, false)) {
        return result;   // FitReject::Open
    }
    mFile   = file.get();
    mLen    = 0;
    mPos    = 0;
    mOffset = 0;
    mCrc    = 0;
    mCrcEnd = 0xFFFFFFFFu;   // until the header says otherwise, everything read is covered
    for (auto& d : mDefs) {
        d = Definition {};
    }
    const size_t fileSize = file->size();

    auto finish = [&](FitReject why) {
        file->close();
        mFile         = nullptr;
        result.reject = why;
        return result;
    };

    // -- Header -------------------------------------------------------------------
    uint8_t header[14] = {};
    if (!take(header[0]) || (header[0] != 12 && header[0] != 14)) {
        return finish(FitReject::Header);
    }
    const uint8_t headerSize = header[0];
    if (!take(header + 1, headerSize - 1u) || std::memcmp(header + 8, ".FIT", 4) != 0) {
        return finish(FitReject::Header);
    }
    const uint32_t dataSize = value(header + 4, 4, false);
    if (dataSize == 0 || fileSize < static_cast<size_t>(headerSize) + dataSize + 2u) {
        return finish(FitReject::Truncated);   // never finalised, or cut short
    }
    // The CRC covers the header and the data. The bytes already buffered were
    // folded in full by the first refill; redo the sum with the real end.
    mCrcEnd = headerSize + dataSize;
    mCrc    = SDK::Fit::fitCrcUpdate(0, mBuf, mLen < mCrcEnd ? mLen : mCrcEnd);

    // -- Records --------------------------------------------------------------------
    bool     haveSession = false;
    uint32_t consumed    = 0;   // data bytes read so far
    auto     used        = [&]() { return mOffset + static_cast<uint32_t>(mPos) - headerSize; };

    while ((consumed = used()) < dataSize) {
        uint8_t rh = 0;
        if (!take(rh)) {
            return finish(FitReject::Truncated);
        }

        if ((rh & kCompressedTimestamp) == 0 && (rh & kDefinition) != 0) {
            // Definition message.
            Definition& d = mDefs[rh & kLocalTypeMask];
            d            = Definition {};
            uint8_t fixed[5] = {};
            if (!take(fixed, sizeof(fixed))) {
                return finish(FitReject::Truncated);
            }
            d.bigEndian         = fixed[1] == 1;
            d.global            = static_cast<uint16_t>(value(fixed + 2, 2, d.bigEndian));
            const uint8_t count = fixed[4];
            uint16_t      offset = 0;
            for (uint8_t i = 0; i < count; ++i) {
                uint8_t f[3] = {};
                if (!take(f, sizeof(f))) {
                    return finish(FitReject::Truncated);
                }
                const uint8_t num = f[0], size = f[1];
                if (d.global == kMesgSession) {
                    const auto off = static_cast<int16_t>(offset);
                    if (num == kFieldStartTime) { d.offStart = off;    d.sizeStart = size; }
                    if (num == kFieldSport)     { d.offSport = off;    d.sizeSport = size; }
                    if (num == kFieldSubSport)  { d.offSubSport = off; d.sizeSubSport = size; }
                    if (num == kFieldElapsed)   { d.offElapsed = off;  d.sizeElapsed = size; }
                    if (num == kFieldTimer)     { d.offTimer = off;    d.sizeTimer = size; }
                }
                offset = static_cast<uint16_t>(offset + size);
            }
            if ((rh & kDeveloperData) != 0) {
                uint8_t devCount = 0;
                if (!take(devCount)) {
                    return finish(FitReject::Truncated);
                }
                for (uint8_t i = 0; i < devCount; ++i) {
                    uint8_t f[3] = {};
                    if (!take(f, sizeof(f))) {
                        return finish(FitReject::Truncated);
                    }
                    offset = static_cast<uint16_t>(offset + f[1]);
                }
            }
            d.size    = offset;
            d.defined = true;
            continue;
        }

        // Data message: a normal header, or a compressed-timestamp one whose
        // local type is in bits 5-6.
        const uint8_t local = (rh & kCompressedTimestamp) ? static_cast<uint8_t>((rh >> 5) & 0x03)
                                                           : static_cast<uint8_t>(rh & kLocalTypeMask);
        const Definition& d = mDefs[local];
        if (!d.defined) {
            return finish(FitReject::Format);
        }
        if (d.global != kMesgSession || haveSession || d.size > sizeof(mRecord)) {
            if (!skip(d.size)) {
                return finish(FitReject::Truncated);
            }
            continue;
        }

        if (!take(mRecord, d.size)) {
            return finish(FitReject::Truncated);
        }
        auto field = [&](int16_t off, uint8_t size) -> uint32_t {
            if (off < 0 || size == 0 || size > 4 || off + size > d.size) {
                return kInvalid32;
            }
            const uint32_t v = value(mRecord + off, size, d.bigEndian);
            // The invalid sentinel of a 1-, 2- or 4-byte unsigned field is all ones.
            const uint32_t allOnes = size >= 4 ? kInvalid32 : ((1u << (8u * size)) - 1u);
            return v == allOnes ? kInvalid32 : v;
        };
        const uint32_t start   = field(d.offStart, d.sizeStart);
        const uint32_t sport   = field(d.offSport, d.sizeSport);
        const uint32_t sub     = field(d.offSubSport, d.sizeSubSport);
        const uint32_t timer   = field(d.offTimer, d.sizeTimer);
        const uint32_t elapsed = field(d.offElapsed, d.sizeElapsed);
        result.startFit     = start == kInvalid32 ? 0 : start;
        result.sport        = sport == kInvalid32 ? 0xFF : static_cast<uint8_t>(sport);
        result.subSport     = sub == kInvalid32 ? 0xFF : static_cast<uint8_t>(sub);
        const uint32_t ms   = timer != kInvalid32 ? timer : (elapsed != kInvalid32 ? elapsed : 0);
        result.timerSeconds = ms / 1000u;   // scale 1000
        haveSession         = true;
    }
    if (consumed != dataSize) {
        return finish(FitReject::Format);   // a record ran past the data size
    }

    // -- File CRC -----------------------------------------------------------------------
    uint8_t crc[2] = {};
    if (!take(crc, sizeof(crc))) {
        return finish(FitReject::Truncated);
    }
    if (static_cast<uint16_t>(crc[0] | (crc[1] << 8)) != mCrc) {
        return finish(FitReject::Crc);
    }
    return finish(haveSession ? FitReject::None : FitReject::NoSession);
}

} // namespace Streak
