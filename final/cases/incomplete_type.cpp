// Example program
#include <iostream>
#include <string>
#include <memory>

using namespace std;

class T;
void f(std::unique_ptr<T> x) {
    cout << "end of f" << endl;
}
void f(std::shared_ptr<T> x) {}
// void g(T x) {}

class T {
public:
    T() {
        cout << "T()" << endl;
    }
    ~T() {
        cout << "~T()" << endl;
    }
};

int main()
{
  unique_ptr<T> p; //(new T());
  f(std::move(p));
}
