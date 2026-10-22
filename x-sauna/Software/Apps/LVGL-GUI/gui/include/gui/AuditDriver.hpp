/**
 ******************************************************************************
 * @file    AuditDriver.hpp
 * @brief   The simulator's text-fit audit run: hands each screen its worst
 *          case (99:59:59, 65535 bpm, a recovery of +32768, the longest
 *          phase name, 255 rounds), checks every label with Audit::check()
 *          and exits with how many problems there were. Started when the
 *          environment variable XSAUNA_AUDIT is set. Compiled to nothing on
 *          the watch.
 ******************************************************************************
 */

#ifndef AUDIT_DRIVER_HPP
#define AUDIT_DRIVER_HPP

class Model;

namespace AuditDriver
{
#if defined(SIMULATOR)
/// Start the run if XSAUNA_AUDIT is set; does nothing otherwise.
void startIfRequested(Model& model);
#else
inline void startIfRequested(Model&) {}
#endif
} // namespace AuditDriver

#endif // AUDIT_DRIVER_HPP
