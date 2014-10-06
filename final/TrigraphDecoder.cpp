#include "TrigraphDecoder.h"
#include "common.h"

namespace compiler {

namespace ppToken {

void TrigraphDecoder::put(int c) {
  switch (n_) {
    case 0:
      if (c != '?') {
        send_(c);
      } else {
        ++n_;
      }
      break;
    case 1:
      if (c != '?') {
        send_('?');
        send_(c); // c cannot be the first character
        n_ = 0;
      } else {
        ++n_;
      }
      break;
    case 2: {
      const char* from = R"(=/'()!<>-)";
      const char* to = R"(#\^[]|{}~)";
      bool fnd = false;
      for (int i = 0; from[i]; ++i) {
        if (from[i] == c) {
          fnd = true;
          send_(to[i]);
          n_ = 0;
        }
      }
      if (!fnd) {
        // the first character cannot be part of a trigraph
        send_('?');
        // starting from the second character, put c
        n_ = 1;
        put(c);
      }
      break;
    }
    default:
      // a logical error (TODO: add a message)
      CHECK(false);
      break;
  }
}

} // ppToken

} // compiler
