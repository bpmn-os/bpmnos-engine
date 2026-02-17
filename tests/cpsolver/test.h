#ifndef USE_SCIP_SOLVER

SCENARIO( "SCIP solver", "[cpsolver]" ) {
  WARN("SCIP solver not tested");
}

#else

#include "scipsolver/test.h"

#endif // USE_SCIP_SOLVER

#ifndef USE_HEXALY_SOLVER

SCENARIO( "Hexaly solver", "[cpsolver]" ) {
  WARN("Hexaly solver not tested");
}

#else

#include "hexalysolver/test.h"

#endif // USE_HEXALY_SOLVER
