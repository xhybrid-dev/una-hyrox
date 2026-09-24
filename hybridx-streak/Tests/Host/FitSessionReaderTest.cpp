/**
 * Host tests for Streak::FitSessionReader: real writer output (the SDK's
 * FitWriter, and HybridX Race's ActivityWriter files), every SDK sport, and the
 * ways a file can be broken. Expected values for Race's files are fitdecode's
 * (fitdecode 0.11, raw values), so the reader agrees with an independent decoder.
 */

#include <gtest/gtest.h>

#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "FitFixture.hpp"
#include "FitSessionReader.hpp"
#include "SDK/Fit/FitCrc.hpp"
#include "SDK/Fit/FitProfile.hpp"
#include "SDK/Fit/FitWriter.hpp"
#include "TreeFileSystem.hpp"

using Streak::FitReject;
using Streak::FitSession;
using Streak::FitSessionReader;

namespace
{

FitSession readBytes(const std::string& bytes)
{
    TreeFileSystem fs("/Apps/HybridXStreak");
    fs.addFile("/Apps/Running/Activity/202609/a.fit", bytes);
    static FitSessionReader reader;
    return reader.read(fs, "../Running/Activity/202609/a.fit");
}

std::string readDisk(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

/// Append a little-endian CRC over everything so far.
std::string withCrc(std::string bytes)
{
    const uint16_t crc = SDK::Fit::fitCrcUpdate(0, bytes.data(), bytes.size());
    bytes.push_back(static_cast<char>(crc & 0xFF));
    bytes.push_back(static_cast<char>(crc >> 8));
    return bytes;
}

std::string header14(uint32_t dataSize)
{
    std::string h(14, '\0');
    h[0] = 14;
    h[1] = 0x20;
    h[4] = static_cast<char>(dataSize & 0xFF);
    h[5] = static_cast<char>((dataSize >> 8) & 0xFF);
    h[6] = static_cast<char>((dataSize >> 16) & 0xFF);
    h[7] = static_cast<char>(dataSize >> 24);
    std::memcpy(&h[8], ".FIT", 4);
    return h;
}

} // namespace

TEST(FitSessionReader, ReadsTheSdkWritersSession)
{
    Fixture::FitSpec spec;
    spec.sport     = 1;
    spec.subSport  = 0;
    spec.startUnix = Fixture::unixOf(2026, 9, 22, 7, 15, 0);
    spec.timerS    = 1834;
    const FitSession s = readBytes(Fixture::fitBytes(spec));

    ASSERT_TRUE(s.ok()) << static_cast<int>(s.reject);
    EXPECT_EQ(s.sport, 1);
    EXPECT_EQ(s.subSport, 0);
    EXPECT_EQ(s.timerSeconds, 1834u);
    EXPECT_EQ(s.startFit + Streak::kFitEpochOffset, spec.startUnix);
}

TEST(FitSessionReader, EverySdkSport)
{
    // FitProfile.hpp Sport and SubSport values, as each SDK app writes them (NOTES E.9).
    const struct { uint8_t sport, sub; } kCases[] = {
        {1, 0}, {1, 1}, {2, 0}, {2, 6}, {10, 0}, {11, 0}, {17, 0}, {0, 0},
    };
    for (const auto& c : kCases) {
        Fixture::FitSpec spec;
        spec.sport     = c.sport;
        spec.subSport  = c.sub;
        spec.startUnix = Fixture::unixOf(2026, 9, 20, 9, 0, 0);
        const FitSession s = readBytes(Fixture::fitBytes(spec));
        ASSERT_TRUE(s.ok());
        EXPECT_EQ(s.sport, c.sport);
        EXPECT_EQ(s.subSport, c.sub);
        EXPECT_EQ(s.timerSeconds, 1800u);
    }
}

TEST(FitSessionReader, AgreesWithFitdecodeOnHybridXRaceFiles)
{
    // Real ActivityWriter output (hybridx-race docs/experiments/fit-candidates),
    // values from fitdecode.
    const struct { const char* file; uint32_t start; uint8_t sport, sub; uint32_t timer; } kCases[] = {
        {"H-running-fresh.fit", 1159000061u, 1, 0, 4144},
        {"J-cardio-fresh.fit", 1158913661u, 10, 26, 4144},
        {"K-sim-500m-runs.fit", 1159001336u, 1, 0, 4144},
    };
    for (const auto& c : kCases) {
        const std::string bytes = readDisk(std::string(RACE_FIT_DIR) + "/" + c.file);
        ASSERT_GT(bytes.size(), 1000u) << c.file;
        const FitSession s = readBytes(bytes);
        ASSERT_TRUE(s.ok()) << c.file << " reject " << static_cast<int>(s.reject);
        EXPECT_EQ(s.startFit, c.start) << c.file;
        EXPECT_EQ(s.sport, c.sport) << c.file;
        EXPECT_EQ(s.subSport, c.sub) << c.file;
        EXPECT_EQ(s.timerSeconds, c.timer) << c.file;
    }
}

TEST(FitSessionReader, RejectsATruncatedFile)
{
    Fixture::FitSpec spec;
    std::string bytes = Fixture::fitBytes(spec);
    bytes.resize(bytes.size() - 40);
    EXPECT_EQ(readBytes(bytes).reject, FitReject::Truncated);
}

TEST(FitSessionReader, RejectsAFileNeverFinalised)
{
    // A recording in progress: the header's data size is still 0.
    Fixture::FitSpec spec;
    std::string bytes = Fixture::fitBytes(spec);
    std::memset(&bytes[4], 0, 4);
    EXPECT_EQ(readBytes(bytes).reject, FitReject::Truncated);
}

TEST(FitSessionReader, RejectsABadCrc)
{
    Fixture::FitSpec spec;
    std::string bytes = Fixture::fitBytes(spec);
    bytes[bytes.size() / 2] ^= 0x01;   // a record byte, not a header one
    const FitSession s = readBytes(bytes);
    EXPECT_FALSE(s.ok());
    EXPECT_TRUE(s.reject == FitReject::Crc || s.reject == FitReject::Format);
}

TEST(FitSessionReader, RejectsAnythingNotFit)
{
    EXPECT_EQ(readBytes("hello, this is not a FIT file at all").reject, FitReject::Header);
    EXPECT_EQ(readBytes("").reject, FitReject::Header);
}

TEST(FitSessionReader, MissingFileIsOpenFailure)
{
    TreeFileSystem   fs("/Apps/HybridXStreak");
    FitSessionReader reader;
    EXPECT_EQ(reader.read(fs, "../Running/Activity/202609/none.fit").reject, FitReject::Open);
}

TEST(FitSessionReader, NoSessionMessage)
{
    // A valid file of one record message.
    std::string data;
    data += '\x40';                        // definition, local 0
    data += std::string("\x00\x00\x14\x00\x01", 5);   // reserved, LE, global 20 (record), 1 field
    data += std::string("\xFD\x04\x86", 3);           // timestamp, 4 bytes, uint32
    data += '\x00';                        // data, local 0
    data += std::string("\x01\x02\x03\x04", 4);
    const std::string file = withCrc(header14(static_cast<uint32_t>(data.size())) + data);
    EXPECT_EQ(readBytes(file).reject, FitReject::NoSession);
}

TEST(FitSessionReader, BigEndianDefinitionAndCompressedTimestampHeader)
{
    std::string data;
    // Definition, local 1, big-endian, global 18 (session): start_time u32,
    // sport enum, sub_sport enum, total_timer_time u32.
    data += '\x41';
    data += std::string("\x00\x01\x00\x12\x04", 5);
    data += std::string("\x02\x04\x86", 3);
    data += std::string("\x05\x01\x00", 3);
    data += std::string("\x06\x01\x00", 3);
    data += std::string("\x08\x04\x86", 3);
    // Data with a compressed-timestamp header: bit 7, local type 1 in bits 5-6.
    data += static_cast<char>(0x80 | (1 << 5) | 3);
    data += std::string("\x44\x55\x66\x77", 4);   // start_time 0x44556677, big-endian
    data += '\x02';                                  // Cycling
    data += '\x06';                                  // IndoorCycling
    data += std::string("\x00\x1B\x77\x40", 4);   // 1800000 ms
    const FitSession s = readBytes(withCrc(header14(static_cast<uint32_t>(data.size())) + data));
    ASSERT_TRUE(s.ok()) << static_cast<int>(s.reject);
    EXPECT_EQ(s.startFit, 0x44556677u);
    EXPECT_EQ(s.sport, 2);
    EXPECT_EQ(s.subSport, 6);
    EXPECT_EQ(s.timerSeconds, 1800u);
}

TEST(FitSessionReader, TimerFallsBackToElapsed)
{
    std::string data;
    data += '\x40';
    data += std::string("\x00\x00\x12\x00\x02", 5);   // LE, session, 2 fields
    data += std::string("\x05\x01\x00", 3);           // sport
    data += std::string("\x07\x04\x86", 3);           // total_elapsed_time only
    data += '\x00';
    data += '\x01';
    data += std::string("\x60\xEA\x00\x00", 4);        // 60000 ms
    const FitSession s = readBytes(withCrc(header14(static_cast<uint32_t>(data.size())) + data));
    ASSERT_TRUE(s.ok());
    EXPECT_EQ(s.timerSeconds, 60u);
    EXPECT_EQ(s.startFit, 0u);
    EXPECT_EQ(s.subSport, 0xFF);
}

TEST(FitSessionReader, UndefinedLocalTypeIsFormatError)
{
    std::string data = std::string("\x03\x00\x00", 3);   // data for local 3, never defined
    EXPECT_EQ(readBytes(withCrc(header14(3) + data)).reject, FitReject::Format);
}

TEST(FitSessionReader, ReaderIsReusable)
{
    TreeFileSystem fs("/Apps/HybridXStreak");
    Fixture::FitSpec a, b;
    a.sport = 2;
    b.sport = 17;
    b.timerS = 600;
    fs.addFile("../X/a.fit", Fixture::fitBytes(a));
    fs.addFile("../X/b.fit", "junk");
    fs.addFile("../X/c.fit", Fixture::fitBytes(b));
    FitSessionReader reader;
    EXPECT_EQ(reader.read(fs, "../X/a.fit").sport, 2);
    EXPECT_FALSE(reader.read(fs, "../X/b.fit").ok());
    const FitSession c = reader.read(fs, "../X/c.fit");
    EXPECT_EQ(c.sport, 17);
    EXPECT_EQ(c.timerSeconds, 600u);
}
