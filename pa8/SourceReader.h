#pragma once
#include "common.h"
#include <utility>
#include <vector>
#include <string>
#include <set>
#include <istream>

namespace compiler {

namespace {
// For pragma once implementation:
// system-wide unique file id type `PA5FileId`
typedef std::pair<unsigned long int, unsigned long int> PA5FileId;
}

class SourceReader
{
public:
  // for reading from stdin
  SourceReader(std::istream& in, const std::string& source);
  SourceReader(const std::string& source);

  bool get(char& c);
  void include(const std::string& source);
  const std::string& file() const {
    CHECK(!names_.empty());
    return names_.back();
  }
  std::string& file() {
    CHECK(!names_.empty());
    return names_.back();
  }
  size_t line() const {
    CHECK(!lines_.empty());
    return lines_.back();
  }
  size_t& line() {
    CHECK(!lines_.empty());
    return lines_.back();
  }
  void pragmaOnce(const std::string& source);
private:
  void open(const std::string& name);
  void open(std::istream& in, const std::string& name);
  std::string getPathRel(const std::string& source) const;

  std::vector<std::pair<std::string, size_t>> sources_;
  std::vector<std::string> names_;
  std::vector<size_t> lines_;
  bool previousIsNewLine_ { false };
  std::set<PA5FileId> pragmaOnced_;
};

}
