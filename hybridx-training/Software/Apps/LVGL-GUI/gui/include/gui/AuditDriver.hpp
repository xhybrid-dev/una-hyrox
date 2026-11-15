/**
 ******************************************************************************
 * @file    AuditDriver.hpp
 * @brief   X Training's text-fit audit run (TP3): with XTRAINING_AUDIT set, the
 *          simulator walks the plan's screens with the widest text each can be
 *          given (every kind of session, with the longest "what" the three
 *          plans make at any level; slow imperial paces; the longest problems),
 *          checks every tracked label (gui/Audit.hpp), prints a line per screen
 *          and exits 1 if anything overflows or was cut. As HybridX Workout's.
 ******************************************************************************
 */

#ifndef AUDIT_DRIVER_HPP
#define AUDIT_DRIVER_HPP

class Model;

namespace AuditDriver
{
#if defined(SIMULATOR)
/// Start the run if XTRAINING_AUDIT is set; does nothing otherwise.
void startIfRequested(Model& model);
#else
inline void startIfRequested(Model&) {}
#endif
} // namespace AuditDriver

#endif // AUDIT_DRIVER_HPP
