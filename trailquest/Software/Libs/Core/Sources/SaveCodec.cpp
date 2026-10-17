/**
 ******************************************************************************
 * @file    SaveCodec.cpp
 * @brief   A game in progress as text, and back (see SaveCodec.hpp).
 ******************************************************************************
 */

#include "SaveCodec.hpp"

#include <cstdio>
#include <cstring>

#include "data/Species.hpp"

namespace TrailQuest::SaveCodec
{

namespace
{

enum Key : uint8_t {
    kTq = 0, kRng, kLv, kXp, kSt, kPc, kP0, kP1, kP2, kBag, kStage, kNode, kHut, kMin,
    kWx, kLap, kBoss, kCache, kMet, kBeat, kWon, kLost, kPerf, kKeyCount
};

const char* const kNames[kKeyCount] = {
    "tq", "rng", "lv", "xp", "st", "pc", "p0", "p1", "p2", "bag", "stage", "node", "hut", "min",
    "wx", "lap", "boss", "cache", "met", "beat", "won", "lost", "perf",
};

/// Parse an unsigned decimal of up to 10 digits that fits 32 bits.
bool parseU32(const char* s, size_t len, uint32_t& out)
{
    if (len == 0 || len > 10) {
        return false;
    }
    uint64_t v = 0;
    for (size_t i = 0; i < len; ++i) {
        if (s[i] < '0' || s[i] > '9') {
            return false;
        }
        v = v * 10u + static_cast<uint32_t>(s[i] - '0');
    }
    if (v > 0xFFFFFFFFull) {
        return false;
    }
    out = static_cast<uint32_t>(v);
    return true;
}

/// "a,b,c,d,e" into five values.
bool parseBag(const char* s, size_t len, uint32_t out[Item::Count])
{
    size_t  start = 0;
    uint8_t n     = 0;
    for (size_t i = 0; i <= len; ++i) {
        if (i == len || s[i] == ',') {
            if (n >= Item::Count || !parseU32(s + start, i - start, out[n])) {
                return false;
            }
            ++n;
            start = i + 1;
        }
    }
    return n == Item::Count;
}

bool savePoint(const RunState& s)
{
    const NodeDef& n = nodeDef(s.stage, s.node);
    switch (n.kind) {
        case NodeKind::Start:
        case NodeKind::Hut:
            return true;
        case NodeKind::Boss:
            return (s.bossesBeaten & (1u << s.stage)) != 0;   // a stage cleared
        default:
            return false;
    }
}

} // namespace

bool valid(const RunState& s)
{
    if (s.rng == 0 || s.level < kMinLevel || s.level > kMaxLevel) {
        return false;
    }
    if (s.level < kMaxLevel ? s.xp >= xpToNext(s.level) : s.xp != 0) {
        return false;
    }
    if (s.partnerCount > kMaxPartners) {
        return false;
    }
    for (uint8_t i = 0; i < kMaxPartners; ++i) {
        if (i < s.partnerCount) {
            if (s.partners[i] >= Spirit::Count || isBoss(s.partners[i])) {
                return false;
            }
            for (uint8_t j = 0; j < i; ++j) {
                if (s.partners[j] == s.partners[i]) {
                    return false;
                }
            }
        } else if (s.partners[i] != kNone) {
            return false;
        }
    }
    if (bagCount(s) > kBagCapacity) {
        return false;
    }
    for (ItemId i = 0; i < Item::Count; ++i) {
        if (itemDef(i).key && s.bag[i] > 1) {
            return false;
        }
    }
    if (s.stage >= kStageCount) {
        return false;
    }
    const StageDef& st = stageDef(s.stage);
    if (s.node >= st.nodeCount || s.hutNode >= st.nodeCount) {
        return false;
    }
    const NodeKind hut = st.nodes[s.hutNode].kind;
    if (hut != NodeKind::Hut && hut != NodeKind::Start) {
        return false;
    }
    if (!savePoint(s)) {
        return false;
    }
    if (s.stamina == 0 || s.stamina > runnerMaxWithPartners(s).stamina) {
        return false;
    }
    if (s.weather >= static_cast<uint8_t>(Weather::Count) || s.lap > kMaxLap) {
        return false;
    }
    if (s.bossesBeaten >= (1u << kStageCount) || s.cachesTaken >= (1u << kStageCount)) {
        return false;
    }
    const uint32_t allSpecies = Spirit::Count >= 32 ? 0xFFFFFFFFu : ((1u << Spirit::Count) - 1u);
    if ((s.spiritsMet & ~allSpecies) != 0 || (s.spiritsBeaten & ~s.spiritsMet) != 0) {
        return false;
    }
    return true;
}

size_t encode(const RunState& s, char* out, size_t cap)
{
    if (out == nullptr || cap == 0) {
        return 0;
    }
    const int n = std::snprintf(out, cap,
        "tq=%lu\nrng=%lu\nlv=%u\nxp=%u\nst=%u\n"
        "pc=%u\np0=%u\np1=%u\np2=%u\n"
        "bag=%u,%u,%u,%u,%u\n"
        "stage=%u\nnode=%u\nhut=%u\nmin=%u\nwx=%u\nlap=%u\n"
        "boss=%u\ncache=%u\nmet=%lu\nbeat=%lu\nwon=%u\nlost=%u\nperf=%u\n",
        static_cast<unsigned long>(kVersion), static_cast<unsigned long>(s.rng),
        static_cast<unsigned>(s.level), static_cast<unsigned>(s.xp), static_cast<unsigned>(s.stamina),
        static_cast<unsigned>(s.partnerCount), static_cast<unsigned>(s.partners[0]),
        static_cast<unsigned>(s.partners[1]), static_cast<unsigned>(s.partners[2]),
        static_cast<unsigned>(s.bag[0]), static_cast<unsigned>(s.bag[1]), static_cast<unsigned>(s.bag[2]),
        static_cast<unsigned>(s.bag[3]), static_cast<unsigned>(s.bag[4]),
        static_cast<unsigned>(s.stage), static_cast<unsigned>(s.node), static_cast<unsigned>(s.hutNode),
        static_cast<unsigned>(s.minutes), static_cast<unsigned>(s.weather), static_cast<unsigned>(s.lap),
        static_cast<unsigned>(s.bossesBeaten), static_cast<unsigned>(s.cachesTaken),
        static_cast<unsigned long>(s.spiritsMet), static_cast<unsigned long>(s.spiritsBeaten),
        static_cast<unsigned>(s.duelsWon), static_cast<unsigned>(s.duelsLost), static_cast<unsigned>(s.perfects));
    if (n <= 0 || static_cast<size_t>(n) >= cap) {
        out[0] = '\0';
        return 0;
    }
    return static_cast<size_t>(n);
}

bool decode(const char* text, size_t len, RunState& out)
{
    static_assert(Item::Count == 5, "the bag line holds five counts");
    if (text == nullptr) {
        return false;
    }
    uint32_t v[kKeyCount]   = {};
    bool     seen[kKeyCount] = {};
    uint32_t bag[Item::Count] = {};

    size_t start = 0;
    for (size_t i = 0; i <= len; ++i) {
        if (i < len && text[i] != '\n' && text[i] != '\0') {
            continue;
        }
        const char*  line    = text + start;
        size_t       lineLen = i - start;
        start = i + 1;
        if (lineLen > 0 && line[lineLen - 1] == '\r') {
            --lineLen;
        }
        if (lineLen == 0) {
            if (i < len && text[i] == '\0') {
                break;
            }
            continue;
        }
        const char* eq = static_cast<const char*>(std::memchr(line, '=', lineLen));
        if (eq == nullptr) {
            return false;   // not a key=value line: not ours
        }
        const size_t keyLen = static_cast<size_t>(eq - line);
        const char*  val    = eq + 1;
        const size_t valLen = lineLen - keyLen - 1;
        for (uint8_t k = 0; k < kKeyCount; ++k) {
            if (std::strlen(kNames[k]) == keyLen && std::strncmp(kNames[k], line, keyLen) == 0) {
                if (seen[k]) {
                    return false;   // a key twice: not something we wrote
                }
                seen[k] = true;
                const bool ok = k == kBag ? parseBag(val, valLen, bag) : parseU32(val, valLen, v[k]);
                if (!ok) {
                    return false;
                }
                break;
            }
        }
        if (i < len && text[i] == '\0') {
            break;
        }
    }
    for (bool s : seen) {
        if (!s) {
            return false;
        }
    }
    if (v[kTq] != kVersion) {
        return false;
    }

    // Narrowing, checked: a value too big for its field is refused, not cut.
    auto fits8  = [](uint32_t x) { return x <= 0xFFu; };
    auto fits16 = [](uint32_t x) { return x <= 0xFFFFu; };
    static const uint8_t kBytes[] = { kLv, kPc, kP0, kP1, kP2, kStage, kNode, kHut, kWx, kLap, kBoss, kCache };
    static const uint8_t kWords[] = { kXp, kSt, kMin, kWon, kLost, kPerf };
    for (uint8_t k : kBytes) {
        if (!fits8(v[k])) {
            return false;
        }
    }
    for (uint8_t k : kWords) {
        if (!fits16(v[k])) {
            return false;
        }
    }
    for (uint32_t b : bag) {
        if (!fits8(b)) {
            return false;
        }
    }

    RunState s;
    s.rng          = v[kRng];
    s.level        = static_cast<uint8_t>(v[kLv]);
    s.xp           = static_cast<uint16_t>(v[kXp]);
    s.stamina      = static_cast<uint16_t>(v[kSt]);
    s.partnerCount = static_cast<uint8_t>(v[kPc]);
    s.partners[0]  = static_cast<SpeciesId>(v[kP0]);
    s.partners[1]  = static_cast<SpeciesId>(v[kP1]);
    s.partners[2]  = static_cast<SpeciesId>(v[kP2]);
    for (ItemId i = 0; i < Item::Count; ++i) {
        s.bag[i] = static_cast<uint8_t>(bag[i]);
    }
    s.stage         = static_cast<uint8_t>(v[kStage]);
    s.node          = static_cast<uint8_t>(v[kNode]);
    s.hutNode       = static_cast<uint8_t>(v[kHut]);
    s.minutes       = static_cast<uint16_t>(v[kMin]);
    s.weather       = static_cast<uint8_t>(v[kWx]);
    s.lap           = static_cast<uint8_t>(v[kLap]);
    s.bossesBeaten  = static_cast<uint8_t>(v[kBoss]);
    s.cachesTaken   = static_cast<uint8_t>(v[kCache]);
    s.spiritsMet    = v[kMet];
    s.spiritsBeaten = v[kBeat];
    s.duelsWon      = static_cast<uint16_t>(v[kWon]);
    s.duelsLost     = static_cast<uint16_t>(v[kLost]);
    s.perfects      = static_cast<uint16_t>(v[kPerf]);

    if (!valid(s)) {
        return false;
    }
    out = s;
    return true;
}

} // namespace TrailQuest::SaveCodec
