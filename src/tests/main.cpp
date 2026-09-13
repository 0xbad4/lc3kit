#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <cstdint>
#include <sstream>

#include <lc3kit/asm>
#include <lc3kit/vm>

using namespace lc3kit;

// NOTE: doctest generates main() via DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN.

// ============================================================================
// Helpers
// ============================================================================

// Assemble a source string and return the binary in a stringstream.
// REQUIREs on assembly failure - each assembler error is attached via INFO()
// first, so a failure prints the full error list instead of just "false".
static std::stringstream assemble(
    const std::string& source,
    bool ext = false
) {
    lasm::source_code src {
        source.c_str(),
        (uint32_t)source.size()
    };

    lasm::Asm asm_;
    asm_.set_ext_enabled(ext);
    asm_.assemble(src);

    for (const auto& err : asm_.errors()) {
        INFO("assembly error at ", err.pos.line, ":", err.pos.col, " - ", lasm::err_str(err.type));
    }

    REQUIRE_MESSAGE(asm_.ok(), "assembly failed - see attached error list above");

    std::ostringstream oss;
    asm_.dump(oss);

    return std::stringstream(oss.str());
}


// Load a stringstream into a VM.
static void vm_load(
    vm::VM& vm,
    std::stringstream& ss
) {
    ss.seekg(0);
    vm.load(ss);

    REQUIRE_MESSAGE(vm.ok(), "VM load failed");
}


// Run to completion and REQUIRE no errors.
static void vm_run(vm::VM& vm) {
    auto err = vm.run();

    REQUIRE_MESSAGE(
        err == vm::error_type::NO_ERROR,
        "VM run() returned an error before completion: " << vm::err_str(err)
    );

    REQUIRE_MESSAGE(
        vm.ok(),
        "VM execution failed: " << vm::err_str(vm.get_last_error())
    );
}


// ============================================================================
// TEST 1 - General LC-3 assembly
//
// Covers:
//   - AND (clear)
//   - ADD (immediate + register)
//   - LD
//   - ST
//   - BR (conditional loop)
//   - Condition codes
//   - Memory write/read round-trip
//   - HALT
// ============================================================================

TEST_CASE("general LC-3 assembly") {

    const std::string src = R"(
        .ORIG x3000

        AND  R3, R3, #0      ; R3 = 0  (accumulator)
        LEA  R1, ARRAY       ; R1 = address of ARRAY
        LD   R2, COUNT       ; R2 = 5  (loop counter)

LOOP    LDR  R4, R1, #0      ; R4 = mem[R1]
        ADD  R3, R3, R4      ; R3 += R4
        ADD  R1, R1, #1      ; R1++
        ADD  R2, R2, #-1     ; R2--
        BRp  LOOP            ; if R2 > 0, loop

        ST   R3, RESULT      ; store sum
        HALT

COUNT   .FILL #5
RESULT  .BLKW #1
ARRAY   .FILL #1
        .FILL #2
        .FILL #3
        .FILL #4
        .FILL #5

        .END
    )";

    auto binary = assemble(src);

    vm::VM vm;

    vm.reset();
    vm.set_ext_enabled(false);

    vm_load(vm, binary);
    vm_run(vm);

    // Layout:
    //
    // x3000: AND
    // x3001: LEA
    // x3002: LD
    // x3003: LDR
    // x3004: ADD
    // x3005: ADD
    // x3006: ADD
    // x3007: BRp
    // x3008: ST
    // x3009: HALT
    // x300A: COUNT
    // x300B: RESULT
    // x300C: ARRAY[0]
    // x300D: ARRAY[1]
    // x300E: ARRAY[2]
    // x300F: ARRAY[3]
    // x3010: ARRAY[4]

    SUBCASE("result is written to memory and to R3") {
        std_word_t result = vm.mem_read(0x300B);

        CHECK(result == 15);
        CHECK(vm.reg_read(vm::registers::R3) == 15);
    }

    SUBCASE("condition code is Z after the loop") {
        // The final ADD R2,R2,#-1 makes R2 = 0 and sets Z.
        // BRp does not branch. ST and HALT do not modify condition codes.
        std_word_t psr = vm.reg_read(vm::registers::PSR);
        std_word_t nzp = psr & 0x7;

        CHECK(nzp == (std_word_t)vm::r_cond::ZRO);
    }
}


// ============================================================================
// TEST 2 - lc3kit-ext instructions
//
// Covers:
//   - SHL (immediate + register mode)
//   - SHR (arithmetic)
//   - MUL
//   - DIV
//   - Condition codes after lc3kit-ext
//   - lc3kit-ext disabled
//   - Division by zero
// ============================================================================

TEST_CASE("lc3kit-ext instructions") {

    SUBCASE("basic arithmetic operations") {
        const std::string src = R"(
            .ORIG x3000

            AND  R0, R0, #0
            ADD  R0, R0, #4     ; R0 = 4

            SHL  R1, R0, #2     ; R1 = 4 << 2 = 16
            SHR  R2, R1, #1     ; R2 = 16 >> 1 = 8  (arithmetic)
            MUL  R3, R0, R2     ; R3 = 4 * 8 = 32
            DIV  R4, R3, R0     ; R4 = 32 / 4 = 8

            ; Test register mode
            SHL  R5, R0, R0     ; R5 = 4 << 4 = 64
            MUL  R6, R2, R2     ; R6 = 8 * 8 = 64

            .END
        )";

        auto binary = assemble(src, true);

        vm::VM vm;

        vm.reset();
        vm.set_ext_enabled(true);

        vm_load(vm, binary);
        vm_run(vm);

        // stupid note removed
        CHECK(vm.reg_read(vm::registers::R1) == 16); // SHL imm: 4 << 2
        CHECK(vm.reg_read(vm::registers::R2) == 8);  // SHR imm: 16 >> 1
        CHECK(vm.reg_read(vm::registers::R3) == 32); // MUL reg: 4 * 8
        CHECK(vm.reg_read(vm::registers::R4) == 8);  // DIV reg: 32 / 4
        CHECK(vm.reg_read(vm::registers::R5) == 64); // SHL reg: 4 << 4
        CHECK(vm.reg_read(vm::registers::R6) == 64); // MUL reg: 8 * 8
    }


    SUBCASE("arithmetic right shift") {
        const std::string src = R"(
            .ORIG x3000

            ; Load -16 into R0
            AND  R0, R0, #0
            ADD  R0, R0, #-16   ; R0 = -16 = 0xFFF0

            SHR  R1, R0, #1     ; R1 = -16 >> 1 = -8
            SHR  R2, R0, #2     ; R2 = -16 >> 2 = -4

            HALT
            .END
        )";

        auto binary = assemble(src, true);

        vm::VM vm;

        vm.reset();
        vm.set_ext_enabled(true);

        vm_load(vm, binary);
        vm_run(vm);

        CHECK((int16_t)vm.reg_read(vm::registers::R1) == -8); // -16 >> 1
        CHECK((int16_t)vm.reg_read(vm::registers::R2) == -4); // -16 >> 2
    }


    SUBCASE("disabled at the VM") {
        const std::string src = R"(
            .ORIG x3000

            AND  R0, R0, #0
            ADD  R0, R0, #4

            SHL  R1, R0, #2     ; lc3kit-ext opcode

            HALT
            .END
        )";

        // still assembled WITH the extension enabled - this subcase is
        // specifically about the VM rejecting it at run time, not about the
        // assembler rejecting the mnemonic.
        auto binary = assemble(src, true);

        vm::VM vm;

        vm.reset();
        vm.set_ext_enabled(false);

        vm_load(vm, binary);
        vm.run();

        REQUIRE(!vm.ok());
        CHECK(vm.get_last_error() == vm::error_type::ILLEGAL_OPCODE);
    }


    SUBCASE("division by zero") {
        const std::string src = R"(
            .ORIG x3000

            AND  R0, R0, #0     ; R0 = 0

            AND  R1, R1, #0
            ADD  R1, R1, #8     ; R1 = 8

            DIV  R2, R1, R0     ; 8 / 0 - should error

            HALT
            .END
        )";

        auto binary = assemble(src, true);

        vm::VM vm;

        vm.reset();
        vm.set_ext_enabled(true);

        vm_load(vm, binary);
        vm.run();

        REQUIRE(!vm.ok());
        CHECK(vm.get_last_error() == vm::error_type::DIVISION_BY_ZERO);
    }
}
