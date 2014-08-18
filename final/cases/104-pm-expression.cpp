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
   int r = (S *)ps->*px;
   
   cout << r << endl;
   
   return 0;
}
