Provide better error message for cases like this.

const int a[5];
int *p = a;
1 translation units
start translation unit /tmp/fileEDPZTF
check(BF): p pointer to int copy-initializer: IdExpression(a => [variable [E] [S] array of 5 const int]) is-constant: 0
ERROR: cannot initialize p (pointer to int) with IdExpression(a => [variable [E] [S] array of 5 const int])
ERROR: [translationUnit] expect <eof>; got: simple int KW_INT

