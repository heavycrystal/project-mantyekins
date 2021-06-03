#include    "../typedefs.h"

#define     ZF_CHECK(input)     cpu_registers.zf = (cpu_registers. ## input == 0)
#define     HF_CHECK(input)     cpu_registers.hf = (((cpu_registers. ## input) & 0x0F) == 0)

#define     SET_FLAG(input)     cpu_registers. ## input = true;
#define     UNSET_FLAG(input)   cpu_registers. ## input = false;

union u16_as_bytes
{
    u16 as_u16;
    u8  as_u8[2];
};

class cpu_subsystem
{
#ifndef DEBUG    
    private:
#else
    public:
#endif  

    struct cpu_registers_struct
    {
        u8              a;
        u16_as_bytes    bc;
        u16_as_bytes    de;
        u16_as_bytes    hl;
        u16             sp;
        u16             pc;

        bool            zf;
        bool            nf;
        bool            hf;
        bool            cf;

        u8              byte_at_pc;
        u8              byte_at_sp;
    }   cpu_registers;

    void    u8_alu_parse();
    void    u8_lsm_parse();
    void    u16_alu_parse();
    void    u16_lsm_parse();
    void    u8_misc_parse();
    void    ctrl_branch_parse();
    void    ctrl_misc_parse();

    public:

    cpu_subsystem() = delete;

    void    instruction_dispatch();
};    