#include "Preprocessor.h"
#include "common.h"
#include "PPTokenizer.h"
#include "PPDirective.h"

namespace compiler { 

using namespace std;

void Preprocessor::process()
{
  PostTokenReceiver postTokenReceiver([this](const PostToken& token) {
    if (token.getType() == PostTokenType::Invalid) {
      Throw("invalid token: {}", token.toStr());
    } else if (token.getType() != PostTokenType::NewLine) {
      out_ << token.toStr() << endl;
    }
  });
  PostTokenizer postTokenizer(postTokenReceiver);

  PPDirective ppDirective(bind(&PostTokenizer::put, 
                               &postTokenizer,
                               placeholders::_1),
                          &sourceReader_);

  PPTokenizer ppTokenizer;
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