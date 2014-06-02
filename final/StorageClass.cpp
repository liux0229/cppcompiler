#include "StorageClass.h"

namespace compiler {

using namespace std;

ostream& operator<<(ostream& out, StorageClass c) {
  static const char names[] = "STE";
  const int n = sizeof(names) / sizeof(*names) - 1;
  auto v = static_cast<uint32_t>(c);
  out << "[";
  for (int i = 0; i < n; ++i) {
    if (v & (1 << i)) {
      out << names[i];
    }
  }
  out << "]";
  return out;
}

ostream& operator<<(ostream& out, Linkage linkage) {
  switch (linkage) {
    // case Linkage::No:
    //  out << "[N]";
    //  break;
    case Linkage::Internal:
      out << "[I]";
      break;
    case Linkage::External:
      out << "[E]";
      break;
  }
  return out;
}

ostream& operator<<(ostream& out, StorageDuration duration) {
  switch (duration) {
    case StorageDuration::No:
      out << "[N]";
      break;
    case StorageDuration::Static:
      out << "[S]";
      break;
    case StorageDuration::ThreadLocal:
      out << "[T]";
      break;
  };
  return out;
}

}
