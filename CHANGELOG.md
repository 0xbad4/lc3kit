# Changelog

## [1.0.0] - Initial release

First public push of the LC3kit project.

### VM

- Full LC-3 ISA implementation
- LC3kit-ext instruction set extension (opcode `0b1101`)
- Privilege model: supervisor/user mode via PSR[15], supervisor stack (SSP/USP) switching on trap entry and RTI
- Trap handling: builtin C++ implementations for GETC, OUT, PUTS, IN, PUTSP,
  HALT with full PSR/PC push-pop on supervisor stack, observable by debuggers
- Two boot modes: `BUILTIN_OS` (C++ trap shortcuts, no OS image required) and
  `CUSTOM_OS` (pure hardware simulator, user loads their own LC-3 OS)
- Hardware abstraction: `Keyboard` (KBSR/KBDR memory-mapped, thread-safe emit)
  and `Display` (DSR/DDR memory-mapped, callback-based output)
- Hook system: lifecycle, per-instruction, memory read/write, register write,
  breakpoint, trap, halt, invalid instruction, invalid memory access hooks
- Breakpoint support: `add_break_point` / `remove_break_point`, fires
  `on_breakpoint` hook with bool return (true=resume, false=pause)
- Step mode: `step()` for single-instruction execution, `run()` for continuous,
  `resume()` via `run()` after breakpoint pause
- Object file loading: multiple `.ORIG` sections supported (requires separate load for each, Execution starts at last section by default.)
- Full error reporting via `error_t` and `get_last_error()`

### Assembler

- Two-pass assembler pipeline: Lexer -> Parser -> SymTabGenerator -> Encoder
- Lexer: all LC-3 token types, hex (`xNN`), decimal (`#N`, bare `N`),
  strings with escape sequences (`\n`, `\t`, `\r`, `\\`, `\"`),
  mnemonic/directive keyword table, multiple error accumulation (`errors_t`)
- Parser: full LC-3 instruction set, all BR variants and J* aliases,
  all TRAP aliases (GETC/OUT/PUTS/IN/PUTSP/HALT), all directives
  (`.ORIG`, `.END`, `.FILL`, `.BLKW`, `.STRINGZ`), label-only lines,
  consecutive label chains via `InsNOP` carrier nodes
- Visitor pattern: `Visitor` with one `visit()` per concrete instruction class
- Symbol table: pass 1 assigns addresses, forward label references resolved in
  pass 2 by the encoder
- Encoder: full instruction encoding against Appendix A bit layouts, PC-relative
  offset computation, range checking on all immediate and offset fields,
  source location carried to encoder for precise error reporting
- LC3kit-ext encoding: `InsSHL`, `InsSHR`, `InsMUL`, `InsDIV`, runtime
  enable/disable via `enabled_ext()`
- Output: `dump(std::ostream&)` emits big-endian binary `.obj`, caller owns I/O
- `Asm` orchestrates the full pipeline with per-stage error collection

### Project

- Header-only library, C++20
- CMake install support with `lc3kit::lc3kit` target and `find_package(lc3kit)` support
- Include via `#include <lc3kit/vm>` and `#include <lc3kit/sm>`

### Notes

- `lc3kit::vm` and `lc3kit::asm` share some type names (`error_type`, `err_str`, etc.) - always use the full namespace qualifier (`vm::error_type`, `asm::error_type`) to avoid ambiguity when both headers are included.

## [1.0.1] - Fix bugs and Added docs

- Assembler: Fixed directives not being treated as case-insensitive.
- VM       : Execution origin now can change if running in STEP mode.
- Docs     : Added LC3kit-ext encoding documentation and live HTML documentation.
