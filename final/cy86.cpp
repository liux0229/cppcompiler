#include "Preprocessor.h"
#include "Cy86Compiler.h"

#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;
using namespace compiler;

namespace {

  struct ElfHeader
  {
    ElfHeader() {
      // Work around VC++ limitation
      unsigned char values[16] {
        0x7f, 'E', 'L', 'F', // magic bytes
          2, // 64-bit architecture
          1, // two's compliment, little-endian
          1, // ELF specification version 1.0
          0, // System V ABI
          0, // ABI Version
          0, 0, 0, 0, 0, 0, 0 // Unused padding
      };
      memcpy(ident, values, sizeof ident);
    }

    unsigned char ident[16];

    short int type = 2; // executable file type
    short int machine = 0x3E; // x86-64 Architecture
    int version = 1; // ELF specification version 1.0
    long int entry; // entry point virtual memory address

    long int phoff = 64; // start of program segment header array file offset
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

    static const int executable = 1 << 0;
    static const int writable = 1 << 1;
    static const int readable = 1 << 2;

    int flags = executable | writable | readable; // segment permissions

    long int offset = 0; // source file offset
    long int vaddr = 0x400000; // destination (virtual) memory address
    long int paddr = 0; // unused, doesn't use physical memory
    long int filesz; // source length
    long int memsz; // destination length
    long int align = 0; // unused, alignment of file/memory
  };

  // bootstrap system call interface, used by RABSetFileExecutable
  extern "C" long int syscall(long int n, ...) throw ();

  // PA9SetFileExecutable: sets file at `path` executable
  // returns true on success
  bool PA9SetFileExecutable(const string& path)
  {
#if WIN32
    int res = 0;
#else
    int res = syscall(/* chmod */ 90, path.c_str(), 0755);
#endif

    return res == 0;
  }

  template<typename T>
  void write(ostream& out, const T& v) {
    out.write(reinterpret_cast<const char*>(&v), sizeof(T));
  }

  void write(ostream& out, const vector<char>& v) {
    out.write(v.data(), v.size());
  }

}

class ProgramGenerator {
public:
  ProgramGenerator(const vector<string>& input, const string& output)
    : input_(input),
    output_(output) {
  }
  void generate() {
    BuildEnv env;
    vector<UToken> tokens;
    for (auto& src : input_) {
      Preprocessor processor(env, src, [&tokens](const Token& token) {
        // TODO: fatal user defined literals (maybe in immediate)
        if (token.isNewLine()) {
          return;
        }
        tokens.push_back(token.copy());
      });
      processor.process();
    }
    vector<char> code;
    size_t start;
    tie(code, start) = Cy86Compiler().compile(move(tokens));
    output(code, start, output_);
  }
private:
  void output(const vector<char>& code, size_t start, const string& output) {
    // const char* rawData = "Assembly Programming Is Fun. LISP.\n";
    // vector<unsigned char> data(rawData, rawData + strlen(rawData));
    ElfHeader elfHeader;
    ProgramSegmentHeader programSegmentHeader;
    elfHeader.entry = static_cast<long>(0x400000 +
      sizeof(ElfHeader) +
      sizeof(ProgramSegmentHeader) +
      start);
    programSegmentHeader.filesz = static_cast<long>(sizeof(ElfHeader)+
      sizeof(ProgramSegmentHeader) +
      code.size());
    programSegmentHeader.memsz = programSegmentHeader.filesz;

    {
      ofstream out(output);
      write(out, elfHeader);
      write(out, programSegmentHeader);
      // write(out, data);
      write(out, code);
    }

    PA9SetFileExecutable(output);
  }

  vector<string> input_;
  string output_;
};

int main(int argc, char** argv) {
  try {
    vector<string> args;

    for (int i = 1; i < argc; i++)
      args.emplace_back(argv[i]);

    if (args.size() < 3 || args[0] != "-o")
      throw logic_error("invalid usage");

    string outfile = args[1];
    size_t nsrcfiles = args.size() - 2;

    vector<string> input;
    for (size_t i = 0; i < nsrcfiles; i++) {
      input.push_back(args[i + 2]);
    }

    ProgramGenerator(input, outfile).generate();

  }
  catch (exception& e) {
    cerr << "ERROR: " << e.what() << endl;
    return EXIT_FAILURE;
  }
}

