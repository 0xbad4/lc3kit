#pragma once

// Parser Classes

#include "lexer.h"
#include <variant>
#include <unordered_set>
#include <limits>
#include <memory>

namespace lc3kit::lasm {
    // Forward declarations
    class Instruction;
    class InsADD;
    class InsAND;
    class InsSHL;
    class InsSHR;
    class InsMUL;
    class InsDIV;
    class InsNOT;
    class InsBR;
    class InsLD;
    class InsLDI;
    class InsLDR;
    class InsLEA;
    class InsST;
    class InsSTI;
    class InsSTR;
    class InsJMP;
    class InsJSR;
    class InsJSRR;
    class InsRET;
    class InsRTI;
    class InsTRAP;
    class InsNOP;
    class InsORIG;
    class InsEND;
    class InsFILL;
    class InsBLKW;
    class InsSTRINGZ;
    
    /**
     * @brief Operand form used by instruction nodes: register, immediate, or label.
     */
    using ins_src2_t = std::variant<std::monostate, registers, std_word_t, str_t>;
    using instruction_ptr = std::unique_ptr<Instruction>;
    using instructions = std::vector<instruction_ptr>;

    #define ins_uptr(X) std::make_unique<X>

    /**
     * @brief Visitor interface for traversing parsed instruction nodes.
     */
    class Visitor {
        public:
            // arithmetic
            virtual void visit(InsADD*)     = 0;
            virtual void visit(InsAND*)     = 0;
            virtual void visit(InsNOT*)     = 0;

            virtual void visit(InsSHL*)     = 0;
            virtual void visit(InsSHR*)     = 0;
            virtual void visit(InsMUL*)     = 0;
            virtual void visit(InsDIV*)     = 0;

            // branch
            virtual void visit(InsBR*)      = 0;

            // load
            virtual void visit(InsLD*)      = 0;
            virtual void visit(InsLDI*)     = 0;
            virtual void visit(InsLDR*)     = 0;
            virtual void visit(InsLEA*)     = 0;

            // store
            virtual void visit(InsST*)      = 0;
            virtual void visit(InsSTI*)     = 0;
            virtual void visit(InsSTR*)     = 0;

            // control flow
            virtual void visit(InsJMP*)     = 0;
            virtual void visit(InsJSR*)     = 0;
            virtual void visit(InsJSRR*)    = 0;
            virtual void visit(InsRET*)     = 0;
            virtual void visit(InsRTI*)     = 0;

            // trap
            virtual void visit(InsTRAP*)    = 0;

            virtual void visit(InsNOP*)    = 0;

            // directives
            virtual void visit(InsORIG*)    = 0;
            virtual void visit(InsEND*)     = 0;
            virtual void visit(InsFILL*)    = 0;
            virtual void visit(InsBLKW*)    = 0;
            virtual void visit(InsSTRINGZ*) = 0;
    };

    // ------------ bases
    class Instruction {
        public:
            const str_t label;   // optional label attached to this instruction
            const tpos  pos;
            std_word_t  address = 0;

            explicit Instruction(str_t lbl = "", tpos p = {}) : label(MV(lbl)), pos(p) {}

            virtual void accept(Visitor*) = 0;
            virtual ~Instruction() = default;
    };

    // ADD, AND + EXT: SHL, SHR, MUL, DIV
    class InsArithmetic : public Instruction {
        public:
            const registers dest_reg;
            const registers src_reg1;
            const ins_src2_t src2;  // register, imm5, label or not set

            InsArithmetic(registers  dst,
                            registers  src1,
                            ins_src2_t s2  = {},
                            str_t      lbl = {},
                            tpos p = {})  // by value, not ref
                : Instruction(MV(lbl), p)
                , dest_reg(dst), src_reg1(src1), src2(MV(s2)) {}

    };

    // NOT  →  DR, SR  (no src2)
    class InsUnary : public Instruction {
        public:
            const registers dest_reg;
            const registers src_reg;

            InsUnary(registers dst, registers src, str_t lbl = "", tpos p = {})
                : Instruction(MV(lbl), p)
                , dest_reg(dst), src_reg(src) {}
    };

    // BR / Jx  →  NZP mask + pcoffset9
    class InsBRBase : public Instruction {
        public:
            const uint8_t       nzp;     // 3-bit condition mask
            const ins_src2_t offset;  // label or immediate

            InsBRBase(uint8_t nzp_, ins_src2_t off, str_t lbl = "", tpos p = {})
                : Instruction(MV(lbl), p) , nzp(nzp_), offset(off) {}
    };

    // LD, LDI, LEA, ST, STI  →  reg, pcoffset9 (label or imm)
    class InsPCOffset9 : public Instruction {
        public:
            const registers dest_reg;
            const ins_src2_t offset;  // label or immediate

            InsPCOffset9(registers   reg,
                        ins_src2_t  off,
                        str_t       lbl = "", 
                        tpos p = {})
                : Instruction(MV(lbl), p)
                , dest_reg(reg), offset(off) {}
    };

    // LDR, STR  →  reg, BaseR, offset6
    class InsBaseOffset : public Instruction {
        public:
            const registers    reg;
            const registers    base_reg;
            const ins_src2_t   offset;   // imm6 or label

            InsBaseOffset(registers  r,
                            registers  base,
                            ins_src2_t off,
                            str_t      lbl = "",
                            tpos p = {})
                : Instruction(MV(lbl), p)
                , reg(r), base_reg(base), offset(off) {}
    };

    // JMP, JSRR  →  BaseR only
    class InsBaseReg : public Instruction {
        public:
            const registers base_reg;

            InsBaseReg(registers base, str_t lbl = "", tpos p = {})
                : Instruction(MV(lbl), p)
                , base_reg(base) {}
    };

    // JSR  →  pcoffset11
    class InsOffset11 : public Instruction {
        public:
            const ins_src2_t offset;

            InsOffset11(ins_src2_t off, str_t lbl = "", tpos p = {})
                : Instruction(MV(lbl), p)
                , offset(off) {}
    };

    // RET, RTI  →  no operands
    class InsNoOperand : public Instruction {
        public:
            explicit InsNoOperand(str_t lbl = "", tpos p = {})
                : Instruction(MV(lbl), p) {}
    };

    // TRAP  →  trapvector8
    class InsTrapBase : public Instruction {
        public:
            const std_word_t vector;

            InsTrapBase(std_word_t vec, str_t lbl = "", tpos p = {})
                : Instruction(MV(lbl), p)
                , vector(vec) {}
    };

    // directives with a single value (.FILL, .BLKW, .ORIG)
    class InsDirectiveValue : public Instruction {
        public:
            const ins_src2_t value;   // immediate or label

            InsDirectiveValue(ins_src2_t val, str_t lbl = "", tpos p = {})
                : Instruction(MV(lbl), p)
                , value(val) {}
    };

    class InsSTRINGZ : public Instruction {
        public:
            const str_t text;

            InsSTRINGZ(str_t str, str_t lbl = "", tpos p = {})
                : Instruction(MV(lbl), p)
                , text(MV(str)) {}

            void accept(Visitor* v) { v->visit(this); }
    };

    // directives
    class InsORIG : public InsDirectiveValue {
        public:
            using InsDirectiveValue::InsDirectiveValue;
            void accept(Visitor* v) { v->visit(this); }
    };

    class InsFILL : public InsDirectiveValue {
        public:
            using InsDirectiveValue::InsDirectiveValue;
            void accept(Visitor* v) { v->visit(this); }
    };

    class InsBLKW : public InsDirectiveValue {
        public:
            using InsDirectiveValue::InsDirectiveValue;
            void accept(Visitor* v) { v->visit(this); }
    };

    class InsEND : public InsNoOperand {
        public:
            using InsNoOperand::InsNoOperand;
            void accept(Visitor* v) { v->visit(this); }
    };

    // ------------ instructions
    class InsADD : public InsArithmetic {
        public:
            using InsArithmetic::InsArithmetic;
            
            void accept(Visitor* vis) {
                vis->visit(this);
            }
    };

    class InsAND : public InsArithmetic {
        public:
            using InsArithmetic::InsArithmetic;
            
            void accept(Visitor* vis) {
                vis->visit(this);
            }
    };

    // lc3kit-ext
    class InsSHL : public InsArithmetic {
        public:
            using InsArithmetic::InsArithmetic;
            
            void accept(Visitor* vis) {
                vis->visit(this);
            }
    };
    
    class InsSHR : public InsArithmetic {
        public:
            using InsArithmetic::InsArithmetic;
            
            void accept(Visitor* vis) {
                vis->visit(this);
            }
    };
    
    class InsMUL : public InsArithmetic {
        public:
            using InsArithmetic::InsArithmetic;
            
            void accept(Visitor* vis) {
                vis->visit(this);
            }
    };

    class InsDIV : public InsArithmetic {
        public:
            using InsArithmetic::InsArithmetic;
            
            void accept(Visitor* vis) {
                vis->visit(this);
            }
    };

    class InsNOT : public InsUnary {
        public:
            using InsUnary::InsUnary;
            
            void accept(Visitor* vis) {
                vis->visit(this);
            }
    };

    class InsBR : public InsBRBase {
        public:
            using InsBRBase::InsBRBase;
            void accept(Visitor* v) { v->visit(this); }
    };

    class InsLD : public InsPCOffset9 {
        public:
            using InsPCOffset9::InsPCOffset9;
            void accept(Visitor* v) { v->visit(this); }
    };

    class InsLDI : public InsPCOffset9 {
        public:
            using InsPCOffset9::InsPCOffset9;
            void accept(Visitor* v) { v->visit(this); }
    };

    class InsLEA : public InsPCOffset9 {
        public:
            using InsPCOffset9::InsPCOffset9;
            void accept(Visitor* v) { v->visit(this); }
    };
    
    class InsLDR : public InsBaseOffset {
        public:
            using InsBaseOffset::InsBaseOffset;
            void accept(Visitor* v) { v->visit(this); }
    };

    // store
    class InsST : public InsPCOffset9 {
        public:
            using InsPCOffset9::InsPCOffset9;
            void accept(Visitor* v) { v->visit(this); }
        };

    class InsSTI : public InsPCOffset9 {
        public:
            using InsPCOffset9::InsPCOffset9;
            void accept(Visitor* v) { v->visit(this); }
    };

    class InsSTR : public InsBaseOffset {
        public:
            using InsBaseOffset::InsBaseOffset;
            void accept(Visitor* v) { v->visit(this); }
    };

    // control flow
    class InsJMP : public InsBaseReg {
        public:
            using InsBaseReg::InsBaseReg;
            void accept(Visitor* v) { v->visit(this); }
    };

    class InsJSR : public InsOffset11 {
        public:
            using InsOffset11::InsOffset11;
            void accept(Visitor* v) { v->visit(this); }
    };

    class InsJSRR : public InsBaseReg {
        public:
            using InsBaseReg::InsBaseReg;
            void accept(Visitor* v) { v->visit(this); }
    };

    class InsRET : public InsNoOperand {
        public:
            using InsNoOperand::InsNoOperand;
            void accept(Visitor* v) { v->visit(this); }
    };

    class InsRTI : public InsNoOperand {
        public:
            using InsNoOperand::InsNoOperand;
            void accept(Visitor* v) { v->visit(this); }
    };

    class InsNOP : public InsNoOperand {
        public:
            using InsNoOperand::InsNoOperand;
            void accept(Visitor* v) override { v->visit(this); }
        };
    
    // trap
    // TRAP + all aliases (GETC, OUT, PUTS, IN, PUTSP, HALT)
    // parser resolves aliases to their vector before constructing
    class InsTRAP : public InsTrapBase {
        public:
            using InsTrapBase::InsTrapBase;
            void accept(Visitor* v) { v->visit(this); }
    };
}
