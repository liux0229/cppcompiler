#include <iostream>

using namespace std;

int main()
{
    int i = 0;
    // note the peculiar for-init-statement
    for (struct S{} s; i < 10; ++i ) { 
        cout << i << endl;
    }
    return 0;
}
