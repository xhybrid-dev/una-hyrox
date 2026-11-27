/**
 ******************************************************************************
 * @file    GlanceView.hpp
 * @brief   What the VO2max glance has to show, read from the Run app's shared
 *          file: pure, host-tested (GlanceViewTest).
 *
 * The Run app writes ../SharedData/HybridX/vo2max.json (Vo2History::
 * toSharedJson, version 2) when it opens and when a run is saved:
 *   {"v":2,"vo2maxX10":523,"runs":5,"utc":1791000000,"beforeX10":519,
 *    "trend":[505,512,519,523]}
 * The trend is the shown figure as it stood after each kept run, oldest
 * first, the last being today's figure. Every value is range-checked; a file
 * that fails any check (or is version 1, without the trend) reads as no data,
 * never a crash.
 ******************************************************************************
 */

#ifndef RUN_GLANCE_VIEW_HPP
#define RUN_GLANCE_VIEW_HPP

#include <cstddef>
#include <cstdint>

#include "Vo2Config.hpp"

namespace RunVo2
{
namespace Glance
{

/// Bars the glance shows: one per kept run.
constexpr uint8_t kGlanceRuns = Config::kHistoryRuns;

struct View {
    bool     hasData   = false;   ///< At least one run with an estimate.
    uint16_t vo2x10    = 0;       ///< The shown figure.
    uint16_t beforeX10 = 0;       ///< The figure before the latest run; 0 = none.
    uint8_t  runs      = 0;       ///< Runs kept (1..kGlanceRuns).
    uint8_t  count     = 0;       ///< Points in trend.
    uint16_t trend[kGlanceRuns] = {};   ///< Oldest first; the last is vo2x10.
};

/// The shared file's path, from the glance's own folder.
constexpr const char* kSharedFile = "../SharedData/HybridX/vo2max.json";

/// Largest file the glance reads (Vo2History::kMaxSharedJsonBytes, with room).
constexpr size_t kMaxFileBytes = 256;

/// Fill @p out from the file's text. Returns out.hasData.
bool makeView(const char* text, size_t len, View& out);

}  // namespace Glance
}  // namespace RunVo2

#endif  // RUN_GLANCE_VIEW_HPP
