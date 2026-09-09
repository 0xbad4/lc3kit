CXX := g++

# The compiler complains about not checking all cases of an enum in a switch.
CXXFLAGS := -std=c++20 -Wall -Wextra -Wno-switch -I../include

OUTPUT_DIR  := ./bin
EXAMPLES_DIR := ./examples


.PHONY: clean init test asm vm


init:
	@mkdir -p $(OUTPUT_DIR)


test: init
	$(CXX) tests/main.cpp $(CXXFLAGS) -o $(OUTPUT_DIR)/test
	@$(OUTPUT_DIR)/test


APP_DIR := ./app

asm: init
	$(CXX) $(APP_DIR)/asm.cpp $(CXXFLAGS) -o $(OUTPUT_DIR)/lc3kit-asm
	@echo "==================="
	@echo "Assembler binary ready @ $(OUTPUT_DIR)/lc3kit-asm"
	@echo "==================="
	@$(OUTPUT_DIR)/lc3kit-asm
	@echo "==================="
	
vm: init
	$(CXX) $(APP_DIR)/vm.cpp $(CXXFLAGS) -o $(OUTPUT_DIR)/lc3kit-vm
	@echo "==================="
	@echo "Assembler binary ready @ $(OUTPUT_DIR)/lc3kit-asm"
	@echo "==================="
	@$(OUTPUT_DIR)/lc3kit-vm
	@echo "==================="


VM_DIR := $(EXAMPLES_DIR)/vm/$(T)
VM_SRC := $(VM_DIR)/$(T)

example: init
	$(CXX) $(VM_SRC).cpp $(CXXFLAGS) -o $(OUTPUT_DIR)/$(T)

	@echo "Emulating ..."
	
	@$(OUTPUT_DIR)/$(T) $(VM_SRC).obj

clean:
	rm -rf $(OUTPUT_DIR)/*