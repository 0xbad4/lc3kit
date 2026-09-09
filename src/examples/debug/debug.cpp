#include <iostream>
#include <iomanip>
#include <lc3kit/vm>

using namespace lc3kit;

void print_state(vm::VM& vm) {
    std::cout << "  PC=0x" << std::hex << std::setw(4) << std::setfill('0')
              << vm.pc() << "  ";
    for (int i = 0; i < 8; i++) {
        std::cout << "R" << std::dec << i << "="
                  << vm.reg_read((vm::registers)i) << " ";
    }
    uint16_t psr = vm.reg_read(vm::registers::PSR);
    std::cout << " N=" << ((psr >> 2) & 1)
              << " Z=" << ((psr >> 1) & 1)
              << " P=" << ((psr >> 0) & 1)
              << "\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <program.obj>\n";
        return 1;
    }

    // STEP mode -- step() is valid, run() would work too but defeats the point
    vm::VM vm(vm::exec_policy::STEP);

    vm::Display display;
    display.set_callback([](char c, void*) { std::cout << c; std::cout.flush(); });
    vm.set_hw_display(&display);

    // set a breakpoint at x3004 (5th instruction)
    vm.add_break_point(0x3004);

    vm.hooks().on_breakpoint = [](vm::VM& vm, uint16_t addr) {
        std::cout << "\n[breakpoint hit @ 0x" << std::hex << addr << "]\n";
        print_state(vm);
        std::cout << "press Enter to continue stepping...\n";
        std::cin.get();
        return true;
    };

    
    vm.hooks().on_halt = [](vm::VM& vm, std_word_t pc, bool f) {
        (void)vm;
        (void)pc;
        (void)f;
        std::cout << "\n[vm] stopped.\n";
    };
    
    std::ifstream file(argv[1], std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "[vm] failed to open file\n";
        return 1;
    }

    vm.load(file);

    if (!vm.ok()) {
        std::cerr << "[vm] failed to load\n";
        return 1;
    }

    std::cout << "stepping through program -- press Enter after each instruction\n\n";

    // for step, its initial state
    vm.run();

    while (vm.is_running()) {
        uint16_t pc = vm.step();
        std::cout << "[stepped to 0x" << std::hex << pc << "]\n";
        print_state(vm);

        if (!vm.ok()) break;  // HALT or error -- don't prompt for another step

        std::cout << "Enter to step > ";
        std::cin.get();
    }


    return vm.ok() ? 0 : 1;
}
