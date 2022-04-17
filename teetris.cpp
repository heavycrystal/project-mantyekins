#include    <cstdio>
#include    <cstdint>
#include    <cstdlib>
#include    <cstring>
#include    <climits>

#define     CARTRIDGE_ROM_SIZE      32768
#define     VIDEO_RAM_SIZE          8192
#define     WORK_RAM_SIZE           8192
#define     OAM_SIZE                160
#define     IO_REGISTERS_SIZE       128
#define     HIGH_RAM_SIZE           127

#define     FILE_OPEN_ERROR         "Error opening cartridge file. Aborting.\n"
#define     FILE_READ_ERROR         "Error reading cartridge file. Aborting.\n"
#define     ROM_SIZE_INVALID_ERROR  "ROM is not expected size of 32768 bytes. Aborting.\n"
#define     ROM_UNSUPPORTED_ERROR   "ROM is not of a supported type. Aborting.\n"
#define     ROM_CHECKSUM_ERROR      "ROM header checksum did not match. Aborting.\n"

/*  Lobs off cut bits from input, and returns the count lowermost bits. 
    Generating bitmasks like this is not ideal, but hoping for constant folding to optimize it away. */
#define     BIT_CUT(input, cut, count)  (((input) >> cut) & ((1 << count) - 1))
#define     OP_BIT(cut, count)          BIT_CUT(opcode, cut, count)
#define     SET_BIT(input, bit)         (input |= (1 << bit))
#define     CLR_BIT(input, bit)         (input &= ~(1 << bit))

#define     _A                      gb_cpu->af.as_u8.lower   
#define     _F                      gb_cpu->af.as_u8.upper  
#define     _B                      gb_cpu->bc.as_u8.lower
#define     _C                      gb_cpu->bc.as_u8.upper
#define     _D                      gb_cpu->de.as_u8.lower
#define     _E                      gb_cpu->de.as_u8.upper
#define     _H                      gb_cpu->hl.as_u8.lower
#define     _L                      gb_cpu->hl.as_u8.upper
#define     _BC                     gb_cpu->bc.as_u16
#define     _DE                     gb_cpu->de.as_u16
#define     _HL                     gb_cpu->hl.as_u16
#define     _SP                     gb_cpu->sp.as_u16
#define     _PC                     gb_cpu->pc.as_u16
#define     _BC_UNION               gb_cpu->bc
#define     _DE_UNION               gb_cpu->de
#define     _HL_UNION               gb_cpu->hl
#define     _SP_UNION               gb_cpu->sp
#define     _PC                     gb_cpu->pc.as_u16

#define     _ZF                     BIT_CUT(gb_cpu->af.as_u8.upper, 7, 1)
#define     _NF                     BIT_CUT(gb_cpu->af.as_u8.upper, 6, 1)
#define     _HF                     BIT_CUT(gb_cpu->af.as_u8.upper, 5, 1)
#define     _CF                     BIT_CUT(gb_cpu->af.as_u8.upper, 4, 1)

#define     _ZF_SET                 SET_BIT(_F, 7)
#define     _ZF_CLR                 CLR_BIT(_F, 7)
#define     _NF_SET                 SET_BIT(_F, 6)
#define     _NF_CLR                 CLR_BIT(_F, 6)
#define     _HF_SET                 SET_BIT(_F, 5)
#define     _HF_CLR                 CLR_BIT(_F, 5)
#define     _CF_SET                 SET_BIT(_F, 4)
#define     _CF_CLR                 CLR_BIT(_F, 4)

#define     ZF_CHECK(output)            (output == 0) ? _ZF_SET : _ZF_CLR
/*  Takes the operands that are added or subtracted and modifies HF or CF accordingly */
#define     HF_CHECK(input1, input2)    (((BIT_CUT(input1, 0, 4) + BIT_CUT(input1, 0, 4)) & 0x10) == 0x10) ? _HF_SET : _HF_CLR   
#define     CF_CHECK(input1, input2)    ((UINT16_MAX - input1) < input2) ? _CF_SET : _CF_CLR   

#define     READ_U8_NO_INC(address)         (*gb_memory->memory_mapper(address))
#define     READ_I8_NO_INC(address)         ((int8_t)READ_U8_NO_INC(address))
#define     READ_U8(address)                READ_U8_NO_INC(address++)
#define     READ_I8(address)                READ_I8_NO_INC(address++)
#define     WRITE_U8_NO_INC(address, value) READ_U8_NO_INC(address) = value
#define     WRITE_U8(address, value)        WRITE_U8_NO_INC(address++)

#define     _X(prefix, suffix)          prefix ## 0 ## suffix: \
                                        case prefix ## 1 ## suffix
#define     _XX(prefix, suffix)         prefix ## 00 ## suffix: \
                                        case prefix ## 01 ## suffix: \
                                        case prefix ## 10 ## suffix: \
                                        case prefix ## 11 ## suffix 
#define     _XXX(prefix, suffix)         prefix ## 000 ## suffix: \
                                        case prefix ## 001 ## suffix: \
                                        case prefix ## 010 ## suffix: \
                                        case prefix ## 011 ## suffix: \
                                        case prefix ## 100 ## suffix: \
                                        case prefix ## 101 ## suffix: \
                                        case prefix ## 110 ## suffix: \
                                        case prefix ## 111 ## suffix

#define     SYNC_TO_CPU()               ppu_clock_step(); \
                                        audio_clock_step()

#define     ERR_EXIT(error)             fprintf(stderr, error); \
                                        exit(1)

/*  implementing the condition lookup table in terms of macros, look at the decoding document for more information */
#define     CONDITION(cond_code)   ((1 - BIT_CUT(cond_code, 0, 1)) ^ ((BIT_CUT(cond_code, 1, 1) == 1) ? _CF : _ZF))

/* implementing the register lookup tables in terms of macros, look at the decoding document for more information */
#define     R8_LHALF(reg_code)          ((reg_code >= 2) ? ((reg_code == 2) ? _D : _E) : ((reg_code == 0) ? _B : _C))
#define     R8_RHALF(reg_code)          ((reg_code >= 6) ? ((reg_code == 6) ? READ_U8_NO_INC(_HL) : _A) : ((reg_code == 4) ? _H : _L))
#define     R8(reg_code)                ((reg_code >= 4) ? R8_RHALF(reg_code) : R8_LHALF(reg_code))
#define     R16_G1(reg_code)            ((reg_code >= 2) ? ((reg_code == 2) ? _HL_UNION : _SP_UNION) : ((reg_code == 0) ? _BC_UNION : _DE_UNION)) 
#define     R16_G2(reg_code)            ((reg_code >= 2) ? ((reg_code == 2) ? _HL++ : _HL--) : ((reg_code == 0) ? _BC : _DE))  

struct gb_vram_t {
    uint8_t         data[VIDEO_RAM_SIZE];
    bool            is_locked;
};

struct gb_oam_t {
    uint8_t         data[OAM_SIZE];
    bool            is_locked;
};

class gb_memory_t {
    private:
    uint8_t         cartridge_rom[CARTRIDGE_ROM_SIZE];
    uint8_t         work_ram[WORK_RAM_SIZE];
    uint8_t         io_registers[IO_REGISTERS_SIZE];
    uint8_t         high_ram[HIGH_RAM_SIZE];
    uint8_t         interrupt_enable_register;

    public:
    gb_vram_t       video_ram;
    gb_oam_t        oam;

    gb_memory_t() {
        memset(work_ram, 0, sizeof(work_ram));
        memset(video_ram.data, 0, sizeof(video_ram.data));
        video_ram.is_locked = true;
        memset(oam.data, 0, sizeof(oam.data));
        oam.is_locked = true;
        memset(high_ram, 0, sizeof(high_ram));
    }

    /*  Open a given file, check if it is 32768 bytes exactly as we expect, and then load it to memory.
        Checks if opened file in fact a valid GB cartridge and also one that we do support, via rudimentary header checks.
        Returns true if the cartridge can continue execution, false if the cartridge is unsupported. */
    void load_and_verify_cartridge(char* cartridge_file_name) {
        FILE* cartridge_file = fopen(cartridge_file_name, "rb");
        if(cartridge_file == NULL) {
            ERR_EXIT(FILE_OPEN_ERROR);
        }

        /*  Checking if the file we open is actually 32768 bytes as we expect. */
        if((fread(this->cartridge_rom, 1, CARTRIDGE_ROM_SIZE, cartridge_file) != CARTRIDGE_ROM_SIZE) || (fgetc(cartridge_file) != EOF)) {
            ERR_EXIT(ferror(cartridge_file) ? FILE_READ_ERROR : ROM_SIZE_INVALID_ERROR);
        }

        /*  We could check the Nintendo logo byte by byte, like an actual Game Boy does.
            But that's legally dubious and locks out homebrew software so we don't do it. */

        /*  Checks if the cartridge is of the only type that we can currently run, 0x00 [only ROM] */
        if(cartridge_rom[0x0147] != 0x00) {
            ERR_EXIT(ROM_UNSUPPORTED_ERROR);
        }

        /*  Checks if the cartridge has the only ROM and RAM size that we can currently run, 0x00 [32kB or 32768 bytes], 0x00 [no RAM] */
        if((cartridge_rom[0x0148] != 0x00) || (cartridge_rom[0x0149] != 0x00)) {
            ERR_EXIT(ROM_UNSUPPORTED_ERROR);
        }

        /* Performing header checksum calculation */
        uint8_t checksum = 0;
        for(int loop_var = 0x0134; loop_var <= 0x014C; loop_var++) {
            checksum = checksum - cartridge_rom[loop_var] - 1;
        }
        if(cartridge_rom[0x014D] != checksum) {
            ERR_EXIT(ROM_CHECKSUM_ERROR);
        }

        /*  ROM seems to pass our rudimentary checks, go for launch. */
        return;
    }

    /*  This function directs any memory accesses by the CPU to the appropriate peripheral.
        It also enforces the OAM/video RAM locking by the video subsystem/PPU.
        It aborts the emulator if any invalid memory accesses are made, otherwise returns a pointer to the requested byte. */
    uint8_t* memory_mapper(uint16_t address) {
        if(address <= 0x7FFF) {
            return &this->cartridge_rom[address]; /* in Cartridge ROM */
        }
        else if(address <= 0x9FFF) {
            if(this->video_ram.is_locked == false) {
                return &this->video_ram.data[address - 0x8000]; /* in video RAM */
            }
            /* invalid memory access, video RAM access while locked */
        }
        else if(address <= 0xBFFF) {
            /* invalid memory access, external RAM */
        }
        else if(address <= 0xDFFF) {
            return &this->work_ram[address - 0xC000]; /* in work RAM */
        }
        /*  due to how the Game Boy is implemented, this region of memory actually redirects to work RAM.
            It is known unofficially as echo RAM. Nintendo does not allow games to use this memory area. */
        else if(address <= 0xFDFF) {
            /* invalid memory access, echo RAM */
        }
        else if(address <= 0xFE9F) {
            if(this->oam.is_locked == false) {
                return &this->oam.data[address - 0xFE00]; /* in OAM */
            }
            /* invalid memory access, OAM access while locked */        
        }
        else if(address <= 0xFEFF) {
            /* invalid memory access, general prohibited area */
        }
        else if(address <= 0xFF7F) {
            /* switch to proper flag data structure. */
            return &this->io_registers[address - 0xFF00]; /* in IO registers */
        }
        else if(address <= 0xFFFE) {
            return &this->high_ram[address - 0xFF80]; /* in high RAM */
        }
        return &this->interrupt_enable_register;
    }
};

/*  A very janky union, just for the sake of efficiency. 
    Should break under big-endian systems, and not sure how it interacts with struct padding. */
union gb_cpu_register_t {
    uint16_t as_u16;
    struct as_u8_t {
        uint8_t lower;
        uint8_t upper;
    } as_u8;
};

/*  The struct storing the state of all the CPU registers */
class gb_cpu_t {
    public:
    gb_cpu_register_t af;
    gb_cpu_register_t bc;
    gb_cpu_register_t de;
    gb_cpu_register_t hl;
    gb_cpu_register_t sp;
    gb_cpu_register_t pc;

    uint64_t cycle_count;

    /*  The values of the CPU registers at game initialization are primarily determined by the boot ROM of the Game Boy in question.
        There can be different combinations, using the combination for boot ROM "DMG". 
        Taken from: https://gbdev.io/pandocs/Power_Up_Sequence.html 
        For this boot ROM, the flags depend on value of header checksum. Ignoring this nitpick for now and just setting both H and C. */
    gb_cpu_t() {
        af.as_u8.lower = 0x01;
        af.as_u8.upper = 0b1011000;
        bc.as_u8.lower = 0x00;
        bc.as_u8.upper = 0x13;
        de.as_u8.lower = 0x00;
        de.as_u8.upper = 0xD8;
        hl.as_u8.lower = 0x01;
        hl.as_u8.upper = 0x4D;

        sp.as_u16 = 0xFFFE;
        pc.as_u16 = 0x0100;
    }
};

void ppu_clock_step() {

}

void audio_clock_step() {

}

void fetch_and_execute_instruction(gb_memory_t* gb_memory, gb_cpu_t* gb_cpu) {
    /*  For (more) accurate system emulation, we need to ensure video/PPU and audio subsystems are in sync with the CPU. 
        Since instructions take differing amounts of clock cycles, we split each instruction into "micro-ops" which each take 4 clock cycles each.
        After each micro-op is emulated, we call the video/PPU and audio subsystems so they can sync up. */
    uint8_t opcode = READ_U8(_PC);
    /* Instruction fetch always takes 4 clock cycles so step now. */
    SYNC_TO_CPU();

    /*  This was written with the help of the following instruction decoding guide:
        https://cdn.discordapp.com/attachments/465586075830845475/742438340078469150/SM83_decoding.pdf 
        Timings derived from: https://izik1.github.io/gbops/ 
        Instruction description and further verification done from http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf 
        as well as https://gbdev.io/pandocs/CPU_Instruction_Set.html */
    switch(opcode) {
        case 0x00: { //   NOP;
            break;
        }
        case 0x08: { // LD (u16), SP;
            uint16_t immediate = READ_U8(_PC);
            SYNC_TO_CPU();
            immediate = (READ_U8(_PC) << 8) | immediate;
            SYNC_TO_CPU();
            gb_cpu->sp.as_u8.lower = READ_U8_NO_INC(immediate);
            SYNC_TO_CPU();
            gb_cpu->sp.as_u8.upper = READ_U8_NO_INC(immediate + 1);
            SYNC_TO_CPU();
            break;
        }
        /*  https://gbdev.io/pandocs/Reducing_Power_Consumption.html 
            Leaving instruction unimplemented for now. */
        case 0x10: { //   STOP;
            /* invalid opcode! */
            break;
        }
        case 0x18: { // JR i8; [unconditional]
            int8_t immediate = READ_I8(_PC);
            SYNC_TO_CPU();
            _PC = _PC + immediate;
            SYNC_TO_CPU();
            break;
        }
        case _XX(0b001, 000): { // JR I8; [conditional]
            int8_t immediate = READ_I8(_PC);
            SYNC_TO_CPU();
            if(CONDITION(OP_BIT(3, 2))) {
                _PC = _PC + immediate;
                SYNC_TO_CPU();            
            }
            break;
        }
        case _XX(0b00, 0001): { // LD r16, u16;
            R16_G1(OP_BIT(4, 2)).as_u8.upper = READ_U8(_PC);
            SYNC_TO_CPU();  
            R16_G1(OP_BIT(4, 2)).as_u8.lower = READ_U8(_PC);
            SYNC_TO_CPU();    
            break;
        }
        case _XX(0b00, 1001): { // ADD HL, r16;
            HF_CHECK(_HL, R16_G1(OP_BIT(4, 2)).as_u16);
            CF_CHECK(_HL, R16_G1(OP_BIT(4, 2)).as_u16);
            _HL = _HL + R16_G1(OP_BIT(4, 2)).as_u16;
            SYNC_TO_CPU();
            break;
        }
        case _XX(0b00, 0010): { // LD (r16), A;
            WRITE_U8_NO_INC(R16_G2(OP_BIT(4, 2)), _A);
            SYNC_TO_CPU();
            break;
        }
        case _XX(0b00, 1010): { // LD A, (r16);
            _A = READ_U8_NO_INC(R16_G2(OP_BIT(4, 2)));
            SYNC_TO_CPU();
            break;
        }
        case _XX(0b00, 0011): { // INC r16;
            R16_G1(OP_BIT(4, 2)).as_u16++;
            SYNC_TO_CPU();
            break;
        }
        case _XX(0b00, 1011): { // DEC r16;
            R16_G1(OP_BIT(4, 2)).as_u16--;
            SYNC_TO_CPU();
            break;
        }
        case _XXX(0b00, 100): { // INC r8;
            /* performing read and write in one expression, so advancing clock beforehand */
            if(OP_BIT(3, 3) == 6) {
                SYNC_TO_CPU();
            }
            HF_CHECK(R8(OP_BIT(3, 3)), 1);
            ZF_CHECK(++R8(OP_BIT(3, 3)));
            _NF_CLR;
            /*  writing back to memory */
            if(OP_BIT(3, 3) == 6) {
                SYNC_TO_CPU();
            }
            break;
        }
        case _XXX(0b00, 101): { // DEC r8;
            /*  performing read and write in one expression, so advancing clock beforehand */
            if(OP_BIT(3, 3) == 6) {
                SYNC_TO_CPU();
            }
            HF_CHECK(R8(OP_BIT(3, 3)), -1);
            ZF_CHECK(--R8(OP_BIT(3, 3)));
            _NF_SET;
            /*  writing back to memory */
            if(OP_BIT(3, 3) == 6) {
                SYNC_TO_CPU();
            }
            break;
        }
        case _XXX(0b00, 110): { // LD r8, u8;
            /* to ensure timing of this instruction is accurate, single expression write can't happen if it is (HL), since it involves two memory operations */
            if(OP_BIT(3, 3) != 6) {
                R8(OP_BIT(3, 3)) = READ_U8(_PC);
            }
            SYNC_TO_CPU();
            if(OP_BIT(3, 3) == 6) {
                R8(OP_BIT(3, 3)) = READ_U8(_PC);
                SYNC_TO_CPU();
            }
            break;
        }
        case _X(0b000, 0111): { // RLCA; RLA;
            uint8_t high_bit = BIT_CUT(_A, 7, 1);
            _A = (_A << 1) | ((OP_BIT(4, 1) == 1) ? _CF : 0);
            (high_bit == 1) ? _CF_SET : _CF_CLR; 
            _ZF_CLR;
            _HF_CLR;
            _NF_CLR;
            break;
        }
        case _X(0b000, 1111): { // RRCA; RRA;
            uint8_t low_bit = BIT_CUT(_A, 0, 1);
            _A = (_A >> 1) | ((OP_BIT(4, 1) == 1) ? (_CF << 7) : 0);
            (low_bit == 1) ? _CF_SET : _CF_CLR; 
            _ZF_CLR;
            _HF_CLR;
            _NF_CLR;
            break;
        }
        /* weird opcode, leaving unimplemented for now. */
        case 0x27: { // DAA;
            /* invalid opcode! */
            break;            
        }
        case 0x2F: { // CPL;
            _A = ~_A;
            _NF_SET;
            _HF_SET;
            break;
        }
        case _X(0b0011, 111): { // CCF; SCF;
            (OP_BIT(3, 1) == 1) ? ((_CF == 1) ? _CF_CLR : _CF_SET) : _CF_SET;
            _NF_CLR;
            _HF_CLR;
            break;
        }
        /* just like STOP, leaving unimplemented for now. */
        case 0x76: { // HALT;
            /* invalid opcode! */
            break;
        }
        case _XXX(0b10000, ): // ADD A, r8;
        case _XXX(0b10001, ): {  // ADC A, r8;
            HF_CHECK(_A, R8(OP_BIT(0, 3)) + ((OP_BIT(3, 1) == 1) ? _CF : 0));
            CF_CHECK(_A, R8(OP_BIT(0, 3)) + ((OP_BIT(3, 1) == 1) ? _CF : 0));
            _NF_CLR;
            _A += R8(OP_BIT(0, 3)) + ((OP_BIT(3, 1) == 1) ? _CF : 0);
            ZF_CHECK(_A);
            if(OP_BIT(0, 3) == 6) {
                SYNC_TO_CPU();
            }
            break;
        }
        case _XXX(0b10010, ): // SUB A, r8;
        case _XXX(0b10011, ): // SBC A, r8;
        case _XXX(0b10111, ): { // CP A, r8;
            HF_CHECK(_A, -(R8(OP_BIT(0, 3)) + ((OP_BIT(3, 3) == 3) ? _CF : 0)));
            CF_CHECK(_A, -(R8(OP_BIT(0, 3)) + ((OP_BIT(3, 3) == 3) ? _CF : 0)));
            _NF_SET;
            _A -= (R8(OP_BIT(0, 3)) + ((OP_BIT(3, 3) == 3) ? _CF : 0));
            ZF_CHECK(_A);
            /* CP doesn't actually change the value of A, so reverting. */
            if(OP_BIT(3, 3) == 7) {
                _A += (R8(OP_BIT(0, 3)) + ((OP_BIT(3, 3) == 3) ? _CF : 0));
            }
            if(OP_BIT(0, 3) == 6) {
                SYNC_TO_CPU();
            }
            break;
        }
        case _XXX(0b10100, ): // AND A, r8;
        case _XXX(0b10101, ): // XOR A, r8;
        case _XXX(0b10110, ): { // OR A, r8;
            _A = ((OP_BIT(3, 3) == 4) ? (_A & R8(OP_BIT(0, 3))) : ((OP_BIT(3, 3) == 5) ? (_A ^ R8(OP_BIT(0, 3))) : (_A | R8(OP_BIT(0, 3)))));
            _NF_CLR;
            (OP_BIT(3, 3) == 4) ? _HF_SET : _HF_CLR;
            _CF_CLR;
            ZF_CHECK(_A);
            if(OP_BIT(0, 3) == 6) {
                SYNC_TO_CPU();
            }
            break;
        }
        default: {
            if(OP_BIT(6, 2) == 0b01) { // LD r8, r8; [EXCEPT LD (HL), (HL); which overlaps with HALT;]
            /*  even if read or write is from (HL), it still is done in another 4 cycles. */
                R8(OP_BIT(3, 3)) = R8(OP_BIT(0, 3));
                if((OP_BIT(0, 3) == 6) || (OP_BIT(3, 3) == 6)) {
                    SYNC_TO_CPU();
                }
            }
        }
    }
}

int main(int argc, char** argv) {

    gb_memory_t gb_memory;
    gb_memory.load_and_verify_cartridge(argv[argc - 1]);
    gb_cpu_t gb_cpu;
    
    return 0;
}