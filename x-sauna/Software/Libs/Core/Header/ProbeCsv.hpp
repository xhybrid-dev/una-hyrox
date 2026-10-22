/**
 ******************************************************************************
 * @file    ProbeCsv.hpp
 * @brief   The probe's log, as text: a header line, a "# run" line for each
 *          run and one row a second. Pure C++; the formatters write into a
 *          caller's buffer and report 0 when the line does not fit.
 *
 * One file collects every run (`heat-probe.csv` in the app's folder, readable
 * over USB). A run starts with a "#" line, so a spreadsheet or pandas can skip
 * them (comment="#"), and so the runs can be told apart and counted.
 *
 * Columns:
 *   ms          milliseconds since the sensors started
 *   t_s         the same in whole seconds
 *   mark        presses of R1 so far (each one moves to the next phase)
 *   temp_x100   temperature sensor reading, hundredths of its own unit;
 *               blank until a valid sample has arrived
 *   temp_n      valid temperature samples so far
 *   temp_bad    samples the parser rejected
 *   temp_fields fields in the last temperature sample
 *   temp_gap_ms ms between the last two temperature samples (the sensor's own
 *               timestamps); 0 until there are two
 *   hr          heart rate, bpm; blank until a sample has arrived
 *   trust_x100  the heart rate's trust level, hundredths; blank likewise
 *   hr_n        heart-rate samples so far
 *   phase       what R1 has said: 0 ambient (the start), 1 sauna, 2 plunge, 3 rest
 *   hint_delta_x100  the heat hint's smoothed reading minus the first minute's
 *               mean, hundredths; blank until that minute is in
 *   hint_level  0 unknown, 1 cooler, 2 similar, 3 warmer
 *   hint_trend  0 unknown, 1 falling, 2 steady, 3 rising
 ******************************************************************************
 */

#ifndef SAUNA_PROBE_CSV_HPP
#define SAUNA_PROBE_CSV_HPP

#include <cstddef>
#include <cstdint>

#include "FixedPoint.hpp"

namespace Sauna {

struct CsvRow {
    uint32_t tMs = 0;
    uint32_t tS = 0;
    uint16_t mark = 0;
    int32_t  tempX100 = kX100Invalid;
    uint32_t tempN = 0;
    uint32_t tempBad = 0;
    uint16_t tempFields = 0;
    uint32_t tempGapMs = 0;
    uint32_t hrN = 0;
    uint16_t hr = 0;
    int32_t  trustX100 = kX100Invalid;
    uint8_t  phase = 0;
    int32_t  hintDeltaX100 = 0;
    uint8_t  hintLevel = 0;     ///< 0 (unknown) leaves the delta blank
    uint8_t  hintTrend = 0;
};

/// The column names, ending in a newline. 0 if @p cap is too small.
size_t formatCsvHeader(char *out, size_t cap);

/// One row, ending in a newline. 0 if @p cap is too small.
size_t formatCsvRow(char *out, size_t cap, const CsvRow &row);

/// "# run N <stamp>\n": the start of a run. 0 if @p cap is too small.
size_t formatRunLine(char *out, size_t cap, uint16_t run, const char *stamp);

/// "# <text>\n": a note in the log. Newlines in @p text become spaces.
/// 0 if @p cap is too small.
size_t formatNoteLine(char *out, size_t cap, const char *text);

/**
 * @brief   Counts the "# run" lines in a log read in chunks of any size, so
 *          the next run can be numbered. A line that begins with "# run "
 *          counts; the state carries across chunk boundaries.
 */
class RunCounter {
public:
    void feed(const char *data, size_t len);
    uint16_t runs() const { return mRuns; }

private:
    static constexpr char kTag[] = "# run ";
    uint16_t mRuns = 0;
    uint8_t  mMatched = 0;      ///< Characters of kTag matched on this line so far
    bool     mAtLineStart = true;
    bool     mMismatch = false;
};

} // namespace Sauna

#endif // SAUNA_PROBE_CSV_HPP
