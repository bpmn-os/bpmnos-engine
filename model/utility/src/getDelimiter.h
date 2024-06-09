#include <string>
#include <fstream>

inline char getDelimiter(const std::string& instanceFileOrString, size_t lineBreakPosition = std::string::npos) {
  std::string header;
  if ( lineBreakPosition == std::string::npos ) {
    // no line break in instanceFileOrString so it must be a filename
    // determine delimiter from file
    std::ifstream file(instanceFileOrString);
    if (file.is_open()) {
        std::getline(file, header);
        file.close();
    }
    else {
      throw std::runtime_error("Unable to open file " + instanceFileOrString);
    }
  }
  else {
    // determine delimiter from first line of string
    header = instanceFileOrString.substr(0,lineBreakPosition+1);
  }
  
  if ( header.contains(';') ) {
    return ';';
  }
  if ( header.contains(',') ) {
    return ',';
  }
  if ( header.contains('\t') ) {
    return '\t';
  }
  throw std::runtime_error("Unable to determine delimiter for " + instanceFileOrString);
}
