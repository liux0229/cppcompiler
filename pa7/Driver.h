#pragma once

#include "common.h"
#include "BuildEnv.h"
#include "Preprocessor.h"
#include "Namespace.h"

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
  void process();
  const Namespace& getGlobalNs() const;
 private:
  void postTokenProcessor(const PostToken& token);
  std::vector<UToken> tokens_;
  Preprocessor preprocessor_;
  ParserOption parserOption_;

  // dummy
  Namespace globalNs_;
};

}
