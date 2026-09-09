/*
 * vm.cpp - LC3 VM executable
 *
 * Usage:
 *   lc3kit-vm <program.obj>                      run with builtin OS
 *   lc3kit-vm <program.obj> --cos <os.obj>       run with custom OS
 *   lc3kit-vm <program.obj> --no-ext             disable EXT extension
 *   lc3kit-vm <program.obj> --dump               hex dump memory on stop
 *   lc3kit-vm <program.obj> --cos <os.obj> --no-ext
 */

#include <iostream>
#include <string>
#include <csignal>
#include <thread>
#include <atomic>
#include <iomanip>
#include <lc3kit/vm>

// ----------------- platform

#if defined(__unix__) || defined(__APPLE__)
#   include <termios.h>
#   include <unistd.h>
#   include <sys/select.h>
#   define LC3_UNIX
#elif defined(_WIN32)
#   include <conio.h>
#   include <windows.h>
#   define LC3_WIN32
#endif


using namespace lc3kit;

// ----------------- terminal

#if defined(LC3_UNIX)

static struct termios s_original_tio;

static void disable_input_buffering() {
    tcgetattr(STDIN_FILENO, &s_original_tio);
    struct termios raw = s_original_tio;
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

static void restore_input_buffering() {
    tcsetattr(STDIN_FILENO, TCSANOW, &s_original_tio);
}

#elif defined(LC3_WIN32)

static DWORD s_original_console_mode;

static void disable_input_buffering() {
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(h, &s_original_console_mode);
    SetConsoleMode(h, s_original_console_mode
                      & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT));
}

static void restore_input_buffering() {
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), s_original_console_mode);
}

#endif

// ----------------- signal

static void handle_interrupt(int) {
    restore_input_buffering();
    std::cout << "\n[vm] interrupted\n";
    std::exit(1);
}


// ----------------- keyboard thread

static void kb_thread_fn(vm::vm_keyboard& kb, std::atomic<bool>& running) {
    while (running.load()) {
#if defined(LC3_UNIX)
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        struct timeval tv = {0, 10000};  // 10ms — exits cleanly when vm stops
 
        if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0) {
            char c;
            if (::read(STDIN_FILENO, &c, 1) == 1) {
                kb.emit_char(c);
            }
        }
#elif defined(LC3_WIN32)
        if (_kbhit()) {
            kb.emit_char((char)_getch());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif
    }
}


// ----------------- args

struct Args {
    std::string program;
    std::string custom_os;      // empty if not provided
    bool        no_ext = false;
    bool        dump     = false;
};

static void print_usage(const char* argv0) {
    std::cerr << "Usage:\n"
              << "  " << argv0 << " <program.obj> [options]\n\n"
              << "Options:\n"
              << "  --cos <os.obj>   load a custom OS before the program\n"
              << "  --no-ext         disable EXT instruction set extension\n"
              << "  --dump           hex dump used memory on stop\n";
}

static bool parse_args(int argc, char* argv[], Args& out) {
    if (argc < 2) {
        return false;
    }

    out.program = argv[1];

    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--cos") {
            if (i + 1 >= argc) {
                std::cerr << "[vm] --cos requires a filepath\n";
                return false;
            }
            out.custom_os = argv[++i];
        } else if (arg == "--no-ext") {
            out.no_ext = true;
        } else if (arg == "--dump") {
            out.dump = true;
        } else {
            std::cerr << "[vm] unknown option: " << arg << "\n";
            return false;
        }
    }

    return true;
}


// ----------------- memory dump

static void dump_memory(vm::VM& vm) {
    std::cout << "\n[vm] memory dump (" 
              << vm.get_used_memory_size() << " words used)\n";
    std::cout << std::hex << std::setfill('0');
    for (uint16_t i = 0x3000;
         i < 0x3000 + vm.get_used_memory_size(); i++) {
        std::cout << "  [" << std::setw(4) << i << "]  "
                  << std::setw(4) << vm.mem_read(i) << "\n";
    }
    std::cout << std::dec;
}


// ----------------- hooks

static void setup_hooks(vm::VM& vm, const Args& args) {
    vm.hooks().on_start = [](vm::VM&) {
        std::cout << "[vm] running...\n";
    };

    static bool dump_on_exit = args.dump;

    vm.hooks().on_halt = [](vm::VM& vm, std_word_t, bool) {
        std::cout << "\n[vm] halted";
        if (!vm.ok()) {
            std::cout << " (error: " << vm::err_str(vm.get_last_error()) << ")";
        }
        std::cout << "\n";

        if (dump_on_exit) {
            dump_memory(vm);
        }
    };

    vm.hooks().on_invalid_instruction = [](vm::VM&, uint16_t instr) {
        std::cerr << "[vm] illegal instruction: 0x"
                  << std::hex << instr << std::dec << "\n";
    };

    vm.hooks().on_invalid_memory_access = [](vm::VM&,
                                              uint16_t addr,
                                              bool is_write) {
        std::cerr << "[vm] invalid memory "
                  << (is_write ? "write" : "read")
                  << " @ 0x" << std::hex << addr << std::dec << "\n";
    };
}


// ----------------- main

int main(int argc, char* argv[]) {
    Args args;
    
    if (!parse_args(argc, argv, args)) {
        print_usage(argv[0]);
        return 1;
    }

    signal(SIGINT, handle_interrupt);
    disable_input_buffering();

    vm::VM          vm;
    vm::vm_keyboard keyboard;
    vm::vm_display  display;

    // --- display ---
    display.set_callback([](char c, void*) {
        std::cout << c;
        std::cout.flush();
    });
    vm.set_hw_display(&display);

    // --- keyboard ---
    vm.set_hw_keyboard(&keyboard);

    // --- config ---
    vm.set_ext_enabled(!args.no_ext);

    if (!args.custom_os.empty()) {
        vm.set_boot_mode(vm::boot_mode::CUSTOM_OS);
    }

    // --- hooks ---
    setup_hooks(vm, args);

    // --- load ---
    vm.reset();

    if (!args.custom_os.empty()) {

        std::ifstream file(argv[1], std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[vm] failed to open file\n";
            return 1;
        }

        vm.load(file);

        if (!vm.ok()) {
            std::cerr << "[vm] failed to load OS: "
                      << args.custom_os << "\n";
            restore_input_buffering();
            return 1;
        }
        std::cout << "[vm] OS loaded: " << args.custom_os << "\n";
    }

    std::ifstream file(argv[1], std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "[vm] failed to open file\n";
        return 1;
    }

    vm.load(file);
    if (!vm.ok()) {
        std::cerr << "[vm] failed to load: " << args.program << "\n";
        restore_input_buffering();
        return 1;
    }

    // --- keyboard thread ---
    std::atomic<bool> kb_running = true;
    std::thread kb_thread(kb_thread_fn,
                          std::ref(keyboard),
                          std::ref(kb_running));

    // --- run ---
    vm.run();

    // --- cleanup ---
    kb_running = false;
    kb_thread.join();
    restore_input_buffering();

    return vm.ok() ? 0 : 1;
}
