#ifndef BPMNOS_Model_Input_H
#define BPMNOS_Model_Input_H

#include <memory>
#include <string>
#include <unordered_map>
#include <bpmn++.h>

namespace BPMNOS::Model {

/**
 * @brief In-memory inputs for building a data provider without touching the filesystem.
 *
 * Groups the three inputs a data provider needs. The model is handed over as an already-parsed
 * tree (the consumer parses its XML via `XML::XMLObject::createFromString`); the lookup tables and
 * instance are CSV content. Use @ref Model::getLookupTableNames on the parsed model to discover
 * which lookup file names to supply as the keys of @ref lookupTables.
 */
struct Input {
  std::unique_ptr<XML::XMLObject> model;                     ///< Parsed BPMN model tree.
  std::unordered_map<std::string, std::string> lookupTables; ///< Lookup table CSV content keyed by source file name.
  std::string instance;                                      ///< Instance CSV content.
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Input_H
