my_print: 
// stack frame setup
isub64 sp sp 8;
move64 [sp] bp;
move64 bp sp;

// check x == 0
ieq64 x8 [bp + 16] 0;
jumpif x8 my_end_print;

// call print(x/10)
sdiv64 x64 [bp + 16] 10;
isub64 sp sp 8;
move64 [sp] x64;
call my_print;
iadd64 sp sp 8;

// print (x%10)
smod64 x64 [bp + 16] 10;
iadd8 x8 x8 '0';
move8 [0x400001] x8;
syscall3 t64 1 1 0x400001 1;

my_end_print: 
move64 bp [sp];
iadd64 sp sp 8;
ret;
