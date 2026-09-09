#include <iostream>
#include <thread>
#include <atomic>
#include <csignal>
#include <lc3kit/vm>

#if defined(__unix__) || defined(__APPLE__)
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#endif

#if defined(_WIN32)
#include <conio.h>
#include <windows.h>
#endif

using namespace lc3kit;

// terminal 

#if defined(__unix__) || defined(__APPLE__)

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

#elif defined(_WIN32)

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

// signal handler

static void handle_interrupt(int) {
    restore_input_buffering();
    std::cout << "\n[vm] interrupted\n";
    std::exit(1);
}

// keyboard thread

// blocking read loop in its own thread.
// when a key arrives, emit() writes it into KBSR/KBDR directly.
// the LC-3 program's own KBSR[15] polling loop picks it up naturally.
static void kb_thread_fn(vm::vm_keyboard& kb, std::atomic<bool>& running) {
    while (running.load()) {

        #if defined(__unix__) || defined(__APPLE__)
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(STDIN_FILENO, &fds);
            struct timeval tv = {0, 10000};  // 10ms timeout so thread exits cleanly

            if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0) {
                char c;
                if (::read(STDIN_FILENO, &c, 1) == 1) {
                    if (kb.emit_char(c) != vm::error_type::NO_ERROR) {
                        std::cout << "[vm] broken hardware\n";
                    }
                }
            }
        #elif defined(_WIN32)
            if (_kbhit()) {
                if (kb.emit((char)_getch()) != vm::NO_ERROR) {
                    std::cout << "[vm] broken hardware\n";
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        #endif
    }
}

// setup

static void setup_display(vm::VM& vm, vm::vm_display& display) {
    display.set_callback([](char c, void*) {
        std::cout << c;
        std::cout.flush();
    });
    vm.set_hw_display(&display);
}

static void setup_hooks(vm::VM& vm) {
    vm.hooks().on_halt = [](vm::VM& vm, std_word_t, bool) {
        std::cout << "\n[vm] stopped\n";
        if (!vm.ok())
            std::cerr << "[vm] error: " << vm::err_str(vm.get_last_error()) << "\n";
    };
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <program.obj>\n";
        return 1;
    }

    signal(SIGINT, handle_interrupt);
    disable_input_buffering();

    vm::VM          vm;
    vm::vm_keyboard keyboard;
    vm::vm_display  display;

    vm.set_hw_keyboard(&keyboard);
    setup_display(vm, display);
    setup_hooks(vm);

    std::ifstream file(argv[1], std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "[vm] failed to open file\n";
        return 1;
    }

    vm.load(file);

    if (!vm.ok()) {
        std::cerr << "[vm] failed to load: " << argv[1] << "\n";
        restore_input_buffering();
        return 1;
    }

    std::atomic<bool> kb_running = true;
    std::thread kb_thread(kb_thread_fn, std::ref(keyboard), std::ref(kb_running));

    vm.hooks().on_start = [](vm::VM& vm) {
        std::cout << "[vm] starting @pc=0x" << std::hex << vm.pc() << std::dec << std::endl;
    };

    vm.run();

    kb_running = false;
    kb_thread.join();

    restore_input_buffering();
    return vm.ok() ? 0 : 1;
}
