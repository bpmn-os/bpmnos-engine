#ifndef BPMNOS_Engine_H
#define BPMNOS_Engine_H

#include "model/data/src/DataProvider.h"
#include <cnl/all.h>

namespace BPMNOS {

//typedef cnl::scaled_integer< int32_t, cnl::power<-8> > number; // max: 8.4 million, precision: ~ 0.004
typedef cnl::scaled_integer< int64_t, cnl::power<-16> > number; // max: 1.4e14, precision: ~ 0.000015

class Engine {
public:
  Engine(DataProvider* dataProvider);
  DataProvider* dataProvider;
};

} // namespace BPMNOS

#endif // BPMNOS_Engine_H
