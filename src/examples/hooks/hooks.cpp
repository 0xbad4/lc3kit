#include <iostream>
#include <iomanip>
#include <lc3kit/vm>

using namespace lc3kit;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <program.obj>\n";
        return 1;
    }

    vm::VM vm;

    vm::Display display;
    display.set_callback([](char c, void*) { std::cout << c; std::cout.flush(); });
    vm.set_hw_display(&display);

    auto& h = vm.hooks();

    h.on_start = [](vm::VM&) {
        std::cout << "[hook] on_start\n";
    };

    h.before_instruction = [](vm::VM&, uint16_t pc) {
        std::cout << "[hook] before_instruction @ 0x"
                  << std::hex << pc << std::dec << "\n";
    };

    h.after_instruction = [](vm::VM&, uint16_t pc) {
        std::cout << "[hook] after_instruction  @ 0x"
                  << std::hex << pc << std::dec << "\n";
    };

    h.before_memory_read = [](vm::VM&, uint16_t addr) {
        std::cout << "[hook] before_memory_read  addr=0x"
                  << std::hex << addr << std::dec << "\n";
    };

    h.after_memory_read = [](vm::VM&, uint16_t addr, uint16_t val) {
        std::cout << "[hook] after_memory_read   addr=0x"
                  << std::hex << addr << " val=" << val << std::dec << "\n";
    };

    h.before_memory_write = [](vm::VM&, uint16_t addr,
                                uint16_t old_val, uint16_t new_val) {
        std::cout << "[hook] before_memory_write addr=0x"
                  << std::hex << addr
                  << " old=" << old_val << " new=" << new_val
                  << std::dec << "\n";
    };

    h.after_memory_write = [](vm::VM&, uint16_t addr, uint16_t val) {
        std::cout << "[hook] after_memory_write  addr=0x"
                  << std::hex << addr << " val=" << val << std::dec << "\n";
    };

    h.before_register_write = [](vm::VM&, vm::registers reg,
                                  uint16_t old_val, uint16_t new_val) {
        std::cout << "[hook] before_register_write reg="
                  << (int)reg << " old=" << old_val << " new=" << new_val << "\n";
    };

    h.after_register_write = [](vm::VM&, vm::registers reg,
                                 uint16_t val) {
        std::cout << "[hook] after_register_write  reg="
                  << (int)reg << " val=" << val << "\n";
    };

    h.before_trap = [](vm::VM&, uint8_t vec) {
        std::cout << "[hook] before_trap vec=0x"
                  << std::hex << (int)vec << std::dec << "\n";
    };

    h.after_trap = [](vm::VM&, uint8_t vec) {
        std::cout << "[hook] after_trap  vec=0x"
                  << std::hex << (int)vec << std::dec << "\n";
    };

    h.on_halt = [](vm::VM&, std_word_t pc, bool) {
        std::cout << "[hook] on_halt @ 0x"
                  << std::hex << pc << std::dec << "\n";
    };

    h.on_invalid_instruction = [](vm::VM&, uint16_t instr) {
        std::cerr << "[hook] on_invalid_instruction 0x"
                  << std::hex << instr << std::dec << "\n";
    };

    h.on_invalid_memory_access = [](vm::VM&, uint16_t addr, bool is_write) {
        std::cerr << "[hook] on_invalid_memory_access addr=0x"
                  << std::hex << addr << std::dec
                  << (is_write ? " (write)" : " (read)") << "\n";
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

    vm.run();
    return vm.ok() ? 0 : 1;
}
