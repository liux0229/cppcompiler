#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
using namespace std;

const string output = "infinite-loop";

// bootstrap system call interface, used by RABSetFileExecutable
extern "C" long int syscall(long int n, ...) throw ();

// RABSetFileExecutable: sets file at `path` executable
// returns true on success
bool RABSetFileExecutable(const string& path)
{
  int res = syscall(/* chmod */ 90, path.c_str(), 0755);

  return res == 0;
}

struct ElfHeader
{
  unsigned char ident[16] =
  {
    0x7f, 'E', 'L', 'F', // magic bytes
    2, // 64-bit architecture
    1, // two's compliment, little-endian
    1, // ELF specification version 1.0,
    0, // System V ABI
    0, // ABI Version
    0, 0, 0, 0, 0, 0, 0 // Unused padding
  };

  short int type = 2; // executable file type
  short int machine = 0x3E; // x86-64 Architecture
  int version = 1; // ELF specification version 1.0
  long int entry = 0x400078; // entry point virtual memory address

  long int phoff = 0x40; // start of program segment header array file offset
  long int shoff = 0; // no sections

  int processor_flags = 0; // no processor-specific flags
  short int ehsize = 64; // ELF header is 64 bytes long
  short int phentsize = 56; // program header table entry size
  short int phnum = 1; // number of program headers       
  short int shentsize = 0; // no section header table entry size
  short int shnum = 0; // no sections
  short int shstrndx = 0; // no section header string table index
};

struct ProgramSegmentHeader
{
  int type = 1; // PT_LOAD: loadable segment

  static constexpr int executable = 1 << 0;
  static constexpr int writable = 1 << 1;
  static constexpr int readable = 1 << 2;

  int flags = executable | writable | readable; // segment permissions

  long int offset = 0; // source file offset
  long int vaddr = 0x400000; // destination (virtual) memory address
  long int paddr = 0; // unused, doesn't use physical memory
  long int filesz = 122; // source length
  long int memsz = 122; // destination length
  long int align = 0; // unused, alignment of file/memory
};

template<typename T>
void write(ostream& out, const T& s) {
  out.write(reinterpret_cast<const char*>(&s), sizeof(T));
}

void write(ostream& out) {
  ElfHeader elfHeader;
  ProgramSegmentHeader programSegmentHeader;
  write(out, elfHeader);
  write(out, programSegmentHeader);
  out.put(0xEB);
  out.put(0xFE);
}

int main()
{
  {
    ofstream out(output.c_str(), ios_base::out | ios_base::binary);
    write(out);
  }
  assert(RABSetFileExecutable(output));
}
