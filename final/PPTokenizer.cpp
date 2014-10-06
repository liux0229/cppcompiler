#include "PPTokenizer.h"
#include "Decoders.h"
#include "common.h"

#include <cctype>
#include <string>

namespace compiler {

namespace ppToken {

using namespace std;

template<typename T>
void PPTokenizer::init()
{
  decoders_.push_back(make_unique<T>());
}

PPTokenizer::PPTokenizer()
{
  init<Utf8Decoder>();
  init<LineSplicer>();
  init<TrigraphDecoder>();
  init<UniversalCharNameDecoder>();
  init<CommentDecoder>();

  int n = static_cast<int>(decoders_.size());
  for (int i = 0; i < n - 1; i++) {
    decoders_[i]->sendTo([this, i](int x) {
      /* cout << format("decoder {} produced `{}`\n", i, char(x)); */
      if ((tokenizer_.insideRawString() &&
        decoders_[i + 1]->turnOffForRawString()) ||
        (tokenizer_.insideQuotedLiteral() &&
        decoders_[i + 1]->turnOffForQuotedLiteral())) {
        receivedChar(x);
      }
      else {
        decoders_[i + 1]->put(x);
      }
    });
  }

  using namespace std::placeholders;
  decoders_.back()->sendTo(bind(&PPTokenizer::receivedChar, this, _1));
  tokenizer_.sendTo(bind(&PPTokenizer::receivedToken, this, _1));
}

void PPTokenizer::process(int c) {
  if (c == EndOfFile) {
    // TODO: handle eliminated newline preceded by a newline
    // and a file ending with '\' (shouldn't add a second newline)
    if (pCh_ != -1 && pCh_ != '\n') {
      decoders_.front()->put('\n');
    }
  }
  decoders_.front()->put(c);
}

void PPTokenizer::receivedChar(int c) {
  pCh_ = c;
  tokenizer_.put(c);
}

bool PPTokenizer::canMergeIntoUserDefined(const PPToken& token) const {
  return pToken_.isQuotedLiteral() &&
    token.type == PPTokenType::Identifier;
}

void PPTokenizer::receivedToken(const PPToken& token) {
  if (canMergeIntoUserDefined(token)) {
    vector<int> data = pToken_.data;
    data.insert(data.end(), token.data.begin(), token.data.end());
    pToken_ = PPToken(pToken_.getUserDefined(), move(data));
  }
  else {
    if (pToken_.isQuotedLiteral()) {
      send_(pToken_);
    }
    pToken_ = token;
  }
  if (!pToken_.isQuotedLiteral()) {
    send_(pToken_);
  }
}

} // ppToken

} // compiler
