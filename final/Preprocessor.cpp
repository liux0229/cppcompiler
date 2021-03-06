#include "Preprocessor.h"
#include "common.h"
#include "preprocessing_token/PPTokenizer.h"
#include "PPDirective.h"

namespace compiler { 

using namespace std;

void Preprocessor::process()
{
  TokenReceiver postTokenReceiver(send_);
  Tokenizer postTokenizer(postTokenReceiver);

  PPDirective ppDirective(bind(&Tokenizer::put, 
                               &postTokenizer,
                               placeholders::_1),
                          buildEnv_,
                          &sourceReader_);

  ppToken::PPTokenizer ppTokenizer;
  ppTokenizer.sendTo(bind(&PPDirective::put,
                     &ppDirective,
                     placeholders::_1));

  for (char c; sourceReader_.get(c); )
  {
    ppTokenizer.process(static_cast<unsigned char>(c));
  }

  ppTokenizer.process(EndOfFile);
}

}
