#include <iostream>
#include <lc3kit/vm>

using namespace lc3kit;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <program.obj>\n";
        return 1;
    }

    vm::VM vm;
    vm.set_ext_enabled(true);

    vm.hooks().on_invalid_instruction = [](vm::VM&, uint16_t instr) {
        std::cerr << "[vm] illegal instruction: 0x" << std::hex << instr
                  << " (EXT disabled?)\n" << std::dec;
    };

    vm.hooks().on_halt = [](vm::VM& vm, std_word_t pc, bool f) {
        (void)f;
        (void)pc;
        std::cout << "\n[vm] final registers:\n";

        for (int i = 0; i < 8; i++) {
            auto r = (vm::registers)i;
            std::cout << "  R" << i << " = " << vm.reg_read(r) << "\n";
        }
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
