#pragma once
#include "common.h"

namespace compiler {

enum class StorageClass : uint32_t {
  Unspecified = 0x0,
  Static = 0x1,
  ThreadLocal = 0x2,
  Extern = 0x4,
};
std::ostream& operator<<(std::ostream& out, StorageClass c);

inline StorageClass operator|(StorageClass a, StorageClass b) {
  return static_cast<StorageClass>(
           static_cast<uint32_t>(a) |
           static_cast<uint32_t>(b));
}
inline bool operator&(StorageClass a, StorageClass b) {
  return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) > 0;
}
inline StorageClass operator~(StorageClass c) {
  return static_cast<StorageClass>(~static_cast<uint32_t>(c));
}

enum class Linkage {
  // No,
  Internal,
  External
};
std::ostream& operator<<(std::ostream& out, Linkage linkage);

enum class StorageDuration {
  No,
  Static,
  ThreadLocal
};
std::ostream& operator<<(std::ostream& out, StorageDuration duration);

}
