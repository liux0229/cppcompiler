namespace X1 {
    struct N {};
}

namespace X2 {
    namespace N {
    }
}

int main()
{
    using namespace X1;
    using namespace X2;
    namespace Y = N;
}
