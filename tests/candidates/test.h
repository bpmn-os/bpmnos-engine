// Candidate-collection and resume tests, split into logical units:
//   rebuild/  — candidate collections rebuild correctly from a notice(SystemState)
//   resume/   — a run resumed from an installed (copied) system state terminates as a normal run would
//   scenario/ — the Scenario stores correct completion/arrival status (timestamps) for a task
#include "rebuild/test.h"
#include "resume/test.h"
#include "scenario/test.h"
