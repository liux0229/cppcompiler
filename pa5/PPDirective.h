#pragma once
#include "PreprocessingToken.h"
#include "MacroProcessor.h"
#include <functional>
#include <vector>

namespace compiler {

class SourceReader;

class PPDirective
{
public:
  PPDirective(std::function<void (const PPToken&)> send,
              SourceReader* sourceReader = nullptr)
    : send_(send),
      sourceReader_(sourceReader) { }
  void put(const PPToken& token);
private:
  void handleDirective();
  void handleExpand();
  bool isEnabled(size_t level) const;
  void handleIf(const std::vector<PPToken>& directive);
  void handleElif(const std::vector<PPToken>& directive);
  void handleElse(const std::vector<PPToken>& directive);
  void handleEndif(const std::vector<PPToken>& directive);
  bool evaluateIf(const std::vector<PPToken>& directive);
  void handleInclude(const std::vector<PPToken>& directive);
  void checkEmpty(const char *cur) const;
  void checkElse(const char* cur) const;

  std::function<void (const PPToken&)> send_;
  std::vector<PPToken> text_;
  std::vector<PPToken> directive_;
  MacroProcessor macroProcessor_;
  int state_ { 0 }; // controls whether we are expecting a directive line or a
                    // text line
  // pos 0 - current
  // pos 1 - true if any if is true
  // pos 2 - whether we have encountered else
  std::vector<int> ifBlock_;

  SourceReader* sourceReader_;
};

}
