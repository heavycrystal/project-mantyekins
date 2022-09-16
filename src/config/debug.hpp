/* ensure assertions are enabled */
#ifndef NDEBUG
    #define NDEBUG
#endif

/* debugging routines */      
#define     ENABLE_INTERRUPT_STEP       0
#define     ENABLE_EXEC_BREAKPOINTS     0
#define     ENABLE_READ_BREAKPOINTS     0
#define     ENABLE_WRITE_BREAKPOINTS    0
#define     ENABLE_OPCODE_BREAKPOINTS   0
#define     ENABLE_DEBUG_PRINTF         0

#if ENABLE_EXEC_BREAKPOINTS
static uint16_t EXEC_BREAKPOINTS[] = { 0x0100 };
#endif
#if ENABLE_READ_BREAKPOINTS
static uint16_t READ_BREAKPOINTS[] = { 0xA000 };
#endif
#if ENABLE_WRITE_BREAKPOINTS
static uint16_t WRITE_BREAKPOINTS[] = { 0xA000 };
#endif
#if ENABLE_OPCODE_BREAKPOINTS
static uint8_t  OPCODE_BREAKPOINTS[] = { 0x34, 0x35 };    
#endif