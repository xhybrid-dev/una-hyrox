/**
 * make_fit: write one activity .fit with the SDK's real FitWriter, for the
 * simulator's fixture tree (docs/experiments/sim_fixtures.sh).
 *
 *   make_fit <out.fit> <sport> <sub_sport> <local YYYYMMDDTHHMMSS> <minutes> [utc offset min]
 *
 * sport and sub_sport are FitProfile.hpp values (Running 1, Cycling 2,
 * Training 10, Walking 11, Hiking 17, Generic 0; sub-sport Treadmill 1).
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include "FitFixture.hpp"

namespace
{

/// A plain stdio file behind IFile.
class DiskFile : public SDK::Interface::IFile
{
public:
    explicit DiskFile(const char* path) : mPath(path) {}
    ~DiskFile() override { close(); }

    void setPath(const char*) override {}
    const char* getPath() const override { return mPath; }
    bool exist() const override { return true; }
    bool rename(const char*) override { return false; }
    bool remove() override { return false; }
    size_t size() const override
    {
        if (!mFp) return 0;
        const long pos = std::ftell(mFp);
        std::fseek(mFp, 0, SEEK_END);
        const long end = std::ftell(mFp);
        std::fseek(mFp, pos, SEEK_SET);
        return static_cast<size_t>(end);
    }
    bool open(bool wMode, bool override) override
    {
        close();
        if (!wMode) {
            mFp = std::fopen(mPath, "rb");
        } else if (override) {
            mFp = std::fopen(mPath, "w+b");
        } else {
            // FA_OPEN_ALWAYS: keep the contents, create if missing.
            mFp = std::fopen(mPath, "r+b");
            if (!mFp) mFp = std::fopen(mPath, "w+b");
        }
        return mFp != nullptr;
    }
    bool isOpen() const override { return mFp != nullptr; }
    bool close() override
    {
        if (mFp) std::fclose(mFp);
        mFp = nullptr;
        return true;
    }
    bool read(char* buff, size_t btr, size_t& br) override
    {
        br = mFp ? std::fread(buff, 1, btr, mFp) : 0;
        return mFp != nullptr;
    }
    bool write(const char* buff, size_t btw, size_t& bw) override
    {
        bw = mFp ? std::fwrite(buff, 1, btw, mFp) : 0;
        return bw == btw;
    }
    bool seek(size_t offset) override { return mFp && std::fseek(mFp, static_cast<long>(offset), SEEK_SET) == 0; }
    bool truncate(size_t offset) override
    {
        return mFp && std::fflush(mFp) == 0 && ftruncate(fileno(mFp), static_cast<off_t>(offset)) == 0;
    }
    bool flush() override { return mFp && std::fflush(mFp) == 0; }
    size_t getPosition() const override { return mFp ? static_cast<size_t>(std::ftell(mFp)) : 0; }

private:
    const char* mPath;
    FILE*       mFp = nullptr;
};

} // namespace

int main(int argc, char** argv)
{
    if (argc < 6 || std::strlen(argv[4]) != 15 || argv[4][8] != 'T') {
        std::fprintf(stderr, "usage: make_fit <out.fit> <sport> <sub_sport> <YYYYMMDDTHHMMSS> <minutes> [utc_offset_min]\n");
        return 2;
    }
    int y, mo, d, h, mi, s;
    if (std::sscanf(argv[4], "%4d%2d%2dT%2d%2d%2d", &y, &mo, &d, &h, &mi, &s) != 6) {
        std::fprintf(stderr, "bad time %s\n", argv[4]);
        return 2;
    }
    Fixture::FitSpec spec;
    spec.sport     = static_cast<uint8_t>(std::atoi(argv[2]));
    spec.subSport  = static_cast<uint8_t>(std::atoi(argv[3]));
    spec.timerS    = static_cast<uint32_t>(std::atoi(argv[5])) * 60u;
    spec.startUnix = Fixture::unixOf(y, mo, d, h, mi, s, argc > 6 ? std::atoi(argv[6]) : 0);

    DiskFile file(argv[1]);
    if (!file.open(true, true) || !Fixture::writeFit(file, spec)) {
        std::fprintf(stderr, "failed to write %s\n", argv[1]);
        return 1;
    }
    file.close();
    return 0;
}
