#include    "cpu.hpp"

#define     OP_BIT(cut, count)      BIT_CUT(opcode, cut, count)

#define     _A                      this->af.as_u8.upper   
#define     _F                      this->af.as_u8.lower  
#define     _SP                     this->sp.as_u16
#define     _PC                     this->pc.as_u16

#define     _ZF                     GET_BIT(this->af.as_u8.lower, 7)
#define     _NF                     GET_BIT(this->af.as_u8.lower, 6)
#define     _HF                     GET_BIT(this->af.as_u8.lower, 5)
#define     _CF                     GET_BIT(this->af.as_u8.lower, 4)

#define     ZF_SET                  SET_BIT(_F, 7)
#define     ZF_CLR                  CLR_BIT(_F, 7)
#define     NF_SET                  SET_BIT(_F, 6)
#define     NF_CLR                  CLR_BIT(_F, 6)
#define     HF_SET                  SET_BIT(_F, 5)
#define     HF_CLR                  CLR_BIT(_F, 5)
#define     CF_SET                  SET_BIT(_F, 4)
#define     CF_CLR                  CLR_BIT(_F, 4)

/*  Takes the operands that are added or subtracted and modifies flags accordingly */
#define     ZF_CHECK(input1, input2)        (TRUNC_ADD(input1, input2) == 0) ? ZF_SET : ZF_CLR
#define     ADD_HF_CHECK(input1, input2)    ((BIT_CUT(input1, 0, 4) + BIT_CUT(input2, 0, 4)) > 0x0F) ? HF_SET : HF_CLR   
#define     SUB_HF_CHECK(input1, input2)    (BIT_CUT(input1, 0, 4) < BIT_CUT(input2, 0, 4)) ? HF_SET : HF_CLR 
#define     ADD_CF_CHECK(input1, input2)    ((input1 + input2) > 0xFF) ? CF_SET : CF_CLR 
#define     SUB_CF_CHECK(input1, input2)    (input1 < input2) ? CF_SET : CF_CLR  

#define     READ_U8_NO_INC(address)         (gb_memory->read_memory(address))
#define     READ_U8(address)                READ_U8_NO_INC(address++)
#define     READ_I8(address)                ((int8_t)READ_U8(address))
#define     WRITE_U8_NO_INC(address, value) gb_memory->write_memory(address, value)

#define     U16_WITH_SYNC(address)          this->scratch.as_u8.lower = READ_U8(address); \
                                            SYNC_TO_CPU_; \
                                            this->scratch.as_u8.upper = READ_U8(address); \
                                            SYNC_TO_CPU_
#define     SYNC_READ_HL_                   SYNC_TO_CPU_; \
                                            this->scratch.as_u8.lower = READ_U8_NO_INC(this->hl.as_u16)
#define     WRITE_HL_                       WRITE_U8_NO_INC(this->hl.as_u16, this->scratch.as_u8.lower)                                                                     


#define     X_(prefix, suffix)          prefix ## 0 ## suffix: \
                                        case prefix ## 1 ## suffix
#define     XX_(prefix, suffix)         prefix ## 00 ## suffix: \
                                        case prefix ## 01 ## suffix: \
                                        case prefix ## 10 ## suffix: \
                                        case prefix ## 11 ## suffix 
#define     XXX_(prefix, suffix)        prefix ## 000 ## suffix: \
                                        case prefix ## 001 ## suffix: \
                                        case prefix ## 010 ## suffix: \
                                        case prefix ## 011 ## suffix: \
                                        case prefix ## 100 ## suffix: \
                                        case prefix ## 101 ## suffix: \
                                        case prefix ## 110 ## suffix: \
                                        case prefix ## 111 ## suffix

#define     SYNC_TO_CPU_                gb_timer->div_increment(); \
                                        this->cycle_count++

                                        /*  implementing the condition lookup table in terms of macros, look at the decoding document for more information */
#define     CONDITION(cond_code)   ((1 - GET_BIT(cond_code, 0)) ^ (BIT_IS_SET(cond_code, 1) ? _CF : _ZF))

/* implementing the register lookup tables in terms of macros, look at the decoding document for more information */
#define     R8_LHALF(reg_code)          TERNARY_4_WAY(reg_code, this->bc.as_u8.upper, this->bc.as_u8.lower, this->de.as_u8.upper, this->de.as_u8.lower)
#define     R8_RHALF(reg_code)          TERNARY_4_WAY(reg_code, this->hl.as_u8.upper, this->hl.as_u8.lower, this->scratch.as_u8.lower, _A)
#define     R8(reg_code)                ((reg_code >= 4) ? R8_RHALF(reg_code - 4) : R8_LHALF(reg_code))
#define     R16_G1(reg_code)            TERNARY_4_WAY(reg_code, this->bc, this->de, this->hl, this->sp) 
#define     R16_G2(reg_code)            TERNARY_4_WAY(reg_code, this->bc.as_u16, this->de.as_u16, this->hl.as_u16++, this->hl.as_u16--) 
#define     R16_G3(reg_code)            TERNARY_4_WAY(reg_code, this->bc, this->de, this->hl, this->af)

#define     REG_PRINT_                  printf("AF = %04x, BC = %04x, DE = %04x, HL = %04x, PC = %04x, SP = %04x\n", this->af.as_u16, this->bc.as_u16, this->de.as_u16, this->de.as_u16, this->pc.as_u16, this->sp.as_u16) 

/*  The values of the CPU registers at game initialization are primarily determined by the boot ROM of the Game Boy in question.
    There can be different combinations, using the combination for boot ROM "DMG". 
    Taken from: https://gbdev.io/pandocs/Power_Up_Sequence.html 
    For this boot ROM, the flags depend on value of header checksum. Ignoring this nitpick for now and just setting both H and C. */
gb_cpu_t::gb_cpu_t(): ro_cycle_count(cycle_count) {
    this->af.as_u8.upper = 0x01;
    this->af.as_u8.lower = 0b10110000;
    this->bc.as_u8.upper = 0x00;
    this->bc.as_u8.lower = 0x13;
    this->de.as_u8.upper = 0x00;
    this->de.as_u8.lower = 0xD8;
    this->hl.as_u8.upper = 0x01;
    this->hl.as_u8.lower = 0x4D;
}

/*  Timings and other information taken from: https://gbdev.io/pandocs/Interrupts.html */
void gb_cpu_t::handle_interrupts(gb_memory_t* gb_memory, gb_timer_t* gb_timer, gb_ioregs_t* gb_ioregs) {
    if(!this->interrupt_master_enable && !this->is_halted) {
        return;
    }

    /*  0x40 -> vBlank 
        0x48 -> STAT
        0x50 -> Timer
        0x58 -> Serial
        0x60 -> Joypad 
        Decreasing priority from up to down. The HALT bug has not been implemented. */
    for(int loop_var = 0; loop_var < 5; loop_var++) {
        if(GET_BIT(gb_ioregs->as_name.ie, loop_var) & GET_BIT(gb_ioregs->as_name.iflag, loop_var)) {
#if ENABLE_INTERRUPT_STEP
            printf("Interupt fired: %02x. Entering single stepping mode.\n", loop_var);
            is_stepping = true;
#endif
            if(this->interrupt_master_enable) {
                this->interrupt_master_enable = false;
                CLR_BIT(gb_ioregs->as_name.iflag, loop_var);
                SYNC_TO_CPU_;
                SYNC_TO_CPU_;
                WRITE_U8_NO_INC(--_SP, this->pc.as_u8.upper);
                SYNC_TO_CPU_;
                WRITE_U8_NO_INC(--_SP, this->pc.as_u8.lower);
                SYNC_TO_CPU_;
                _PC = 0x40 + (0x08 * loop_var);
                SYNC_TO_CPU_;
            }
            this->is_halted = false;
            break;
        }
    }
}

void gb_cpu_t::fetch_and_execute_instruction(gb_memory_t* gb_memory, gb_timer_t* gb_timer) {
    if(this->ime_scheduled == IME_SETTING1) {
        ime_scheduled = IME_SETTING2;
    }
    else if(this->ime_scheduled == IME_SETTING2) {
        this->interrupt_master_enable = true;
        ime_scheduled = IME_UNSET;
    }
    if(this->is_halted) {
        SYNC_TO_CPU_;
        return;
    }

    /*  For (more) accurate system emulation, we need to ensure video/PPU and audio subsystems are in sync with the CPU on an M-cycle level.
        Since instructions take differing amounts of M-cycles, we split each instruction into "micro-ops" which each take 1 M-cycle.
        After each micro-op is emulated, we call the video/PPU and audio subsystems so they can sync up. */
    uint8_t opcode = READ_U8(_PC);
#if ENABLE_OPCODE_BREAKPOINTS
    for(uint8_t loop_var : OPCODE_BREAKPOINTS) {
        if(opcode == loop_var) {
            fprintf(stderr, "Opcode breakpoint reached at %04x, entering single stepping mode.\n", _PC);
            is_stepping = true;
        }
    }
#endif
    /*  since a lot of ALU ops finish in one clock cycle, we can't advance universally after instruction fetch since registers need to be updated
        before that sometimes. A somewhat clunky solution is triggering a cycle after the switch-case, conditionally. */
    if(is_stepping) {
        fprintf(stderr, "OPCODE: %02x\n", opcode);
        REG_PRINT_;
#if     ENABLE_DEBUG_PRINTF
#endif
    }
    bool post_clock_advance = true; 

    /*  This was written with the help of the following instruction decoding guide:
        https://cdn.discordapp.com/attachments/465586075830845475/742438340078469150/SM83_decoding.pdf 
        Timings derived from: https://izik1.github.io/gbops/ 
        Instruction description and further verification done from http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf 
        as well as https://gbdev.io/pandocs/CPU_Instruction_Set.html */
    switch(opcode) {
        case 0x00: { // NOP;
            break;
        }
        case 0x08: { // LD (u16), SP;
            SYNC_TO_CPU_; // instruction fetch
            U16_WITH_SYNC(_PC);
            WRITE_U8_NO_INC(this->scratch.as_u16, this->sp.as_u8.lower);
            SYNC_TO_CPU_;
            WRITE_U8_NO_INC(this->scratch.as_u16 + 1, this->sp.as_u8.upper);
            break;
        }
        /*  https://gbdev.io/pandocs/Reducing_Power_Consumption.html 
            Leaving instruction unimplemented for now. */
        case 0x10: { // STOP;
            /* invalid opcode! */
            printf("STOP! at %04x\n", _PC);
            ERR_EXIT(INVALID_OPCODE_ERROR);
            break;
        }
        /* unconditional jump has similar timings to a conditional jump with branch taken so merging */
        case 0x18: // JR i8; [unconditional]
        case XX_(0b001, 000): { // JR i8; [conditional]
            SYNC_TO_CPU_; // instruction fetch
            int8_t immediate = READ_I8(_PC);
            SYNC_TO_CPU_;
            if((opcode == 0x18) || CONDITION(OP_BIT(3, 2))) {
                _PC = _PC + immediate;         
            }
            else {
                post_clock_advance = false;
            }
            break;
        }
        case XX_(0b00, 0001): { // LD r16, u16;
            SYNC_TO_CPU_; // instruction fetch  
            R16_G1(OP_BIT(4, 2)).as_u8.lower = READ_U8(_PC);
            SYNC_TO_CPU_; 
            R16_G1(OP_BIT(4, 2)).as_u8.upper = READ_U8(_PC);   
            break;
        }
        /*  https://stackoverflow.com/questions/57958631/game-boy-half-carry-flag-and-16-bit-instructions-especially-opcode-0xe8 
            Assuming it is 2 separate ops. HF and CF update twice and HF is derived from the upper byte. */
        case XX_(0b00, 1001): { // ADD HL, r16;
            /* assuming ZF and NF clear on the first sub-op */
            NF_CLR;
            ADD_HF_CHECK(this->hl.as_u8.lower, R16_G1(OP_BIT(4, 2)).as_u8.lower);
            ADD_CF_CHECK(this->hl.as_u8.lower, R16_G1(OP_BIT(4, 2)).as_u8.lower);
            SYNC_TO_CPU_; // first flag computation complete.
            /* proper HF handling with 3 operands at play */
            ADD_HF_CHECK(R16_G1(OP_BIT(4, 2)).as_u8.upper, _CF);
            if(_HF == 0) {
                ADD_HF_CHECK(this->hl.as_u8.upper, R16_G1(OP_BIT(4, 2)).as_u8.upper + _CF);                
            }
            ADD_CF_CHECK(this->hl.as_u8.upper, R16_G1(OP_BIT(4, 2)).as_u8.upper + _CF);
            this->hl.as_u16 = this->hl.as_u16 + R16_G1(OP_BIT(4, 2)).as_u16; // writing result to HL now, second flag computation complete.
            break;
        }
        case XX_(0b00, 0010): { // LD (r16), A;
            SYNC_TO_CPU_; // instruction fetch
            WRITE_U8_NO_INC(R16_G2(OP_BIT(4, 2)), _A);
            break;
        }
        case XX_(0b00, 1010): { // LD A, (r16);
            SYNC_TO_CPU_; // instruction fetch
            _A = READ_U8_NO_INC(R16_G2(OP_BIT(4, 2)));
            break;
        }
        case XX_(0b00, 0011): { // INC r16;
            SYNC_TO_CPU_; // instruction fetch
            R16_G1(OP_BIT(4, 2)).as_u16++;
            break;
        }
        case XX_(0b00, 1011): { // DEC r16;
            SYNC_TO_CPU_; // instruction fetch
            R16_G1(OP_BIT(4, 2)).as_u16--;
            break;
        }
        case XXX_(0b00, 100): { // INC r8;
            /* performing read and write in one expression, so advancing clock beforehand */
            if(OP_BIT(3, 3) == 6) {
                SYNC_READ_HL_;
                SYNC_TO_CPU_;
            }
            ZF_CHECK(R8(OP_BIT(3, 3)), 1);
            NF_CLR;
            ADD_HF_CHECK(R8(OP_BIT(3, 3)), 1);
            R8(OP_BIT(3, 3))++;
            if(OP_BIT(3, 3) == 6) {
                WRITE_HL_;
            }
            break;
        }
        case XXX_(0b00, 101): { // DEC r8;
            /*  performing read and write in one expression, so advancing clock beforehand */
            if(OP_BIT(3, 3) == 6) {
                SYNC_READ_HL_;
                SYNC_TO_CPU_;
            }
            ZF_CHECK(R8(OP_BIT(3, 3)), -1);
            NF_SET;
            SUB_HF_CHECK(R8(OP_BIT(3, 3)), 1); 
            R8(OP_BIT(3, 3))--;
            if(OP_BIT(3, 3) == 6) {
                WRITE_HL_;
            }
            break;
        }
        case XXX_(0b00, 110): { // LD r8, u8;
            SYNC_TO_CPU_;
            if(OP_BIT(3, 3) == 6) {
                SYNC_TO_CPU_;
            }
            R8(OP_BIT(3, 3)) = READ_U8(_PC);
            if(OP_BIT(3, 3) == 6) {
                WRITE_HL_;
            }
            break;
        }
        case X_(0b000, 0111): { // RLCA; RLA;
            uint8_t high_bit = GET_BIT(_A, 7);
            _A = (_A << 1) | ((OP_BIT(4, 1) == 1) ? _CF : high_bit);
            ZF_CLR;
            NF_CLR;
            HF_CLR;
            (high_bit == 1) ? CF_SET : CF_CLR; 
            break;
        }
        case X_(0b000, 1111): { // RRCA; RRA;
            uint8_t low_bit = GET_BIT(_A, 0);
            _A = (_A >> 1) | (((OP_BIT(4, 1) == 1) ? _CF : low_bit) << 7);
            ZF_CLR;
            NF_CLR;
            HF_CLR;
            (low_bit == 1) ? CF_SET : CF_CLR; 
            break;
        }
        /* Used this for reference: https://forums.nesdev.org/viewtopic.php?t=15944 */
        case 0x27: { // DAA;
            if(!_NF) {
                if(_CF || (_A > 0x99)) {
                    _A += 0x60;
                    CF_SET;
                }
                if(_HF || BIT_CUT(_A, 0, 4) > 0X09) {
                    _A += 0X06;
                }
            }
            else {
                if(_CF) {
                    _A -= 0x60;
                }
                if(_HF) {
                    _A -= 0x06;
                }
            }
            ZF_CHECK(_A, 0);
            HF_CLR;
            break;            
        }
        case 0x2F: { // CPL;
            _A = ~_A;
            NF_SET;
            HF_SET;
            break;
        }
        case X_(0b0011, 111): { // CCF; SCF;
            (OP_BIT(3, 1) == 1) ? ((_CF == 1) ? CF_CLR : CF_SET) : CF_SET;
            NF_CLR;
            HF_CLR;
            break;
        }
        /* just like STOP, leaving unimplemented for now. */
        case 0x76: { // HALT;
            this->is_halted = true;
            break;
        }
        case XXX_(0b10000, ): // ADD A, r8;
        case XXX_(0b10001, ): {  // ADC A, r8;
            if(OP_BIT(0, 3) == 6) {
                SYNC_READ_HL_;
            }
            /* storing old value of CF for ADC, since it might change */
            uint8_t old_CF = _CF;
            ZF_CHECK(_A, R8(OP_BIT(0, 3)) + ((OP_BIT(3, 1) == 1) ? old_CF : 0));
            NF_CLR;
            ADD_HF_CHECK(_A, R8(OP_BIT(0, 3)) + ((OP_BIT(3, 1) == 1) ? old_CF : 0));
            /* checking HF with 3 operands */
            if((OP_BIT(3, 1) == 1) && (_HF == 0)) {
                ADD_HF_CHECK(R8(OP_BIT(0, 3)), old_CF);                
            }
            ADD_CF_CHECK(_A, R8(OP_BIT(0, 3)) + ((OP_BIT(3, 1) == 1) ? old_CF : 0));
            _A += R8(OP_BIT(0, 3)) + ((OP_BIT(3, 1) == 1) ? old_CF : 0);
            break;
        }
        case XXX_(0b10010, ): // SUB A, r8;
        case XXX_(0b10011, ): // SBC A, r8;
        case XXX_(0b10111, ): { // CP A, r8;
            if(OP_BIT(0, 3) == 6) {
                SYNC_READ_HL_;
            }
            /* storing old value of CF for SBC , since it might change */
            uint8_t old_CF = _CF;
            ZF_CHECK(_A, -(R8(OP_BIT(0, 3)) + ((OP_BIT(3, 3) == 3) ? old_CF : 0)));
            NF_SET;
            SUB_HF_CHECK(_A, R8(OP_BIT(0, 3)) + ((OP_BIT(3, 3) == 3) ? old_CF : 0));
            /* checking HF with 3 operands */
            if((OP_BIT(3, 3) == 3) && (_HF == 0)) {
                ADD_HF_CHECK(R8(OP_BIT(0, 3)), old_CF);                
            }
            SUB_CF_CHECK(_A, R8(OP_BIT(0, 3)) + ((OP_BIT(3, 3) == 3) ? old_CF : 0));
            /* CP doesn't actually change the value of A */
            if(OP_BIT(3, 3) != 7) {
                _A -= (R8(OP_BIT(0, 3)) + ((OP_BIT(3, 3) == 3) ? old_CF : 0));
            }
            break;
        }
        case XXX_(0b10100, ): // AND A, r8;
        case XXX_(0b10101, ): // XOR A, r8;
        case XXX_(0b10110, ): { // OR A, r8;
            if(OP_BIT(0, 3) == 6) {
                SYNC_READ_HL_;
            }
            _A = ((OP_BIT(3, 3) == 4) ? (_A & R8(OP_BIT(0, 3))) : ((OP_BIT(3, 3) == 5) ? (_A ^ R8(OP_BIT(0, 3))) : (_A | R8(OP_BIT(0, 3)))));
            ZF_CHECK(_A, 0);
            NF_CLR;
            (OP_BIT(3, 3) == 4) ? HF_SET : HF_CLR;
            CF_CLR;
            break;
        }
        case XX_(0b110, 000): { // RET; [conditional]
            SYNC_TO_CPU_; // instruction fetch
            SYNC_TO_CPU_; // conditional branch
            if(CONDITION(OP_BIT(3, 2))) {
                U16_WITH_SYNC(_SP);
                _PC = this->scratch.as_u16;
            }
            else {
                post_clock_advance = false; // disable extra clock in case branch is not taken.
            }
            break;
        } 
        case 0xE0: { // LD (0xFF00 + u8), A;
            SYNC_TO_CPU_; // instruction fetch
            uint8_t immediate = READ_U8(_PC);
            SYNC_TO_CPU_;
            WRITE_U8_NO_INC(0xFF00 + immediate, _A);
            break;
        }
        /*  https://stackoverflow.com/questions/57958631/game-boy-half-carry-flag-and-16-bit-instructions-especially-opcode-0xe8 
            Assuming same for opcode 0xF8 as well, god help me. */
        case 0xE8: { // ADD SP, i8;
            SYNC_TO_CPU_;
            int8_t immediate = READ_I8(_PC);
            SYNC_TO_CPU_; // after i8 fetch
            SYNC_TO_CPU_; // internal clock advance.
            ZF_CLR;
            NF_CLR;
            /* apparently these flags are calculated on the unsigned representation of i8, not sure why */
            ADD_HF_CHECK(_SP, immediate);
            ADD_CF_CHECK(BIT_CUT(_SP, 0, 8), (uint8_t)immediate);
            _SP = _SP + immediate;
            break;
        }
        case 0xF0: { // LD A, (0xFF00 + u8);
            SYNC_TO_CPU_;
            uint8_t immediate = READ_U8(_PC);
            SYNC_TO_CPU_;
            _A = READ_U8_NO_INC(0xFF00 + immediate);
            break;
        }
        case XX_(0b11, 0001): { // POP r16;
            SYNC_TO_CPU_;
            R16_G3(OP_BIT(4, 2)).as_u8.lower = READ_U8(_SP);
            SYNC_TO_CPU_;
            R16_G3(OP_BIT(4, 2)).as_u8.upper = READ_U8(_SP);
            /* ensuring that upper four bits of F are always 0 */
            if(OP_BIT(4, 2) == 3) {
                _F &= 0xF0;
            }
            break;
        }
        case X_(0b110, 1001): { // RET; RETI;
            SYNC_TO_CPU_;
            U16_WITH_SYNC(_SP);
            if(OP_BIT(4, 1) == 1) {
                this->ime_scheduled = IME_SETTING1;
            }
            _PC = this->scratch.as_u16;
            break;
        }
        case 0xE9: { // JP HL;
            _PC = this->hl.as_u16;
            break;
        }
        case 0xF9: { // LD SP, HL; 
            SYNC_TO_CPU_;
            _SP = this->hl.as_u16;
            break;
        }
        /* unconditional jump has similar timings to a conditional jump with branch taken so merging */
        case 0xC3: // JP u16;
        case XX_(0b110, 010): { // JP u16; [conditional]
            SYNC_TO_CPU_;
            /*  entire operand is read even if branch is not taken */
            U16_WITH_SYNC(_PC);
            if((opcode == 0xC3) || CONDITION(OP_BIT(3, 2))) {
                _PC = this->scratch.as_u16;
            } 
            else {
                post_clock_advance = false; // disable extra clock in case branch is not taken.
            }            
            break;
        }
        case 0xE2: { // LD (0xFF00 + C), A;
            SYNC_TO_CPU_;
            WRITE_U8_NO_INC(0xFF00 + this->bc.as_u8.lower, _A);
            break;
        }
        case 0xEA: { // LD (u16), A; 
            SYNC_TO_CPU_;
            U16_WITH_SYNC(_PC);
            WRITE_U8_NO_INC(this->scratch.as_u16, _A);
            break;
        }
        case 0xF2: { // LD A, (0xFF00 + C);
            SYNC_TO_CPU_;
            _A = READ_U8_NO_INC(0xFF00 + this->bc.as_u8.lower);
            break;
        }
        case 0xF8: { // LD HL, SP + i8;
            SYNC_TO_CPU_;
            int8_t immediate = READ_I8(_PC);
            SYNC_TO_CPU_;
            this->hl.as_u16 = _SP + immediate;
            ZF_CLR;
            NF_CLR;
            ADD_HF_CHECK(_SP, immediate);
            ADD_CF_CHECK(BIT_CUT(_SP, 0, 8), BIT_CUT(immediate, 0, 8));
            break;
        }
        case 0xFA: { // LD A, (u16); 
            SYNC_TO_CPU_;
            U16_WITH_SYNC(_PC);
            _A = READ_U8_NO_INC(this->scratch.as_u16);
            break;
        }
        /* There is some behaviour regarding EI taking effect only after 1 M-cycle, implementing that. */
        case 0xF3: { // DI;
            this->ime_scheduled = IME_UNSET;
            this->interrupt_master_enable = false;
            break;
        }
        case 0xFB: { // EI;
            this->ime_scheduled = IME_SETTING1;
            break;
        }
        case 0xCB: { // prefix 0xCB instructions
            SYNC_TO_CPU_; // fetching prefix byte
            opcode = READ_U8(_PC);
            if(OP_BIT(6, 2) == 0b00) {
                /* memory access timings for (HL) seem to be pretty consistent across all ALU ops */
                if(OP_BIT(0, 3) == 6) {
                    SYNC_READ_HL_;
                    SYNC_TO_CPU_;
                }
                /* removing 2 upper bits of opcode for switch case since they are common among all instructions, leaving a 6 bit opcode */
                switch(OP_BIT(0, 6)) {
                    case XXX_(0b000, ): // RLC r8;
                    case XXX_(0b010, ): { // RL r8;
                        uint8_t high_bit = GET_BIT(R8(OP_BIT(0, 3)), 7);
                        R8(OP_BIT(0, 3)) = (R8(OP_BIT(0, 3)) << 1) | ((OP_BIT(4, 1) == 1) ? _CF : high_bit);
                        ZF_CHECK(R8(OP_BIT(0, 3)), 0);
                        NF_CLR;
                        HF_CLR;
                        (high_bit == 1) ? CF_SET : CF_CLR; 
                        break;
                    }
                    case XXX_(0b001, ): // RRC r8;
                    case XXX_(0b011, ): { // RR r8;
                        uint8_t low_bit = GET_BIT(R8(OP_BIT(0, 3)), 0);
                        R8(OP_BIT(0, 3)) = (R8(OP_BIT(0, 3)) >> 1) | (((OP_BIT(4, 1) == 1) ? _CF : low_bit) << 7);
                        ZF_CHECK(R8(OP_BIT(0, 3)), 0);
                        NF_CLR;
                        HF_CLR;
                        (low_bit == 1) ? CF_SET : CF_CLR; 
                        break;
                    }
                    case XXX_(0b100, ): { // SLA r8;
                        (BIT_CUT(R8(OP_BIT(0, 3)), 7, 1) == 1) ? CF_SET : CF_CLR;
                        R8(OP_BIT(0, 3)) = (R8(OP_BIT(0, 3)) << 1);
                        ZF_CHECK(R8(OP_BIT(0, 3)), 0);
                        NF_CLR;
                        HF_CLR;
                        break;
                    }
                    case XXX_(0b101, ): // SRA r8;
                    case XXX_(0b111, ): { // SRL r8;
                        BIT_IS_SET(R8(OP_BIT(0, 3)), 0) ? CF_SET : CF_CLR;
                        R8(OP_BIT(0, 3)) = (R8(OP_BIT(0, 3)) >> 1) | ((OP_BIT(4, 1) == 0) ? (GET_BIT(R8(OP_BIT(0, 3)), 7) << 7) : 0);
                        ZF_CHECK(R8(OP_BIT(0, 3)), 0);
                        NF_CLR;
                        HF_CLR;
                        break;                        
                    }
                    default: { // falling through to SWAP r8;
                        R8(OP_BIT(0, 3)) = (BIT_CUT(R8(OP_BIT(0, 3)), 0, 4) << 4) | BIT_CUT(R8(OP_BIT(0, 3)), 4, 4);
                        ZF_CHECK(R8(OP_BIT(0, 3)), 0);
                        NF_CLR;
                        HF_CLR;
                        CF_CLR;
                        break;
                    }
                }
                if(OP_BIT(0, 3) == 6) {
                    WRITE_HL_;
                }
            }
            else if(OP_BIT(6, 2) == 0b01) { // BIT r8;
                if(OP_BIT(0, 3) == 6) {
                    SYNC_READ_HL_;
                }    
                BIT_IS_CLR(R8(OP_BIT(0, 3)), OP_BIT(3, 3)) ? ZF_SET : ZF_CLR;
                NF_CLR;
                HF_SET;           
            }
            else if(OP_BIT(6, 2) == 0b10) { // RES r8;
                if(OP_BIT(0, 3) == 6) {
                    SYNC_READ_HL_;
                    SYNC_TO_CPU_;
                }
                CLR_BIT(R8(OP_BIT(0, 3)), OP_BIT(3, 3));        
                if(OP_BIT(0, 3) == 6) {
                    WRITE_HL_;
                }       
            }
            else if(OP_BIT(6, 2) == 0b11) { // SET r8;
                if(OP_BIT(0, 3) == 6) {
                    SYNC_READ_HL_;
                    SYNC_TO_CPU_;
                }
                SET_BIT(R8(OP_BIT(0, 3)), OP_BIT(3, 3));   
                if(OP_BIT(0, 3) == 6) {
                    WRITE_HL_;
                }              
            }
            break;
        }
        case 0xCD: // CALL; [unconditional]
        case XX_(0b110, 100): { // CALL; [conditional]
            SYNC_TO_CPU_;
            U16_WITH_SYNC(_PC);
            if((opcode == 0xCD) || CONDITION(OP_BIT(3, 2))) {
                SYNC_TO_CPU_;
                WRITE_U8_NO_INC(--_SP, this->pc.as_u8.upper);
                SYNC_TO_CPU_;
                WRITE_U8_NO_INC(--_SP, this->pc.as_u8.lower);
                _PC = this->scratch.as_u16;
            }
            else {
                post_clock_advance = false;
            }
            break;
        }
        case XX_(0b11, 0101): { // PUSH r16;
            SYNC_TO_CPU_; // instruction fetch
            SYNC_TO_CPU_; // internal
            WRITE_U8_NO_INC(--_SP, R16_G3(OP_BIT(4, 2)).as_u8.upper);
            SYNC_TO_CPU_;
            WRITE_U8_NO_INC(--_SP, R16_G3(OP_BIT(4, 2)).as_u8.lower);
            break;
        }
        case 0xC6: // ADD A, u8;
        case 0xCE: { // ADC A, u8;
            SYNC_TO_CPU_;
            uint8_t immediate = READ_U8(_PC);
            /* storing old value of CF for ADC, since it might change */
            uint8_t old_CF = _CF;
            ZF_CHECK(_A, immediate + ((opcode == 0xCE) ? old_CF : 0));
            NF_CLR;
            ADD_HF_CHECK(_A, immediate + ((opcode == 0xCE) ? old_CF : 0));
            /* checking HF with 3 operands */
            if((opcode == 0xCE) && (_HF == 0)) {
                ADD_HF_CHECK(immediate, old_CF);                
            }
            ADD_CF_CHECK(_A, immediate + ((opcode == 0xCE) ? old_CF : 0));
            _A += immediate + ((opcode == 0xCE) ? old_CF : 0);
            break;
        }
        case 0xD6: // SUB A, u8;
        case 0xDE: // SBC A, u8;
        case 0xFE: { // CP A, u8;
            SYNC_TO_CPU_;
            uint8_t immediate = READ_U8(_PC);
            /* storing old value of CF for SBC, since it might change */
            uint8_t old_CF = _CF;
            ZF_CHECK(_A, -(immediate + ((opcode == 0xDE) ? old_CF : 0)));
            NF_SET;
            SUB_HF_CHECK(_A, immediate + ((opcode == 0xDE) ? old_CF : 0));
            /* checking HF with 3 operands */
            if((opcode == 0xDE) && (_HF == 0)) {
                ADD_HF_CHECK(immediate, old_CF);                
            }
            SUB_CF_CHECK(_A, immediate + ((opcode == 0xDE) ? old_CF : 0));
            /* CP doesn't actually change the value of A */
            if(opcode != 0xFE) {
                _A -= (immediate + ((opcode == 0xDE) ? old_CF : 0));
            }
            break;
        }
        case 0xE6: // AND A, u8;
        case 0xEE: // XOR A, u8;
        case 0xF6: { // OR A, u8;
            SYNC_TO_CPU_;
            uint8_t immediate = READ_U8(_PC);
            _A = ((opcode == 0xE6) ? (_A & immediate) : ((opcode == 0xEE) ? (_A ^ immediate) : (_A | immediate)));
            ZF_CHECK(_A, 0);
            NF_CLR;
            (opcode == 0xE6) ? HF_SET : HF_CLR;
            CF_CLR;
            break;
        }
        case XXX_(0b11, 111): { // RST;
            SYNC_TO_CPU_;
            SYNC_TO_CPU_;
            WRITE_U8_NO_INC(--_SP, this->pc.as_u8.upper);
            SYNC_TO_CPU_;
            WRITE_U8_NO_INC(--_SP, this->pc.as_u8.lower);
            _PC = OP_BIT(3, 3) * 0x08;
            break;
        }
        default: {
            if(OP_BIT(6, 2) == 0b01) { // LD r8, r8; [EXCEPT LD (HL), (HL); which overlaps with HALT]
            /*  even if read or write is from (HL), it still is done in another 4 cycles. */
                if(OP_BIT(3, 3) == 6) {
                    SYNC_TO_CPU_;
                }
                if(OP_BIT(0, 3) == 6) {
                    SYNC_READ_HL_;
                }
                R8(OP_BIT(3, 3)) = R8(OP_BIT(0, 3));
                if(OP_BIT(3, 3) == 6) {
                    WRITE_HL_;
                }
            }
            else {
                /* invalid opcode! */
                ERR_EXIT(INVALID_OPCODE_ERROR);               
            }
            break;
        }
    }

    if(post_clock_advance) {
        SYNC_TO_CPU_;
    }
    return;
}