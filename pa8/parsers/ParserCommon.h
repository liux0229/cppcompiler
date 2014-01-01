#pragma once
#include "common.h"

namespace compiler {

struct ParserOption
{
  static bool hasSwitch(std::vector<std::string>& args, const char* name)
  {
    auto it = std::find(args.begin(), args.end(), name);
    bool ret = false;
    if (it != args.end()) {
      ret = true;
      args.erase(it);
    }
    return ret;
  }

  bool isTrace { false };
  bool isCollapse { true };
};

}
