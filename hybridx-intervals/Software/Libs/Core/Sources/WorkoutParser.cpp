#include "WorkoutParser.hpp"

#include <cstring>

namespace Intervals
{

namespace
{

constexpr uint8_t  kMaxSkipDepth = 8;      // nesting depth of an unknown value we will skip
constexpr uint32_t kMaxTimeSec   = 86400;  // 24 h
constexpr uint32_t kMaxDistM     = 100000; // 100 km
constexpr uint32_t kMaxRepeats   = 999;
constexpr uint32_t kMaxPaceSec   = 3600;   // sec/km
constexpr uint32_t kMinBpm       = 30;
constexpr uint32_t kMaxBpm       = 250;
constexpr uint32_t kMaxZone      = kMaxHrThresholds;

/// A cursor over the caller's buffer. Every read checks `mPos < mLen`; the
/// first error sticks (later reads become no-ops that keep failing), so the
/// callers can chain reads and test once.
class Reader {
public:
    Reader(const char* buf, size_t len) : mBuf(buf), mLen(len) {}

    bool ok() const { return mError == ParseError::Ok; }
    ParseError error() const { return mError; }
    size_t pos() const { return mPos; }

    bool fail(ParseError e)
    {
        if (mError == ParseError::Ok) {
            mError    = e;
            mErrorPos = mPos;
        }
        return false;
    }
    size_t errorPos() const { return mErrorPos; }

    void skipBom()
    {
        if (mLen >= 3 && static_cast<uint8_t>(mBuf[0]) == 0xEF && static_cast<uint8_t>(mBuf[1]) == 0xBB &&
            static_cast<uint8_t>(mBuf[2]) == 0xBF) {
            mPos = 3;
        }
    }

    void skipWs()
    {
        while (mPos < mLen && (mBuf[mPos] == ' ' || mBuf[mPos] == '\t' || mBuf[mPos] == '\n' || mBuf[mPos] == '\r')) {
            ++mPos;
        }
    }

    bool atEnd()
    {
        skipWs();
        return mPos >= mLen;
    }

    /// The next non-space character without consuming it; 0 at the end.
    char peek()
    {
        skipWs();
        return mPos < mLen ? mBuf[mPos] : '\0';
    }

    bool expect(char c)
    {
        if (!ok()) {
            return false;
        }
        if (peek() != c) {
            return fail(ParseError::Syntax);
        }
        ++mPos;
        return true;
    }

    /// Consumes `c` if it is next; true when it was.
    bool accept(char c)
    {
        if (ok() && peek() == c) {
            ++mPos;
            return true;
        }
        return false;
    }

    /// Reads a string into `dst` (NUL-terminated) and returns its length in
    /// `outLen`. A string longer than cap-1 sets `*overflow` and keeps
    /// scanning (so the syntax is still checked) rather than truncating
    /// silently; the caller decides what overflow means.
    bool readString(char* dst, size_t cap, size_t& outLen, bool& overflow)
    {
        outLen   = 0;
        overflow = false;
        if (!expect('"')) {
            return false;
        }
        while (true) {
            if (mPos >= mLen) {
                return fail(ParseError::Syntax);
            }
            const char c = mBuf[mPos++];
            if (c == '"') {
                break;
            }
            char ch = c;
            if (c == '\\') {
                if (mPos >= mLen) {
                    return fail(ParseError::Syntax);
                }
                const char e = mBuf[mPos++];
                if (e == '"' || e == '\\' || e == '/') {
                    ch = e;
                } else {
                    // \n, \uXXXX and the rest have no place in a workout name.
                    --mPos;
                    return fail(ParseError::BadValue);
                }
            } else if (static_cast<uint8_t>(c) < 0x20) {
                --mPos;
                return fail(ParseError::Syntax);
            }
            if (outLen + 1 < cap) {
                dst[outLen] = ch;
            } else {
                overflow = true;
            }
            ++outLen;
        }
        if (cap > 0) {
            dst[overflow ? cap - 1 : outLen] = '\0';
        }
        return true;
    }

    /// A non-negative integer no larger than `max`. No sign, fraction or
    /// exponent: those are BadValue, not silently rounded.
    bool readUint(uint32_t& v, uint32_t max)
    {
        if (!ok()) {
            return false;
        }
        skipWs();
        if (mPos >= mLen || mBuf[mPos] < '0' || mBuf[mPos] > '9') {
            return fail(mPos < mLen && (mBuf[mPos] == '-' || mBuf[mPos] == '"' ) ? ParseError::BadValue : ParseError::Syntax);
        }
        if (mBuf[mPos] == '0' && mPos + 1 < mLen && mBuf[mPos + 1] >= '0' && mBuf[mPos + 1] <= '9') {
            return fail(ParseError::Syntax);   // leading zero
        }
        uint64_t acc = 0;
        while (mPos < mLen && mBuf[mPos] >= '0' && mBuf[mPos] <= '9') {
            acc = acc * 10 + static_cast<uint32_t>(mBuf[mPos] - '0');
            if (acc > max) {
                return fail(ParseError::BadValue);
            }
            ++mPos;
        }
        if (mPos < mLen && (mBuf[mPos] == '.' || mBuf[mPos] == 'e' || mBuf[mPos] == 'E')) {
            return fail(ParseError::BadValue);
        }
        v = static_cast<uint32_t>(acc);
        return true;
    }

    /// Skips one JSON value of any type (an unknown key's value).
    bool skipValue(uint8_t depth = 0)
    {
        if (!ok()) {
            return false;
        }
        if (depth > kMaxSkipDepth) {
            return fail(ParseError::Syntax);
        }
        const char c = peek();
        if (c == '"') {
            char   scratch[1];
            size_t n;
            bool   over;
            return readString(scratch, 0, n, over);
        }
        if (c == '{' || c == '[') {
            const char close = (c == '{') ? '}' : ']';
            ++mPos;
            if (accept(close)) {
                return true;
            }
            while (ok()) {
                if (c == '{') {
                    char   scratch[1];
                    size_t n;
                    bool   over;
                    if (!readString(scratch, 0, n, over) || !expect(':')) {
                        return false;
                    }
                }
                if (!skipValue(static_cast<uint8_t>(depth + 1))) {
                    return false;
                }
                if (accept(',')) {
                    continue;
                }
                return expect(close);
            }
            return false;
        }
        // A number or a bare word (true, false, null).
        const size_t start = mPos;
        while (mPos < mLen) {
            const char d = mBuf[mPos];
            const bool isWord = (d >= '0' && d <= '9') || (d >= 'a' && d <= 'z') || (d >= 'A' && d <= 'Z') ||
                                d == '-' || d == '+' || d == '.';
            if (!isWord) {
                break;
            }
            ++mPos;
        }
        return mPos > start ? true : fail(ParseError::Syntax);
    }

private:
    const char* mBuf;
    size_t      mLen;
    size_t      mPos      = 0;
    ParseError  mError    = ParseError::Ok;
    size_t      mErrorPos = 0;
};

/// Reads a short enum-like string. A string too long to be any known name
/// comes back as an empty word, which no table matches (so BadValue).
bool readWord(Reader& r, char (&word)[12])
{
    size_t n;
    bool   over;
    if (!r.readString(word, sizeof(word), n, over)) {
        return false;
    }
    if (over) {
        word[0] = '\0';
    }
    return true;
}

bool parseTarget(Reader& r, Target& target)
{
    target = Target {};
    if (!r.expect('{')) {
        return false;
    }
    bool     haveKind = false, haveLow = false, haveHigh = false;
    uint32_t low = 0, high = 0;
    char     kindWord[12] = {};

    if (!r.accept('}')) {
        while (r.ok()) {
            char   key[12];
            size_t n;
            bool   over;
            if (!r.readString(key, sizeof(key), n, over) || !r.expect(':')) {
                return false;
            }
            if (!over && std::strcmp(key, "kind") == 0) {
                if (!readWord(r, kindWord)) {
                    return false;
                }
                haveKind = true;
            } else if (!over && std::strcmp(key, "low") == 0) {
                if (!r.readUint(low, 65535)) {
                    return false;
                }
                haveLow = true;
            } else if (!over && std::strcmp(key, "high") == 0) {
                if (!r.readUint(high, 65535)) {
                    return false;
                }
                haveHigh = true;
            } else if (!r.skipValue()) {
                return false;
            }
            if (r.accept(',')) {
                continue;
            }
            if (!r.expect('}')) {
                return false;
            }
            break;
        }
    }
    if (!r.ok()) {
        return false;
    }
    if (!haveKind) {
        return r.fail(ParseError::MissingField);
    }

    uint32_t minV = 0, maxV = 0;
    if (std::strcmp(kindWord, "open") == 0) {
        target.kind = TargetKind::Open;
        return true;
    } else if (std::strcmp(kindWord, "pace") == 0) {
        target.kind = TargetKind::Pace;
        minV = 1;
        maxV = kMaxPaceSec;
    } else if (std::strcmp(kindWord, "hrzone") == 0) {
        target.kind = TargetKind::HeartRateZone;
        minV = 1;
        maxV = kMaxZone;
    } else if (std::strcmp(kindWord, "hrbpm") == 0) {
        target.kind = TargetKind::HeartRateBpm;
        minV = kMinBpm;
        maxV = kMaxBpm;
    } else {
        return r.fail(ParseError::BadValue);
    }
    if (!haveLow || !haveHigh) {
        return r.fail(ParseError::MissingField);
    }
    if (low < minV || high > maxV || low > high) {
        return r.fail(ParseError::BadValue);
    }
    target.low  = static_cast<uint16_t>(low);
    target.high = static_cast<uint16_t>(high);
    return true;
}

bool parseStep(Reader& r, Step& step)
{
    step = Step {};
    if (!r.expect('{')) {
        return false;
    }
    char     typeWord[12] = {};
    char     intensityWord[12] = {};
    bool     haveType = false, haveValue = false, haveFrom = false, haveCount = false, haveIntensity = false;
    uint32_t value = 0, from = 0, count = 0;
    Target   target {};

    if (!r.accept('}')) {
        while (r.ok()) {
            char   key[12];
            size_t n;
            bool   over;
            if (!r.readString(key, sizeof(key), n, over) || !r.expect(':')) {
                return false;
            }
            if (over) {
                if (!r.skipValue()) {
                    return false;
                }
            } else if (std::strcmp(key, "type") == 0) {
                if (!readWord(r, typeWord)) {
                    return false;
                }
                haveType = true;
            } else if (std::strcmp(key, "value") == 0) {
                // The largest legal value (24 h of seconds or 100 km of metres) is checked once the type is known.
                if (!r.readUint(value, kMaxDistM)) {
                    return false;
                }
                haveValue = true;
            } else if (std::strcmp(key, "intensity") == 0) {
                if (!readWord(r, intensityWord)) {
                    return false;
                }
                haveIntensity = true;
            } else if (std::strcmp(key, "target") == 0) {
                if (!parseTarget(r, target)) {
                    return false;
                }
            } else if (std::strcmp(key, "from") == 0) {
                if (!r.readUint(from, 255)) {
                    return false;
                }
                haveFrom = true;
            } else if (std::strcmp(key, "count") == 0) {
                if (!r.readUint(count, kMaxRepeats)) {
                    return false;
                }
                haveCount = true;
            } else if (!r.skipValue()) {
                return false;
            }
            if (r.accept(',')) {
                continue;
            }
            if (!r.expect('}')) {
                return false;
            }
            break;
        }
    }
    if (!r.ok()) {
        return false;
    }
    if (!haveType) {
        return r.fail(ParseError::MissingField);
    }

    if (haveIntensity) {
        if (std::strcmp(intensityWord, "active") == 0) {
            step.intensity = StepIntensity::Active;
        } else if (std::strcmp(intensityWord, "rest") == 0) {
            step.intensity = StepIntensity::Rest;
        } else if (std::strcmp(intensityWord, "warmup") == 0) {
            step.intensity = StepIntensity::Warmup;
        } else if (std::strcmp(intensityWord, "cooldown") == 0) {
            step.intensity = StepIntensity::Cooldown;
        } else {
            return r.fail(ParseError::BadValue);
        }
    }

    if (std::strcmp(typeWord, "time") == 0) {
        if (!haveValue) {
            return r.fail(ParseError::MissingField);
        }
        if (value < 1 || value > kMaxTimeSec) {
            return r.fail(ParseError::BadValue);
        }
        step.durationType  = DurationKind::Time;
        step.durationValue = value * 1000u;   // seconds -> ms
        step.target        = target;
    } else if (std::strcmp(typeWord, "dist") == 0) {
        if (!haveValue) {
            return r.fail(ParseError::MissingField);
        }
        if (value < 1 || value > kMaxDistM) {
            return r.fail(ParseError::BadValue);
        }
        step.durationType  = DurationKind::Distance;
        step.durationValue = value * 100u;    // metres -> cm
        step.target        = target;
    } else if (std::strcmp(typeWord, "open") == 0) {
        step.durationType  = DurationKind::Open;
        step.durationValue = 0;
        step.target        = target;
    } else if (std::strcmp(typeWord, "repeat") == 0) {
        if (!haveFrom || !haveCount) {
            return r.fail(ParseError::MissingField);
        }
        step.durationType  = DurationKind::RepeatUntilStepsComplete;
        step.durationValue = from;
        step.repeatCount   = static_cast<uint16_t>(count);
        // A repeat marker has no effort of its own: intensity and target stay at their defaults.
        step.intensity = StepIntensity::Active;
        step.target    = Target {};
    } else {
        return r.fail(ParseError::BadValue);
    }
    return true;
}

ParseResult finish(const Reader& r)
{
    ParseResult res;
    res.error  = r.error();
    res.offset = r.ok() ? 0 : r.errorPos();
    return res;
}

} // namespace

ParseResult parseWorkout(const char* buf, size_t len, Workout& out)
{
    ParseResult res;
    if (buf == nullptr && len != 0) {
        res.error = ParseError::Syntax;
        return res;
    }
    if (len > kMaxWorkoutFileBytes) {
        res.error = ParseError::TooLarge;
        return res;
    }

    out = Workout {};
    Reader r(buf, len);
    r.skipBom();
    if (!r.expect('{')) {
        return finish(r);
    }

    bool haveV = false, haveName = false, haveSport = false, haveSteps = false;

    if (!r.accept('}')) {
        while (r.ok()) {
            char   key[12];
            size_t n;
            bool   over;
            if (!r.readString(key, sizeof(key), n, over) || !r.expect(':')) {
                return finish(r);
            }
            if (!over && std::strcmp(key, "v") == 0) {
                uint32_t v = 0;
                if (!r.readUint(v, 65535)) {
                    // A version this parser can't even read as a small number is still the wrong version.
                    if (r.error() == ParseError::BadValue) {
                        res.error = ParseError::BadVersion;
                        res.offset = r.errorPos();
                        return res;
                    }
                    return finish(r);
                }
                if (v != kWorkoutFileVersion) {
                    r.fail(ParseError::BadVersion);
                    return finish(r);
                }
                haveV = true;
            } else if (!over && std::strcmp(key, "name") == 0) {
                size_t nameLen;
                bool   nameOver;
                if (!r.readString(out.name, Workout::kNameChars, nameLen, nameOver)) {
                    return finish(r);
                }
                if (nameOver) {
                    r.fail(ParseError::NameTooLong);
                    return finish(r);
                }
                if (nameLen == 0) {
                    r.fail(ParseError::BadValue);
                    return finish(r);
                }
                haveName = true;
            } else if (!over && std::strcmp(key, "sport") == 0) {
                char word[12] = {};
                if (!readWord(r, word)) {
                    return finish(r);
                }
                if (std::strcmp(word, "run") == 0) {
                    out.sport = Sport::Running;
                } else if (std::strcmp(word, "bike") == 0) {
                    out.sport = Sport::Cycling;
                } else {
                    r.fail(ParseError::BadValue);
                    return finish(r);
                }
                haveSport = true;
            } else if (!over && std::strcmp(key, "steps") == 0) {
                if (haveSteps) {
                    r.fail(ParseError::BadValue);   // "steps" twice
                    return finish(r);
                }
                if (!r.expect('[')) {
                    return finish(r);
                }
                if (!r.accept(']')) {
                    while (r.ok()) {
                        Step step;
                        if (!parseStep(r, step)) {
                            return finish(r);
                        }
                        if (!out.addStep(step)) {
                            r.fail(ParseError::TooManySteps);
                            return finish(r);
                        }
                        if (r.accept(',')) {
                            continue;
                        }
                        if (!r.expect(']')) {
                            return finish(r);
                        }
                        break;
                    }
                }
                haveSteps = true;
            } else if (!r.skipValue()) {
                return finish(r);
            }

            if (r.accept(',')) {
                continue;
            }
            if (!r.expect('}')) {
                return finish(r);
            }
            break;
        }
    }
    if (!r.ok()) {
        return finish(r);
    }
    if (!r.atEnd()) {
        r.fail(ParseError::Syntax);   // anything after the closing brace
        return finish(r);
    }
    if (!haveV || !haveName || !haveSport || !haveSteps) {
        r.fail(ParseError::MissingField);
        return finish(r);
    }

    const ValidationError v = validate(out);
    if (v != ValidationError::Ok) {
        res.error      = ParseError::Invalid;
        res.validation = v;
        return res;
    }
    return res;
}

} // namespace Intervals
