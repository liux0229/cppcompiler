#include "SourceReader.h"
#include "common.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace compiler {

using namespace std;

namespace {

// bootstrap system call interface, used by PA5GetFileId
extern "C" long int syscall(long int n, ...) throw ();

// PA5GetFileId returns true iff file found at path `path`.
// out parameter `out_fileid` is set to file id
bool PA5GetFileId(const string& path, PA5FileId& out_fileid)
{
	struct
	{
			unsigned long int dev;
			unsigned long int ino;
			long int unused[16];
	} data;

#ifdef WIN32
  // TODO: find equivalent constructs in Windows
  // otherwise any functionality which depends on this
  // (#pragma once)
  // does not work
  int res = 0;
#else
	int res = syscall(4, path.c_str(), &data);
#endif

	out_fileid = make_pair(data.dev, data.ino);

	return res == 0;
}

} // anonymous

SourceReader::SourceReader(const string& source)
{
  open(source);
}

SourceReader::SourceReader(istream& in, const string& source)
{
  open(in, source);
}

void SourceReader::open(const string& name)
{
  PA5FileId id;
  if (PA5GetFileId(name, id)) {
    // cout << format("pragma once check {} {}\n", id.first, id.second);
    if (pragmaOnced_.find(id) != pragmaOnced_.end()) {
      // cout << format("ignore {} because it has been pragma onced\n", name);
      return;
    }
  }
  
  fstream in(name);
  open(in, name);
}

void SourceReader::open(istream& in, const string& name)
{
  ostringstream oss;
  oss << in.rdbuf();
  names_.push_back(name);
  // cout << "push " << name << endl;
  sources_.push_back(make_pair(oss.str(), 0));
  lines_.push_back(1); 
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
  // cout << format("request include {}\n", source);

  string pathRel = getPathRel(source);
  if (ifstream(pathRel)) {
    open(pathRel);
  } else if (ifstream(source)) {
    open(source);
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

    lines_.pop_back();
  }
  if (sources_.empty()) {
    return false;
  }
  c = sources_.back().first[sources_.back().second];
  ++sources_.back().second;

  if (previousIsNewLine_) {
    ++lines_.back();
  }

  previousIsNewLine_ = c == '\n';

  return true;
}

void SourceReader::pragmaOnce(const string& source)
{
  PA5FileId id;
  CHECK(PA5GetFileId(source, id));
  // cout << format("pragma once {} {}\n", id.first, id.second);
  pragmaOnced_.insert(id);
}

}
