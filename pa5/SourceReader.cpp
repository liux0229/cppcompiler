#include "SourceReader.h"
#include "common.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace compiler {

using namespace std;

SourceReader::SourceReader(const string& source)
{
  fstream in(source);
  names_.push_back(source);
  open(source, in);
}

void SourceReader::open(const string& name, istream& in)
{
  ostringstream oss;
  oss << in.rdbuf();
  names_.push_back(name);
  sources_.push_back(make_pair(oss.str(), 0)); 
}

string SourceReader::getPathRel(const string& source) const
{
  auto& f = file();
  auto it = find(f.rbegin(), f.rend(), '/');
  if (it == f.rend()) {
    return "";
  }
  return f.substr(0, it.base() - f.begin()) + source;
}

void SourceReader::include(const string& source)
{
  cout << format("request include {}\n", source);
  string pathRel = getPathRel(source);
  if (ifstream(pathRel)) {
    ifstream in(pathRel);
    open(pathRel, in);
  } else if (ifstream(source)) {
    ifstream in(source);
    open(source, in);
  } else {
    Throw("Cannot open include file {}", source);
  }
}

bool SourceReader::get(char& c)
{
  while (!sources_.empty() && 
         sources_.back().second == sources_.back().first.size()) {
    sources_.pop_back();
    names_.pop_back();
  }
  if (sources_.empty()) {
    return false;
  }
  c = sources_.back().first[sources_.back().second];
  ++sources_.back().second;
  return true;
}

}
