#include "cpu.h"

void cpu_subsystem::u8_alu_parse()
{
    switch(cpu_registers.byte_at_pc)
    {
        case    0x04:   //  INC B;
            cpu_registers.bc.as_u8[0]++;
            ZF_CHECK(bc.as_u8[0]);
            UNSET_FLAG(nf);
            HF_CHECK(bc.as_u8[0]);
            break;

        case    0x05:   //  DEC B;
            cpu_registers.bc.as_u8[0]--;
            ZF_CHECK(bc.as_u8[0]);
            SET_FLAG(nf);
            HF_CHECK(bc.as_u8[0]);
            break;

        case    0x0C:   //  INC C;
            cpu_registers.bc.as_u8[1]++;
            ZF_CHECK(bc.as_u8[1]);
            UNSET_FLAG(nf);
            HF_CHECK(bc.as_u8[1]); 
            break; 

        case    0x0D:   //  DEC C;
            cpu_registers.bc.as_u8[1]--;
            ZF_CHECK(bc.as_u8[1]);
            SET_FLAG(nf);
            HF_CHECK(bc.as_u8[1]); 
            break; 

        case    0x14:   //  INC D;
            cpu_registers.de.as_u8[0]++;
            ZF_CHECK(de.as_u8[0]);
            UNSET_FLAG(nf);
            HF_CHECK(de.as_u8[0]);  
            break;  

        case    0x15:   //  DEC D;
            cpu_registers.de.as_u8[0]--;
            ZF_CHECK(de.as_u8[0]);
            SET_FLAG(nf);
            HF_CHECK(de.as_u8[0]);
            break; 

        case    0x1C:   //  INC E;
            cpu_registers.de.as_u8[1]++;
            ZF_CHECK(de.as_u8[1]);
            UNSET_FLAG(nf);
            HF_CHECK(de.as_u8[1]); 
            break; 

        case    0x1D:   //  DEC E;
            cpu_registers.de.as_u8[1]--;
            ZF_CHECK(de.as_u8[1]);
            SET_FLAG(nf);
            HF_CHECK(de.as_u8[1]); 
            break;

        case    0x24:   //  INC H;
            cpu_registers.hl.as_u8[0]++;
            ZF_CHECK(hl.as_u8[0]);
            UNSET_FLAG(nf);
            HF_CHECK(hl.as_u8[0]);
            break;

        case    0x25:   //  DEC H;
            cpu_registers.hl.as_u8[0]--;
            ZF_CHECK(hl.as_u8[0]);
            SET_FLAG(nf);
            HF_CHECK(hl.as_u8[0]);
            break; 

        case    0x27:   //  DAA
//  unimplemented
            break; 

        case    0x2C:   //  INC L;
            cpu_registers.hl.as_u8[1]++;
            ZF_CHECK(hl.as_u8[1]);
            UNSET_FLAG(nf);
            HF_CHECK(hl.as_u8[1]);
            break;

        case    0x2D:   //  DEC L;
            cpu_registers.hl.as_u8[1]--;
            ZF_CHECK(hl.as_u8[1]);
            SET_FLAG(nf);
            HF_CHECK(hl.as_u8[1]);
            break; 

        case    0x2F:   //  CPL;
            cpu_registers.a = (cpu_registers.a ^ 0xFF);
            SET_FLAG(nf);
            SET_FLAG(hf);
            break;   

        case    0x34:   //  INC (HL);
//  unimplemented
            break;

        case    0x35:   //  DEC (HL);    

    }
}