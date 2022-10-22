#include    "common/common.hpp"
#include    "cpu/cpu.hpp"
#include    "memory/memory.hpp"
#include    "ppu/ppu.hpp"
#include    "timer/timer.hpp"

int main(int argc, char** argv) {

    gb_ioregs_t gb_ioregs;
    gb_timer_t gb_timer(&gb_ioregs);
    gb_vram_t gb_vram;
    gb_oam_t gb_oam;
    gb_memory_t gb_memory(argv[argc - 1], &gb_ioregs, &gb_vram, &gb_oam);
    gb_cpu_t gb_cpu;
    gb_memory.write_memory(0xFF4D, 0xFF);
    char next;
    
    while(gb_cpu.ro_cycle_count < (1 << 26)) {
#if ENABLE_EXEC_BREAKPOINTS
        for(u16 loop_var : EXEC_BREAKPOINTS) {
            if(gb_cpu.pc.as_u16 == loop_var) {
                fprintf(stderr, "Execution breakpoint reached at %04x, entering single stepping mode.\n", loop_var);
                is_stepping = true;
            }
        }
#endif
        if(gb_cpu.is_stepping) {
            scanf(" %c", &next);
            if(next == 'r') {
                fprintf(stderr, "Exiting single stepping mode.\n");
                gb_cpu.is_stepping = false;
            }
        }
        gb_cpu.fetch_and_execute_instruction(&gb_memory, &gb_timer); 
        gb_cpu.handle_interrupts(&gb_memory, &gb_timer, &gb_ioregs);
        if(gb_memory.read_memory(0xFF02) == 0x81) {
            printf("%c", gb_memory.read_memory(0xFF01));
            gb_memory.write_memory(0xFF02, 0x01);
        }
    }
    printf("Performed %zu cycles.\n", gb_cpu.ro_cycle_count);
    return 0;    
}