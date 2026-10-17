// Writes one Trail run's window file, for the simulator and by-hand tests:
//   make_trail_run <out.json> <run-id-utc> [windows=12] [speed m/min=200] [hr=160] [max-hr=0] [grade%=0]
// The windows are steady and past the warm-up, so Run counts them all.
#include <cstdio>
#include <cstdlib>

#include "Vo2WindowFile.hpp"

using namespace RunVo2;

int main(int argc, char** argv)
{
    if (argc < 3) {
        std::fprintf(stderr, "usage: %s <out.json> <run-id-utc> [windows] [speed m/min] [hr] [max-hr] [grade %%]\n", argv[0]);
        return 2;
    }
    const uint32_t id  = static_cast<uint32_t>(std::strtoul(argv[2], nullptr, 10));
    const int      n   = argc > 3 ? std::atoi(argv[3]) : 12;
    const int      spd = argc > 4 ? std::atoi(argv[4]) : 200;
    const int      hr  = argc > 5 ? std::atoi(argv[5]) : 160;
    const int      mx  = argc > 6 ? std::atoi(argv[6]) : 0;
    const double   grd = argc > 7 ? std::atof(argv[7]) : 0.0;
    if (n < 0 || n > kWindowFileMaxWindows) {
        std::fprintf(stderr, "windows must be 0..%u\n", static_cast<unsigned>(kWindowFileMaxWindows));
        return 2;
    }

    static WindowRecord w[kWindowFileMaxWindows];
    for (int i = 0; i < n; ++i) {
        w[i].activeSec    = 360u + 60u * static_cast<uint32_t>(i);
        w[i].goodSec      = 60;
        w[i].speedMMinX10 = static_cast<uint16_t>(spd * 10);
        w[i].spreadX10    = 20;
        w[i].gradeX10     = static_cast<int16_t>(grd * 10.0);
        w[i].hrX10        = static_cast<uint16_t>(hr * 10);
        w[i].hrMin        = static_cast<uint8_t>(hr - 2);
        w[i].hrMax        = static_cast<uint8_t>(hr + 2);
    }
    static char buf[kWindowFileBytes];
    const size_t len = writeWindowFile(buf, sizeof(buf), id, static_cast<uint8_t>(mx), w, static_cast<uint16_t>(n));
    if (len == 0) {
        std::fprintf(stderr, "could not write the file\n");
        return 1;
    }
    std::FILE* f = std::fopen(argv[1], "wb");
    if (f == nullptr || std::fwrite(buf, 1, len, f) != len) {
        std::fprintf(stderr, "could not write %s\n", argv[1]);
        return 1;
    }
    std::fclose(f);
    std::printf("%s: run %u, %d windows, %zu bytes\n", argv[1], id, n, len);
    return 0;
}
