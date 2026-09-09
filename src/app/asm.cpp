// LC3 assembler CLI test
// usage:
//   lc3kit-asm program.asm             hex dump to stdout
//   lc3kit-asm program.asm -o          binary to a.obj
//   lc3kit-asm program.asm -o out.obj  binary to out.obj
//   lc3kit-asm -i program.asm -o       same with explicit -i

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>

#include <lc3kit/asm>

using namespace lc3kit;

struct CLIArgs {
    std::string input;
    std::string output;   // empty = hex to stdout, "a.obj" = default binary
    bool        to_file  = false;
    bool        ext      = false;
};

static void print_usage(const char* argv0) {
    std::cerr << "Usage:\n"
              << "  " << argv0 << " <file.asm>              hex dump to stdout\n"
              << "  " << argv0 << " <file.asm> -o           binary to a.obj\n"
              << "  " << argv0 << " <file.asm> -o out.obj   binary to out.obj\n"
              << "  " << argv0 << " -i <file.asm> [...]     explicit input flag\n"
              << "  " << argv0 << " ... --ext               enable EXT\n";
}

static bool parse_args(int argc, char* argv[], CLIArgs& out) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-i") {
            if (i + 1 >= argc) {
                std::cerr << "-i requires a filepath\n";
                return false;
            }
            out.input = argv[++i];
        }
        else if (arg == "-o") {
            out.to_file = true;
            // next arg is output path only if it doesn't start with '-'
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                out.output = argv[++i];
            } else {
                out.output = "a.obj";
            }
        }
        else if (arg == "--ext") {
            out.ext = true;
        }
        else if (arg[0] != '-' && out.input.empty()) {
            // bare positional argument treated as input file
            out.input = arg;
        }
        else {
            std::cerr << "unknown argument: " << arg << "\n";
            return false;
        }
    }

    if (out.input.empty()) {
        std::cerr << "no input file specified\n";
        return false;
    }

    return true;
}

static void hex_dump(const lasm::sections_t& sections, std::ostream& out) {
    for (const auto& section : sections) {
        out << ".ORIG 0x" << std::hex << std::uppercase
            << std::setw(4) << std::setfill('0') << section.origin << "\n";

        uint16_t addr = section.origin;

        for (std_word_t word : section.words) {
            out << "  [" << std::setw(4) << std::setfill('0') << addr << "]  "
                << std::setw(4) << std::setfill('0') << word << "  ";
            for (int b = 15; b >= 0; b--) {
                out << ((word >> b) & 1);
                if (b == 12 || b == 9 || b == 6 || b == 3) out << ' ';
            }
            out << "\n";
            addr++;
        }
        out << std::dec << "\n";
    }
}

int main(int argc, char* argv[]) {
    CLIArgs args;
    if (!parse_args(argc, argv, args)) {
        print_usage(argv[0]);
        return 1;
    }

    // open input
    std::ifstream file(args.input);
    if (!file.is_open()) {
        std::cerr << "error: cannot open '" << args.input << "'\n";
        return 1;
    }

    // assemble
    lasm::Asm assembler;
    assembler.set_ext_enabled(args.ext);
    assembler.assemble(file);

    // print all errors
    if (!assembler.ok()) {
        for (const auto& err : assembler.errors()) {
            std::cerr << "error [" << err.pos.line+1 << ":" << err.pos.col+1
                      << "]: " << lasm::err_str(err.type) << "\n";
        }
        return 1;
    }

    // output
    if (args.to_file) {
        std::ofstream out(args.output, std::ios::binary);
        if (!out.is_open()) {
            std::cerr << "error: cannot write to '" << args.output << "'\n";
            return 1;
        }
        assembler.dump(out);
        std::cout << "assembled to " << args.output << "\n";
    } 
    else {
        hex_dump(assembler.sections(), std::cout);
    }

    return 0;
}
