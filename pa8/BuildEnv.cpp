#include "BuildEnv.h"
#include "common.h"
#include <ctime>
#include <vector>

namespace compiler {

using namespace std;

BuildEnv::BuildEnv()
{
  time_t result = std::time(NULL);
  string s = asctime(localtime(&result));
  vector<string> v;
  size_t start = 0;
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == ' ') {
      if (i > start) {
        v.push_back(s.substr(start, i - start));
      }
      start = i + 1;
    }
  }
  v.push_back(s.substr(start, s.size() - start));
#if 0
  for (auto& e : v) {
    cout << e << endl;
  }
#endif
  v[4] = v[4].substr(0, v[4].size() - 1);
  date = format("\"{} {} {}\"", v[1], v[2], v[4]);
  time = format("\"{}\"", v[3]);
}

}
