#ifndef BPMNOS_Value_H
#define BPMNOS_Value_H

#include <string>
#include <variant>

namespace BPMNOS {

enum ValueType { STRING, BOOLEAN, INTEGER, DECIMAL };
typedef std::variant< std::string, bool, int, double > Value;

} // BPMNOS

#endif // BPMNOS_Value_H
