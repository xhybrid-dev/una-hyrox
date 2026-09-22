/**
 ******************************************************************************
 * @file    SettingsSerializer.cpp
 * @date    21-09-2026
 * @author  HybridX
 * @brief   Reads and writes the on-watch settings file.
 ******************************************************************************
 *
 * Adapted from the SDK's Running/RunLVGL example, keeping its idiom and its
 * one important habit: a reader getter leaves its output untouched when the
 * key is missing, so every field is given a deterministic value before the
 * read and an older file simply keeps the defaults.
 *
 * AppConfig is the source of truth for the fields the phone can edit; this
 * file also carries them so the watch has something to work from before the
 * config is read, and it is the only home for the last-used race format.
 *
 ******************************************************************************
 */

#include "SettingsSerializer.hpp"

#include <cstdio>
#include <cstring>

#include "SDK/JSON/JsonStreamReader.hpp"
#include "SDK/JSON/JsonStreamWriter.hpp"

#define LOG_MODULE_PRX   "SettingsSer"
#define LOG_MODULE_LEVEL LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

SettingsSerializer::SettingsSerializer(const SDK::Kernel &kernel, const char *pathToFile)
        : mKernel(kernel)
        , mPath(pathToFile)
{
}

bool SettingsSerializer::save(const Settings &settings)
{
    if (mPath == nullptr) {
        return false;
    }

    const char *slash = std::strrchr(mPath, '/');
    if (slash != nullptr) {
        char dir[SDK::Interface::IFileSystem::skMaxPathLen] {};
        std::snprintf(dir, sizeof(dir), "%.*s", static_cast<int>(slash - mPath), mPath);
        if (!mKernel.fs.mkdir(dir)) {
            LOG_ERROR("Cannot create %s\n", dir);
            return false;
        }
    }

    auto file = mKernel.fs.file(mPath);
    if (!file || !file->open(true, true)) {
        LOG_ERROR("Cannot open %s\n", mPath);
        return false;
    }

    SDK::JsonStreamWriter writer(file.get());

    writer.startMap();
    writer.add("version", settings.version);
    writer.add("format", static_cast<uint8_t>(settings.format));
    writer.add("roxzone_splits", settings.roxzoneSplits);
    writer.add("run_distance_m", settings.runDistanceM);
    writer.add("split_lockout_sec", settings.splitLockoutSec);
    writer.add("vibrate_on_split", settings.vibrateOnSplit);
    writer.add("target_finish_min", settings.targetFinishMin);
    writer.endMap();

    file->flush();
    file->close();
    return true;
}

bool SettingsSerializer::load(Settings &settings)
{
    settings = Settings {};  // deterministic defaults before any read

    if (mPath == nullptr) {
        return false;
    }

    auto file = mKernel.fs.file(mPath);
    if (!file || !file->exist() || !file->open()) {
        return false;
    }

    const size_t size = file->size();
    if (size == 0u) {
        file->close();
        return false;
    }

    char *buffer = new (std::nothrow) char[size];
    if (buffer == nullptr) {
        LOG_ERROR("Out of memory reading %s\n", mPath);
        file->close();
        return false;
    }

    size_t read = 0u;
    const bool readOk = file->read(buffer, size, read);
    file->close();

    if (!readOk || read == 0u) {
        delete[] buffer;
        return false;
    }

    SDK::JsonStreamReader reader(buffer, read);
    if (!reader.validate()) {
        LOG_ERROR("Settings JSON is invalid\n");
        delete[] buffer;
        return false;
    }

    reader.get("version", settings.version);

    // Enums are validated on the way in, never blindly cast.
    uint8_t format = 0u;
    if (reader.get("format", format) && format <= static_cast<uint8_t>(Race::Format::HalfB)) {
        settings.format = static_cast<Race::Format>(format);
    }

    reader.get("roxzone_splits", settings.roxzoneSplits);
    reader.get("run_distance_m", settings.runDistanceM);
    reader.get("split_lockout_sec", settings.splitLockoutSec);
    reader.get("vibrate_on_split", settings.vibrateOnSplit);
    reader.get("target_finish_min", settings.targetFinishMin);

    // Clamp what the file claims: it is user-writable over USB.
    if (settings.splitLockoutSec < Settings::kLockoutMinSec ||
        settings.splitLockoutSec > Settings::kLockoutMaxSec) {
        settings.splitLockoutSec = Settings::kLockoutDefaultSec;
    }
    settings.runDistanceM = Race::clampRunDistanceM(settings.runDistanceM);
    if (settings.targetFinishMin > Settings::kTargetFinishMaxMin) {
        settings.targetFinishMin = 0u;
    }

    delete[] buffer;
    return true;
}
