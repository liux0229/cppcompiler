constexpr char msg[] = "hello";
// the current implementation correctly determines the type of msg is 
// const char[]. However it would be good to cross reference it with the
// standard, e.g. "applying const to an array means applying const to array
// elements
const char* p = msg;
