#include "StaticInstanceData.h"

using namespace BPMNOS;

StaticInstanceData::StaticInstanceData(const BPMN::Process* process, const std::string& id)
  : InstanceData(process, id) 
{
  actualValues = defaultValues; // may be overwritten by StaticDataProvider
}

Value StaticInstanceData::getPredictedValue(const Attribute* attribute) const {
  return getActualValue(attribute);
}

Value StaticInstanceData::getAssumedValue(const Attribute* attribute) const {
  return getActualValue(attribute);
}
