#ifndef BPMNOS_Model_Value_H
#define BPMNOS_Model_Value_H

#include <string>
#include <variant>

namespace BPMNOS {

enum ValueType { BOOLEAN, INTEGER, DECIMAL, STRING, COLLECTION };
typedef std::variant< bool, int, double, std::string > Value;

} // BPMNOS

#endif // BPMNOS_Model_Value_H
