#pragma once
#include "common.h"
#include <utility>
#include <vector>
#include <string>
#include <set>

namespace compiler {

namespace {
// For pragma once implementation:
// system-wide unique file id type `PA5FileId`
typedef std::pair<unsigned long int, unsigned long int> PA5FileId;
}

class SourceReader
{
public:
  SourceReader(const std::string& source);
  bool get(char& c);
  void include(const std::string& source);
  const std::string& file() const {
    CHECK(!names_.empty());
    return names_.back();
  }
  std::string& file() {
    // CHECK(!names_.empty());
    if (!names_.empty()) {
      return names_.back();
    } else {
      // hack
      return lastName_;
    }
  }
  size_t line() const {
    CHECK(!lines_.empty());
    return lines_.back();
  }
  size_t& line() {
    CHECK(!lines_.empty());
    return lines_.back();
  }
  void pragmaOnce();
private:
  void open(const std::string& name);
  std::string getPathRel(const std::string& source) const;

  std::vector<std::pair<std::string, size_t>> sources_;
  std::vector<std::string> names_;
  std::vector<size_t> lines_;
  bool previousIsNewLine_ { false };
  std::set<PA5FileId> pragmaOnced_;

  // Used for a hack - to know the __FILE__ of _Pragma(...) is quite
  // non-trivial
  std::string lastName_;
};

}
