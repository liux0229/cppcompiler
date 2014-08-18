#include <iostream>

using namespace std;

int main()
{
    // note that the condition could also be syntactically resolved as
    // an assignment expression
    if (int (x) = 1) {
        cout << x << endl;
    }
    
    int (x) = 1;
   
   return 0;
}
