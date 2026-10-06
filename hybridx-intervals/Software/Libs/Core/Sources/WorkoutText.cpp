#include "WorkoutText.hpp"

#include <cstdio>
#include <cstring>

namespace Intervals::Text
{

namespace
{
size_t finish(char* out, size_t cap, int written)
{
    if (cap == 0) {
        return 0;
    }
    if (written < 0) {
        out[0] = '\0';
        return 0;
    }
    const size_t n = static_cast<size_t>(written);
    return n < cap ? n : cap - 1;
}

/// "1.5", "21.1", "1" (tenths, the ".0" dropped).
size_t tenths(char* out, size_t cap, uint32_t valueTenths)
{
    if (valueTenths % 10u == 0) {
        return finish(out, cap, std::snprintf(out, cap, "%lu", static_cast<unsigned long>(valueTenths / 10u)));
    }
    return finish(out, cap, std::snprintf(out, cap, "%lu.%lu", static_cast<unsigned long>(valueTenths / 10u),
                                          static_cast<unsigned long>(valueTenths % 10u)));
}
} // namespace

size_t clock(char* out, size_t cap, uint32_t seconds)
{
    const uint32_t h = seconds / 3600u;
    const uint32_t m = (seconds / 60u) % 60u;
    const uint32_t s = seconds % 60u;
    if (h > 0) {
        return finish(out, cap, std::snprintf(out, cap, "%lu:%02lu:%02lu", static_cast<unsigned long>(h),
                                              static_cast<unsigned long>(m), static_cast<unsigned long>(s)));
    }
    return finish(out, cap,
                  std::snprintf(out, cap, "%lu:%02lu", static_cast<unsigned long>(m), static_cast<unsigned long>(s)));
}

uint32_t secPerMile(uint32_t secPerKm)
{
    // 1 mile = 1.609344 km.
    return static_cast<uint32_t>((static_cast<uint64_t>(secPerKm) * 1609344u + 500000u) / 1000000u);
}

size_t distance(char* out, size_t cap, uint32_t metres, bool imperial)
{
    if (imperial) {
        // Hundredths of a mile, rounded; trailing zeros dropped.
        const uint32_t h = static_cast<uint32_t>((static_cast<uint64_t>(metres) * 10000u + 80467u) / 160934u);
        int            n;
        if (h % 100u == 0) {
            n = std::snprintf(out, cap, "%lu mi", static_cast<unsigned long>(h / 100u));
        } else if (h % 10u == 0) {
            n = std::snprintf(out, cap, "%lu.%lu mi", static_cast<unsigned long>(h / 100u),
                              static_cast<unsigned long>((h / 10u) % 10u));
        } else {
            n = std::snprintf(out, cap, "%lu.%02lu mi", static_cast<unsigned long>(h / 100u),
                              static_cast<unsigned long>(h % 100u));
        }
        return finish(out, cap, n);
    }
    if (metres < 1000u) {
        return finish(out, cap, std::snprintf(out, cap, "%lu m", static_cast<unsigned long>(metres)));
    }
    char num[16];
    tenths(num, sizeof(num), (metres + 50u) / 100u);
    return finish(out, cap, std::snprintf(out, cap, "%s km", num));
}

const char* intensityWord(StepIntensity intensity)
{
    switch (intensity) {
        case StepIntensity::Warmup:   return "Warm-up";
        case StepIntensity::Rest:     return "Rest";
        case StepIntensity::Cooldown: return "Cool-down";
        case StepIntensity::Active:
        case StepIntensity::Invalid:  break;
    }
    return "Run";
}

size_t duration(char* out, size_t cap, const Step& step, bool imperial)
{
    switch (step.durationType) {
        case DurationKind::Time:
            return clock(out, cap, step.durationValue / 1000u);
        case DurationKind::Distance:
            return distance(out, cap, step.durationValue / 100u, imperial);
        case DurationKind::Open:
        case DurationKind::RepeatUntilStepsComplete:
            break;
    }
    return finish(out, cap, std::snprintf(out, cap, "open"));
}

size_t target(char* out, size_t cap, const Target& t, bool imperial)
{
    switch (t.kind) {
        case TargetKind::Pace: {
            char lo[12];
            char hi[12];
            clock(lo, sizeof(lo), imperial ? secPerMile(t.low) : t.low);
            clock(hi, sizeof(hi), imperial ? secPerMile(t.high) : t.high);
            return finish(out, cap, std::snprintf(out, cap, "%s-%s /%s", lo, hi, imperial ? "mi" : "km"));
        }
        case TargetKind::HeartRateZone:
            if (t.low == t.high) {
                return finish(out, cap, std::snprintf(out, cap, "Zone %u", static_cast<unsigned>(t.low)));
            }
            return finish(out, cap, std::snprintf(out, cap, "Zones %u-%u", static_cast<unsigned>(t.low),
                                                  static_cast<unsigned>(t.high)));
        case TargetKind::HeartRateBpm:
            return finish(out, cap, std::snprintf(out, cap, "%u-%u bpm", static_cast<unsigned>(t.low),
                                                  static_cast<unsigned>(t.high)));
        case TargetKind::Open:
            break;
    }
    return finish(out, cap, std::snprintf(out, cap, "%s", ""));
}

size_t stepLine(char* out, size_t cap, const Step& step, bool imperial)
{
    char d[16];
    duration(d, sizeof(d), step, imperial);
    const char* sep = step.durationType == DurationKind::Open ? ", " : " ";
    return finish(out, cap, std::snprintf(out, cap, "%s%s%s", intensityWord(step.intensity), sep, d));
}

size_t summary(char* out, size_t cap, const WorkoutSummary& sum, bool imperial)
{
    char   parts[3][16] = {};
    size_t n            = 0;
    if (sum.distanceM > 0) {
        distance(parts[n++], sizeof(parts[0]), sum.distanceM, imperial);
    }
    if (sum.timeS > 0) {
        const uint32_t min = (sum.timeS + 59u) / 60u;
        if (min >= 60u) {
            std::snprintf(parts[n++], sizeof(parts[0]), "%lu h %02lu", static_cast<unsigned long>(min / 60u),
                          static_cast<unsigned long>(min % 60u));
        } else {
            std::snprintf(parts[n++], sizeof(parts[0]), "%lu min", static_cast<unsigned long>(min));
        }
    }
    if (sum.openSteps > 0) {
        if (n == 0) {
            std::snprintf(parts[n++], sizeof(parts[0]), "open");
        } else {
            std::snprintf(parts[n++], sizeof(parts[0]), "%u open", static_cast<unsigned>(sum.openSteps));
        }
    }
    if (n == 0) {
        return finish(out, cap, std::snprintf(out, cap, "empty"));
    }
    if (n == 1) {
        return finish(out, cap, std::snprintf(out, cap, "%s", parts[0]));
    }
    if (n == 2) {
        return finish(out, cap, std::snprintf(out, cap, "%s, %s", parts[0], parts[1]));
    }
    return finish(out, cap, std::snprintf(out, cap, "%s, %s, %s", parts[0], parts[1], parts[2]));
}

const char* problem(ParseError error, ValidationError validation)
{
    switch (error) {
        case ParseError::Ok:           return "";
        case ParseError::TooLarge:     return "file too big";
        case ParseError::Syntax:       return "not a workout file";
        case ParseError::BadVersion:   return "newer format: update app";
        case ParseError::MissingField: return "something is missing";
        case ParseError::BadValue:     return "a value is wrong";
        case ParseError::TooManySteps: return "more than 20 steps";
        case ParseError::NameTooLong:  return "name too long";
        case ParseError::Invalid:
            switch (validation) {
                case ValidationError::Empty:        return "no steps";
                case ValidationError::NestedRepeat: return "repeats inside repeats";
                default:                            return "a repeat is wrong";
            }
    }
    return "can't read this file";
}

} // namespace Intervals::Text
