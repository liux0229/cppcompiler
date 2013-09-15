#pragma once

#include <string>

namespace compiler {

class PA6 {
public:
  static bool isClassName(const std::string& identifier) {
    return identifier.find('C') != std::string::npos;
  }

  static bool isTemplateName(const std::string& identifier) {
    return identifier.find('T') != std::string::npos;
  }

  static bool isTypedefName(const std::string& identifier) {
    return identifier.find('Y') != std::string::npos;
  }

  static bool isEnumName(const std::string& identifier) {
    return identifier.find('E') != std::string::npos;
  }

  static bool isNamespaceName(const std::string& identifier) {
    return identifier.find('N') != std::string::npos;
  }
};

}
