#include <iostream>

using namespace std;

int main()
{
    // note the peculiar for-init-statement
    // and condition (a declaration is used)
    for (struct S{} s; int i = 1234; ) { 
        cout << i << endl;
        break;
    }
    return 0;
}
