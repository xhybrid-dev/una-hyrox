/**
 ******************************************************************************
 * @file    WorkoutStore.cpp
 * @brief   The workouts on the watch (see the header).
 ******************************************************************************
 */

#include "WorkoutStore.hpp"

#include <cstdio>
#include <cstring>

using SDK::Interface::IFileSystem;

namespace Intervals
{

namespace
{
constexpr uint16_t kMaxEntries = 200;   ///< bounds the directory read

bool isJson(const char* name)
{
    const size_t n = std::strlen(name);
    if (n < 6 || name[0] == '.') {   // hidden, including macOS "._name.json"
        return false;
    }
    const char* e = name + n - 5;
    return e[0] == '.' && (e[1] | 0x20) == 'j' && (e[2] | 0x20) == 's' && (e[3] | 0x20) == 'o' &&
           (e[4] | 0x20) == 'n';
}

void copyText(char* out, size_t cap, const char* in)
{
    std::strncpy(out, in, cap - 1);
    out[cap - 1] = '\0';
}

/// Case-insensitive, for the list's order (as Trail's route list).
int compareNames(const char* a, const char* b)
{
    for (;; ++a, ++b) {
        char ca = *a >= 'A' && *a <= 'Z' ? static_cast<char>(*a + 32) : *a;
        char cb = *b >= 'A' && *b <= 'Z' ? static_cast<char>(*b + 32) : *b;
        if (ca != cb || ca == '\0') {
            return static_cast<unsigned char>(ca) - static_cast<unsigned char>(cb);
        }
    }
}
} // namespace

WorkoutStore::WorkoutStore(IFileSystem& fs) : mFs(fs) {}

uint8_t WorkoutStore::scan()
{
    char keep[sizeof(WorkoutInfo::file)] = {};
    if (mSelected >= 0) {
        copyText(keep, sizeof(keep), mCurrentInfo.file);
    }

    mCount     = 0;
    mTruncated = false;
    mFs.mkdir(kDir);
    auto dir = mFs.dir(kDir);
    if (dir && dir->open()) {
        uint16_t seen = 0;
        while (seen < kMaxEntries && dir->readNext(mObject)) {
            ++seen;
            if (mObject.isDir || !isJson(mObject.name) || std::strlen(mObject.name) >= sizeof(WorkoutInfo::file)) {
                continue;
            }
            if (mCount >= kMaxWorkouts) {
                mTruncated = true;
                continue;
            }
            WorkoutInfo& w = mInfos[mCount++];
            w              = WorkoutInfo {};
            copyText(w.file, sizeof(w.file), mObject.name);
        }
        dir->close();
    }

    for (uint8_t i = 0; i < mCount; ++i) {
        readAndParse(mInfos[i].file, mScratch, mInfos[i]);
    }

    // By name: what the athlete reads in the list.
    for (uint8_t i = 1; i < mCount; ++i) {
        const WorkoutInfo moving = mInfos[i];
        uint8_t           j      = i;
        while (j > 0 && compareNames(mInfos[j - 1].name, moving.name) > 0) {
            mInfos[j] = mInfos[j - 1];
            --j;
        }
        mInfos[j] = moving;
    }

    // Keep the loaded workout if its file is still here and runnable. Reload
    // it: the file may have been replaced with a new version.
    mSelected = -1;
    if (keep[0] != '\0') {
        for (uint8_t i = 0; i < mCount; ++i) {
            if (std::strcmp(mInfos[i].file, keep) == 0) {
                load(i);
                break;
            }
        }
    }
    return mCount;
}

bool WorkoutStore::readAndParse(const char* file, Workout& out, WorkoutInfo& info)
{
    ++mParses;
    info.runnable = false;
    // The name until the file says otherwise: "Tempo 5k.json" -> "Tempo 5k".
    copyText(info.name, sizeof(info.name), file);
    const size_t n = std::strlen(info.name);
    if (n >= 5 && info.name[n - 5] == '.') {
        info.name[n - 5] = '\0';
    }

    if (std::snprintf(mPath, sizeof(mPath), "%s/%s", kDir, file) >= static_cast<int>(sizeof(mPath))) {
        info.error = ParseError::Syntax;
        return false;
    }
    auto f = mFs.file(mPath);
    if (!f || !f->open(false, false)) {
        info.error = ParseError::Syntax;
        return false;
    }
    if (f->size() > sizeof(mBuf)) {
        f->close();
        info.error = ParseError::TooLarge;
        return false;
    }
    size_t total = 0;
    size_t got   = 0;
    while (total < sizeof(mBuf) && f->read(mBuf + total, sizeof(mBuf) - total, got) && got > 0) {
        total += got;
    }
    f->close();

    const ParseResult res = parseWorkout(mBuf, total, out);
    info.error            = res.error;
    info.validation       = res.validation;
    info.errorAt          = static_cast<uint32_t>(res.offset);
    if (res.error != ParseError::Ok) {
        return false;
    }
    copyText(info.name, sizeof(info.name), out.name);
    info.sport    = out.sport;
    info.summary  = summarise(out);
    // Running only for now: bike workouts are listed, not run (NOTES P3a).
    info.runnable = (out.sport == Sport::Running);
    return true;
}

bool WorkoutStore::load(uint8_t index)
{
    if (index >= mCount || !mInfos[index].runnable) {
        return false;
    }
    WorkoutInfo info = mInfos[index];
    if (!readAndParse(mInfos[index].file, mCurrent, info) || !info.runnable) {
        mInfos[index] = info;   // it changed since the scan: show why
        clear();
        return false;
    }
    mInfos[index] = info;
    mCurrentInfo  = info;
    mSelected     = static_cast<int8_t>(index);
    writeSelection(info.file);
    return true;
}

bool WorkoutStore::restoreSelection()
{
    auto f = mFs.file(kSelFile);
    if (!f || !f->open(false, false)) {
        return false;
    }
    char   name[sizeof(WorkoutInfo::file)] = {};
    size_t got                             = 0;
    f->read(name, sizeof(name) - 1, got);
    f->close();
    name[got < sizeof(name) ? got : sizeof(name) - 1] = '\0';
    for (char* c = name; *c != '\0'; ++c) {
        if (*c == '\n' || *c == '\r') {
            *c = '\0';
            break;
        }
    }
    for (uint8_t i = 0; i < mCount; ++i) {
        if (std::strcmp(mInfos[i].file, name) == 0) {
            return load(i);
        }
    }
    return false;
}

void WorkoutStore::clear()
{
    mSelected    = -1;
    mCurrent     = Workout {};
    mCurrentInfo = WorkoutInfo {};
}

void WorkoutStore::forget()
{
    clear();
    mFs.remove(kSelFile);
}

void WorkoutStore::writeSelection(const char* file)
{
    auto f = mFs.file(kSelFile);
    if (!f || !f->open(true, true)) {
        return;
    }
    size_t written = 0;
    f->write(file, std::strlen(file), written);
    f->close();
}

} // namespace Intervals
