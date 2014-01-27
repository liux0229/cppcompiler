#include "Preprocessor.h"
#include "common.h"
#include "PPTokenizer.h"
#include "PPDirective.h"

namespace compiler { 

using namespace std;

void Preprocessor::process()
{
  PostTokenReceiver postTokenReceiver(send_);
  PostTokenizer postTokenizer(postTokenReceiver);

  PPDirective ppDirective(bind(&PostTokenizer::put, 
                               &postTokenizer,
                               placeholders::_1),
                          buildEnv_,
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
