#include "PredefinedMacros.h"

namespace compiler {

using namespace std;

namespace {
vector<int> toVector(const string& s)
{
  vector<int> r;
  r.reserve(s.size());
  for (char c : s) {
    r.push_back(static_cast<unsigned char>(c));
  }
  return r;
}

}

PredefinedMacros::PredefinedMacros(BuildEnv buildEnv)
{
  macros_.insert(
      make_pair("__CPPGM__", 
                PPToken(PPTokenType::PPNumber, toVector("201303L"))));
  macros_.insert(
      make_pair("__cplusplus", 
                PPToken(PPTokenType::PPNumber, toVector("201103L"))));
  macros_.insert(
      make_pair("__STDC_HOSTED__", 
                PPToken(PPTokenType::PPNumber, toVector("1"))));
  macros_.insert(
      make_pair("__CPPGM_AUTHOR__", 
                PPToken(PPTokenType::StringLiteral, 
                        toVector(R"("Xin Rocky Liu")"))));
  macros_.insert(
      make_pair("__DATE__", 
                PPToken(PPTokenType::StringLiteral, 
                        toVector(buildEnv.date))));
  macros_.insert(
      make_pair("__TIME__", 
                PPToken(PPTokenType::StringLiteral, 
                        toVector(buildEnv.time))));
}

PPToken PredefinedMacros::get(const string& name) const
{
  auto it = macros_.find(name);
  if (it != macros_.end()) {
    return it->second;
  }
  return PPToken {};
}

}
