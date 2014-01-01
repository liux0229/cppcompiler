#pragma once

#include "common.h"
#include "parsers/ParserCommon.h"
#include "BuildEnv.h"
#include "Preprocessor.h"
#include "TranslationUnit.h"

#include <functional>

namespace compiler {

class Driver {
 public:
  Driver(BuildEnv env, 
         const std::string& sourcePath, 
         const ParserOption& option) 
    : preprocessor_(env, 
                    sourcePath, 
                    std::bind(&Driver::postTokenProcessor, 
                              this,
                              std::placeholders::_1)),
      parserOption_(option) {
  }
  UTranslationUnit process();
  const Namespace* getGlobalNamespace() const;
 private:
  void postTokenProcessor(const PostToken& token);
  std::vector<UToken> tokens_;
  Preprocessor preprocessor_;
  ParserOption parserOption_;
};

}
