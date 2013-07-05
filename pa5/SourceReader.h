#pragma once
#include <utility>
#include <vector>
#include <string>

namespace compiler {

class SourceReader
{
public:
  SourceReader(const std::string& source);
  bool get(char& c);
  void include(const std::string& source);
private:
  void open(const std::string& name, std::istream& in);
  const std::string& file() const { return names_.back(); }
  std::string getPathRel(const std::string& source) const;

  std::vector<std::pair<std::string, size_t>> sources_;
  std::vector<std::string> names_;
};

}
