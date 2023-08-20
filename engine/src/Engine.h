#ifndef BPMNOS_Engine_H
#define BPMNOS_Engine_H

#include "model/data/src/DataProvider.h"

namespace BPMNOS {

class Engine {
public:
  Engine(DataProvider* dataProvider);
  DataProvider* dataProvider;
};

} // namespace BPMNOS

#endif // BPMNOS_Engine_H
