#ifndef USE_SCIP_SOLVER

SCENARIO( "SCIP solver", "[cpsolver]" ) {
  WARN("SCIP solver not tested");
}

#else

#include "scipsolver/test.h"

#endif // USE_SCIP_SOLVER
