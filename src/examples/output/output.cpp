#include <iostream>
#include <lc3kit/vm>

using namespace lc3kit;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <program.obj>\n";
        return 1;
    }

    vm::VM vm;

    vm::Display display;
    display.set_callback([](char c, void*) {
        std::cout << c;
        std::cout.flush();
    });
    vm.set_hw_display(&display);

    vm.hooks().on_start = [](vm::VM&) {
        std::cout << "[vm] program started\n";
    };
    
    vm.hooks().on_halt = [](vm::VM&, std_word_t, bool) {
        std::cout << "\n[vm] stopped.\n";
    };

    std::ifstream file(argv[1], std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "[vm] failed to open file\n";
        return 1;
    }

    vm.load(file);
    
    if (!vm.ok()) {
        std::cerr << "[vm] failed to load: " << argv[1] << "\n";
        return 1;
    }

    vm.run();
    return vm.ok() ? 0 : 1;
}
