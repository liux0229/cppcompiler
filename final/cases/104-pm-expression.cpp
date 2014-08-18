#include <iostream>

using namespace std;

struct S {
  int f;
};

int main()
{
   int S::*px = &S::f;
   
   S s;
   s.f = 1234;
   void* ps = &s;
   
   // note (S*)ps binds stronger
   // because the grammar is pm-expression ->* cast-expression
   // and cast-expression's grammar is
   // (type-id) unary-expression
   // (i.e. cast-expression cannot contain a pm-expression)
   int r = (S *)ps->*px;
   
   cout << r << endl;
   
   return 0;
}
