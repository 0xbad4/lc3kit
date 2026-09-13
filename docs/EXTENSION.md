# lc3kit-ext

Occupies the reserved opcode slot `0b1101` with four arithmetic/bitwise operations:
SHL, SHR, MUL, DIV.

Disabled by default. Enable on the VM before execution:

```cpp
vm.set_ext_enabled(true);
```

When disabled, any instruction with opcode `0b1101` sets `ILLEGAL_OPCODE`, stops execution,
and fires `on_invalid_instruction`. This applies during both `run()` and `step()`.

---

## Encoding

```
[15:12]  opcode  — 1101 (fixed)
[11:10]  op      — 00=SHL  01=SHR  10=MUL  11=DIV
[9:7]    DR      — destination register
[6:4]    SR1     — first source register
[3]      mode    — 0=immediate  1=register
[2:0]    imm3 / SR2
```

The layout mirrors ADD and AND: DR / SR1 / mode / operand at the same bit positions,
with op-select bits at `[11:10]` replacing the fixed arithmetic function.

---

## Operations

| op bits | Mnemonic | mode=0 (immediate) | mode=1 (register) |
|---------|----------|--------------------|-------------------|
| `00` | SHL | DR = SR1 << SEXT(imm3) | DR = SR1 << SR2 |
| `01` | SHR | DR = SR1 >> SEXT(imm3) | DR = SR1 >> SR2 |
| `10` | MUL | DR = SR1 * SEXT(imm3)  | DR = SR1 * SR2  |
| `11` | DIV | DR = SR1 / SEXT(imm3)  | DR = SR1 / SR2  |

### Immediate mode

`imm3` is a 3-bit value sign-extended to 16 bits before use. Range: `-4` to `+3`.

For constants outside that range, load the value into a register and use register mode.

### SHR

Arithmetic right shift. The sign bit is replicated. Implemented as
`(int16_t)SR1 >> operand` — standard C++ arithmetic shift for signed integers.

### MUL

Produces the low 16 bits of the result. Carry into bit 16 and above is silently discarded.

### DIV

Integer division, truncating toward zero (standard C++ behavior for signed operands).
If the operand is `0`: sets `DIVISION_BY_ZERO`, clears `m_running`, fires
`on_invalid_instruction`. No result is stored.

---

## Condition codes

N/Z/P are updated after every lc3kit-ext instruction, identical to ADD/AND/NOT/LD/LDI/LDR/LEA.
The register hook chain (`before_register_write` / `after_register_write`) fires as usual.

---

## Example encoding

`SHL R2, R0, R1` (register mode, op=`00`):

```
1101  00  010  000  1  001
^^^   ^^  ^^^  ^^^  ^  ^^^
op   sub   DR  SR1  m  SR2
```

Binary: `1101 0001 0000 1001` = `0xD109`

---

## Notes

- The extension does not add new condition flags or exception vectors beyond
  `DIVISION_BY_ZERO` for DIV by zero.
- All lc3kit-ext operations produce 16-bit results; there is no overflow detection for MUL.
- The `imm3` immediate is sign-extended, so `SHL R0, R0, #-1` is equivalent to
  `SHL R0, R0, #0xFFFF` before masking — the shift count is treated as a signed 16-bit
  quantity. Negative shift counts have hardware-defined behavior in C++ and are not
  range-checked by the VM.
