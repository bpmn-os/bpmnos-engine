#ifndef BPMNOS_Restriction_H
#define BPMNOS_Restriction_H

#include <memory>
#include <vector>
#include <string>
#include <variant>
#include <bpmn++.h>
#include "model/utility/src/Numeric.h"
#include "model/utility/src/StringRegistry.h"
#include "model/parser/src/xml/bpmnos/tRestriction.h"
#include "Attribute.h"

namespace BPMNOS {

class Restriction {
public:
  Restriction(XML::bpmnos::tRestriction* restriction, AttributeMap& attributeMap);
  XML::bpmnos::tRestriction* element;

  std::string& id;
  Attribute* attribute;

  bool negated;
  bool required;
  std::optional<double> minInclusive;
  std::optional<double> maxInclusive;
  std::variant<std::vector<std::string>, std::vector<bool>, std::vector<int>, std::vector<double>> enumeration;

  template <typename T>
  bool isSatisfied(const std::vector<std::optional<T> >& values) const {
    if ( !negated ) {
      // if restriction is not negated
      if ( required && !values[attribute->index].has_value() ) {
        return false;
      }

      // only applies to number types
      if ( attribute->type == Attribute::Type::INTEGER || attribute->type == Attribute::Type::DECIMAL ) {
        if ( minInclusive.has_value() && values[attribute->index].has_value() ) {
          if ( values[attribute->index].get() < numeric<T>(*minInclusive) ) {
            return false;
          }
        }

        if ( maxInclusive.has_value() && values[attribute->index].has_value() ) {
          if ( values[attribute->index].get() > numeric<T>(*maxInclusive) ) {
            return false;
          }
        }
      }
      else {
        throw std::runtime_error("Restriction: illegal attribute type '" + (std::string)attribute->element->type + "' for restriction '" + id + "'");
      }
      
      if ( element->enumeration.size() && values[attribute->index].has_value() ) {
        if ( !find(values[attribute->index].get()) ) {
          return false;
        }
      }
    }
    else {
      // if restriction is negated
      if ( required && values[attribute->index].has_value() ) {
        return false;
      }

      if ( attribute->type == Attribute::Type::INTEGER || attribute->type == Attribute::Type::DECIMAL ) {
        if ( minInclusive.has_value() && values[attribute->index].has_value() ) {
          if ( values[attribute->index].get() >= numeric<T>(*minInclusive) ) {
            return false;
          }
        }

        if ( maxInclusive.has_value() && values[attribute->index].has_value() ) {
          if ( values[attribute->index].get() <= numeric<T>(*maxInclusive) ) {
            return false;
          }
        }
      }
      else {
        throw std::runtime_error("Restriction: illegal attribute type '" + (std::string)attribute->element->type + "' for restriction '" + id + "'");
      }

      if ( element->enumeration.size() && values[attribute->index].has_value() ) {
        if ( find(values[attribute->index].get()) ) {
          return false;
        }
      }

    }
 
    return true;
  }  

  template <typename T>
  bool find(const T& value) const {
    switch ( attribute->type ) {
      case Attribute::Type::STRING :
        for ( std::string& allowedValue : std::get< std::vector< std::string > >(enumeration) ) {
          if ( value == numeric<T>(stringRegistry(allowedValue)) ) {
            return true;
          }
        }
        break;
      case Attribute::Type::BOOLEAN :
        for ( bool allowedValue : std::get< std::vector< bool > >(enumeration) ) {
          if ( value == allowedValue ) {
            return true;
          }
        }
        break;
      case Attribute::Type::INTEGER :
        for ( int allowedValue : std::get< std::vector< int > >(enumeration) ) {
          if ( value == allowedValue ) {
            return true;
          }
        }
        break;
      case Attribute::Type::DECIMAL :
        for ( double allowedValue : std::get< std::vector< double > >(enumeration) ) {
          if ( value == numeric<T>(allowedValue) ) {
            return true;
          }
        }
        break;
    }
    return false;
  }

};

} // namespace BPMNOS

#endif // BPMNOS_Restriction_H
