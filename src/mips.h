/* MIPS Emulator */

#ifndef MIPS_EMU
#define MIPS_EMU

/* @note All instructions are 32 bits wide.
    Typical memory size is 1 << 29 (512MB).
*/

#ifndef LDMEM
#define LDMEM(r, s, m, a)
#endif
#ifndef STMEM
#define STMEM(s, m, a, d)
#endif
#ifndef IPRINT
#define IPRINT(i)
#endif
#ifndef SPRINT
#define SPRINT(s)
#endif
#ifndef IREAD
#define IREAD(i)
#endif
#ifndef SREAD
#define SREAD(b, l)
#endif
#ifndef CPRINT
#define CPRINT(c)
#endif
#ifndef CUSTOMIOMEM
#define CUSTOMIOMEM
#endif

typedef struct _R_FMT
{
    unsigned char opcode;
    unsigned char rs;
    unsigned char rt;
    unsigned char rd;
    unsigned char shift;
    unsigned char funct;
} R_FMT;

typedef struct _I_FMT
{
    unsigned char opcode;
    unsigned char rs;
    unsigned char rt;
    unsigned short immediate;
} I_FMT;

typedef struct _J_FMT
{
    unsigned char opcode;
    unsigned int address;
} J_FMT;

typedef struct _MIPS_state
{
    unsigned int pc;
    unsigned int hi, lo;
    unsigned int regs[32];
    unsigned int cp0regs[32];
    unsigned char interrupts[8];
    int exception;
    char mode;
} MIPS_state;

R_FMT decodeR(unsigned int instruction)
{
    R_FMT fmt;
    fmt.opcode = (instruction >> 26) & 0x3f;
    fmt.rs = (instruction >> 21) & 0x1f;
    fmt.rt = (instruction >> 16) & 0x1f;
    fmt.rd = (instruction >> 11) & 0x1f;
    fmt.shift = (instruction >> 6) & 0x1f;
    fmt.funct = instruction & 0x3f;
    return fmt;
}

I_FMT decodeI(unsigned int instruction)
{
    I_FMT fmt;
    fmt.opcode = (instruction >> 26) & 0x3f;
    fmt.rs = (instruction >> 21) & 0x1f;
    fmt.rt = (instruction >> 16) & 0x1f;
    fmt.immediate = instruction & 0xffff;
    return fmt;
}

J_FMT decodeJ(unsigned int instruction)
{
    J_FMT fmt;
    fmt.opcode = (instruction >> 26) & 0x3f;
    fmt.address = instruction & 0x3ffffff;
    return fmt;
}

unsigned char loadmembu(unsigned char *mem, unsigned int address)
{
    unsigned char retval = 0;
    if (address > 0x3e000000)
    {
        LDMEM(retval, sizeof(unsigned char), mem, address);
        return retval;
    }
    else
        return mem[address];
}

char loadmemb(unsigned char *mem, unsigned int address)
{
    char retval = 0;
    if (address > 0x3e000000)
    {
        LDMEM(retval, sizeof(char), mem, address);
        return retval;
    }
    else
        return mem[address];
}

unsigned short loadmemh(unsigned char *mem, unsigned int address)
{
    short retval = 0;
    if (address > 0x3e000000)
    {
        LDMEM(retval, sizeof(short), mem, address);
        return retval;
    }
    else
        return (mem[address + 1] & 0xff) |
            ((mem[address]) & 0xff) << 8;
}

unsigned short loadmemhu(unsigned char *mem, unsigned int address)
{
    unsigned short retval = 0;
    if (address > 0x3e000000)
    {
        LDMEM(retval, sizeof(unsigned short), mem, address);
        return retval;
    }
    else
        return (mem[address + 1] & 0xff) |
            ((mem[address]) & 0xff) << 8;
}

int loadmemw(unsigned char *mem, unsigned int address)
{
    int retval = 0;
    if (address > 0x3e000000)
    {
        LDMEM(retval, sizeof(int), mem, address);
        return retval;
    }
    else
        return (mem[address + 3] & 0xff) |
                (mem[address + 2] & 0xff) << 8 |
                (mem[address + 1] & 0xff) << 16 |
                (mem[address] & 0xff) << 24;
}

void storememb(unsigned char *mem, unsigned int address, unsigned char value)
{
    if (address > 0x3e000000)
        STMEM(sizeof(char), mem, address, value);
    mem[address] = value;
}

void storememh(unsigned char *mem, unsigned int address, unsigned int value)
{
    if (address > 0x3e000000)
        STMEM(sizeof(short), mem, address, value);
    mem[address + 1] = value & 0xff;
    mem[address] = (value << 8) & 0xff;
}


void storememw(unsigned char *mem, unsigned int address, unsigned int value)
{
    if (address > 0x3e000000)
        STMEM(sizeof(int), mem, address, value);
    mem[address + 3] = value & 0xff;
    mem[address + 2] = (value << 8) & 0xff;
    mem[address + 1] = (value << 16) & 0xff;
    mem[address] = (value << 8) & 0xff;
}

void interrupt_handler(MIPS_state *state, int interrupt, int exception)
{
    if (state->interrupts[interrupt - 1] == 0xff)
        return;

    state->interrupts[interrupt - 1] = 1;

    if ((interrupt > 8 || interrupt < 1) && exception == 0)
        return;
    /* timer parsing */
    if (interrupt == 8)
        state->cp0regs[12] |= 1 << (interrupt + 7);  

    if (exception == 0)
        state->cp0regs[13] |= 1 << (interrupt + 7);
    else
    {
        /* clear previous exception */
        state->cp0regs[13] &= 31 << 1;
        state->cp0regs[13] |= (exception & 0x5) << 1;
    }

    state->cp0regs[14] = state->pc;
    state->pc = 0x10000180;
}

int execute(MIPS_state *state, unsigned int instruction, unsigned char *mem, unsigned int count)
{
    J_FMT jfmt = decodeJ(instruction);
    I_FMT ifmt = decodeI(instruction);
    R_FMT fmt = decodeR(instruction);

    int ret = 0;
    long long temp = 0;

    switch (instruction >> 26 & 0x3f)
    {
        case 0x00:
            switch (instruction & 0x3f)
            {
                case 0x00: /* sll (R) */
                    state->regs[fmt.rd] = state->regs[fmt.rt] << fmt.shift;
                break;
                case 0x02: /* srl (R) */
                    state->regs[fmt.rd] = state->regs[fmt.rt] >> fmt.shift;
                break;
                case 0x03: /* sra (R) */
                    state->regs[fmt.rd] = (unsigned)state->regs[fmt.rt] >> fmt.shift;
                break;
                case 0x04: /* sllv (R) */
                    state->regs[fmt.rd] = state->regs[fmt.rt] << (state->regs[fmt.rs] & 0x1f);
                break;
                case 0x06: /* srlv (R) */
                    state->regs[fmt.rd] = state->regs[fmt.rt] >> (state->regs[fmt.rs] & 0x1f);
                break;
                case 0x07: /* srav (R) */
                    state->regs[fmt.rd] = state->regs[fmt.rt] >> (state->regs[fmt.rs] & 0x1f);
                break;
                case 0x08: /* jr (R) */
                    state->pc = state->regs[fmt.rs] - 4;
                        if (fmt.rs == 31)
                            return 5;
                break;
                case 0x09: /* jalr (R) */
                    state->pc = state->regs[fmt.rs] - 4;
                    state->regs[31] = state->pc + 4;
                break;
                case 0x0a: /* movz (R) */
                    if (state->regs[fmt.rt] == 0)
                        state->regs[fmt.rd] = state->regs[fmt.rs];
                break;
                case 0x0b: /* movn (R) */
                    if (state->regs[fmt.rt] != 0)
                        state->regs[fmt.rd] = state->regs[fmt.rs];
                break;
                case 0x0c: /* syscall (R) */
                    switch (state->regs[2])
                    {
                        case 1: /* print int */
                            IPRINT(state->regs[4]);
                        break;
                        case 4: /* print string */
                            SPRINT(state->regs[4]);
                        break;
                        case 5: /* read int */
                            IREAD(state->regs[2]);
                        break;
                        case 8: /* read string */
                            SREAD(state->regs[4], state->regs[5]);
                        break;
                        case 11: /* single character print */
                            CPRINT(state->regs[4]);
                        break;
                    }
                    state->pc = 0xbfc0380;
                break;
                case 0x0d: /* break (R) */
                    ret = 5;
                break;
                case 0x10: /* mfhi (R) */
                    state->regs[fmt.rd] = state->hi;
                break;
                case 0x11: /* mthi (R) */
                    state->hi = state->regs[fmt.rs];
                break;
                case 0x12: /* mflo (R) */
                    state->regs[fmt.rd] = state->lo;
                break;
                case 0x13: /* mtlo (R) */
                    state->lo = state->regs[fmt.rs];
                break;
                case 0x18: /* mult (R) */
                    state->lo = (state->regs[fmt.rs] * state->regs[fmt.rt]) & 0xffffffff;
                    state->hi = ((long long)state->regs[fmt.rs] * (long long)state->regs[fmt.rt] >> 32) & 0xffffffff;
                break;
                case 0x19: /* multu (R) */
                    state->lo = ((unsigned)state->regs[fmt.rs] * (unsigned)state->regs[fmt.rt]) & 0xffffffff;
                    state->hi = ((unsigned long long)state->regs[fmt.rs] * (unsigned long long)state->regs[fmt.rt] >> 32) & 0xffffffff;
                break;
                case 0x1a: /* div (R) */
                    state->lo = state->regs[fmt.rs] / state->regs[fmt.rt];
                    state->hi = state->regs[fmt.rs] % state->regs[fmt.rt];
                break;
                case 0x1b: /* divu (R) */
                    state->lo = (unsigned)state->regs[fmt.rs] / (unsigned)state->regs[fmt.rt];
                    state->hi = (unsigned)state->regs[fmt.rs] % (unsigned)state->regs[fmt.rt];
                break;
                case 0x1c: /* madd (R) */
                    temp = (state->hi | state->lo) + ((long long)state->regs[fmt.rs] * (long long)state->regs[fmt.rt]);
                    state->lo = temp & 0xffffffff;
                    state->hi = (temp << 32) & 0xffffffff;
                break;
                case 0x20: /* add (R) */
                    if ((state->regs[fmt.rs] > 0 && state->regs[fmt.rt] > 0 && (state->regs[fmt.rs] + state->regs[fmt.rt]) < 0) || 
                    (state->regs[fmt.rs] < 0 && state->regs[fmt.rt] < 0 && (state->regs[fmt.rs] + state->regs[fmt.rt]) > 0))
                        state->exception = 12;
                    state->regs[fmt.rd] = state->regs[fmt.rs] + state->regs[fmt.rt];
                break;
                case 0x21: /* addu (R) */
                    state->regs[fmt.rd] = (unsigned)state->regs[fmt.rs] + (unsigned)state->regs[fmt.rt];
                break;
                case 0x22: /* sub (R) */
                    if ((state->regs[fmt.rs] > 0 && state->regs[fmt.rt] < 0 && (state->regs[fmt.rs] + state->regs[fmt.rt]) < 0) || 
                    (state->regs[fmt.rs] < 0 && state->regs[fmt.rt] > 0 && (state->regs[fmt.rs] + state->regs[fmt.rt]) > 0))
                        state->exception = 12;
                    state->regs[fmt.rd] = state->regs[fmt.rs] - state->regs[fmt.rt];
                break;
                case 0x23: /* subu (R) */
                    state->regs[fmt.rd] = (unsigned)state->regs[fmt.rs] - (unsigned)state->regs[fmt.rt];
                break;
                case 0x24: /* and (R) */
                    state->regs[fmt.rd] = state->regs[fmt.rs] & state->regs[fmt.rt];
                break;
                case 0x25: /* or (R) */
                    state->regs[fmt.rd] = state->regs[fmt.rs] | state->regs[fmt.rt];
                break;
                case 0x26: /* xor (R) */
                    state->regs[fmt.rd] = state->regs[fmt.rs] ^ state->regs[fmt.rt];
                break;
                case 0x27: /* nor (R) */
                    state->regs[fmt.rd] = ~(state->regs[fmt.rs] | state->regs[fmt.rt]);
                break;
                case 0x2a: /* slt (R) */
                    state->regs[fmt.rd] = state->regs[fmt.rs] < state->regs[fmt.rt] ? 1 : 0;
                break;
                case 0x2b: /* sltu (R) */
                    state->regs[fmt.rd] = (unsigned)state->regs[fmt.rs] < (unsigned)state->regs[fmt.rt] ? 1 : 0;
                break;
                case 0x30: /* tge (R) */
                    if (state->regs[fmt.rs] >= state->regs[fmt.rt])
                        state->exception = 1;
                break;
                case 0x31: /* tgeu (R) */
                    if ((unsigned)state->regs[fmt.rs] >= (unsigned)state->regs[fmt.rt])
                        state->exception = 1;
                break;
                case 0x32: /* tlt (R) */
                    if (state->regs[fmt.rs] < state->regs[fmt.rt])
                        state->exception = 1;
                break;
                case 0x33: /* tltu (R) */
                    if ((unsigned)state->regs[fmt.rs] < (unsigned)state->regs[fmt.rt])
                        state->exception = 1;
                break;
                case 0x34: /* teq (R) */
                    if (state->regs[fmt.rs] == state->regs[fmt.rt])
                        state->exception = 12;
                break;
                case 0x36: /* tne (R) */
                    if (state->regs[fmt.rs] != state->regs[fmt.rt])
                        state->exception = 12;
                break;
            }
        break;
        case 0x01: /* bgez/bltz (I) */
            if (state->regs[ifmt.rt] == 0) /* bltz */
            {
                if (state->regs[ifmt.rs] < 0)
                    state->pc += (ifmt.immediate << 2) - 4;
            }
            else if (state->regs[ifmt.rt] == 1) /* bgez */
            {
                if (state->regs[ifmt.rs] >= 0)
                    state->pc += (ifmt.immediate << 2) - 4;
            }
        break;
        case 0x02: /* j (J) */
            state->pc = jfmt.address - 4;
        break;
        case 0x03: /* jal (J) */
            state->pc = jfmt.address - 4;
            state->regs[31] = state->pc + 4;
        break;
        case 0x04: /* beq (I) */
            if (state->regs[ifmt.rs] == state->regs[ifmt.rt])
                state->pc += (ifmt.immediate << 2) - 4;
        break;
        case 0x05: /* bne (I) */
            if(state->regs[ifmt.rs] != state->regs[ifmt.rt])
                state->pc += (ifmt.immediate << 2) - 4;
        break;
        case 0x06: /* blez (I) */
            if (state->regs[ifmt.rs] <= 0)
                state->pc += (ifmt.immediate << 2) - 4;
        break;
        case 0x07: /* bgtz (I) */
            if (state->regs[ifmt.rs] > 0)
                state->pc += (ifmt.immediate << 2) - 4;
        break;
        case 0x08: /* addi (I) */
            state->regs[ifmt.rt] = state->regs[ifmt.rs] + ifmt.immediate;
        break;
        case 0x09: /* addiu (I) */
            state->regs[ifmt.rt] = (unsigned)state->regs[ifmt.rs] + ifmt.immediate;
        break;
        case 0x0a: /* slti (I) */
            state->regs[ifmt.rt] = state->regs[ifmt.rs] < ifmt.immediate ? 1 : 0;
        break;
        case 0x0b: /* sltiu (I) */
            state->regs[ifmt.rt] = (unsigned)state->regs[ifmt.rs] < ifmt.immediate ? 1 : 0;
        break;
        case 0x0c: /* andi (I) */
            state->regs[ifmt.rt] = state->regs[ifmt.rs] & ifmt.immediate;
        break;
        case 0x0d: /* ori (I) */
            state->regs[ifmt.rt] = state->regs[ifmt.rs] | ifmt.immediate;
        break;
        case 0x0e: /* xori (I) */
            state->regs[ifmt.rt] = state->regs[ifmt.rs] ^ ifmt.immediate;
        break;
        case 0x0f: /* lui (I) */
            state->regs[ifmt.rt] = ifmt.immediate << 16;
            state->regs[ifmt.rt] &= 0xffff;
        break;
        case 0x10: /* mfc0/mtc0 (R) */
            if (state->regs[fmt.rs] == 0) /* mfc0 */
                state->regs[fmt.rt] = state->cp0regs[fmt.rd];
            else if (state->regs[fmt.rs] == 4 &&  /* mtc0 (only kernel can write to certain cp0 registers) */
            (((state->regs[fmt.rd] == 0 || state->regs[fmt.rd] == 1 || state->regs[fmt.rd] == 2 || state->regs[fmt.rd] == 4 || state->regs[fmt.rd] == 8 || 
            state->regs[fmt.rd] == 10 || state->regs[fmt.rd] == 12 || state->regs[fmt.rd] == 13 || state->regs[fmt.rd] == 14 || state->regs[fmt.rd] == 15) && state->mode == 1) || state->mode == 0))
                state->cp0regs[fmt.rd] = state->regs[fmt.rt];
        break;
        case 0x18: /* eret (I) */
            state->exception = 0;
            state->pc = state->cp0regs[14];
            state->cp0regs[14] = 0;
        break;
        case 0x20: /* lb (I) */
            state->regs[ifmt.rt] = loadmemb(mem, state->regs[ifmt.rs] + ifmt.immediate);
        break;
        case 0x21: /* lh (I) */
            state->regs[ifmt.rt] = loadmemh(mem, state->regs[ifmt.rs] + ifmt.immediate);
        break;
        case 0x22: /* lwl (I) */
            state->regs[ifmt.rt] = (loadmemw(mem, (state->regs[ifmt.rs] + ifmt.immediate & 0xfffffffc)) << (8 * (3 - (state->regs[ifmt.rs] + ifmt.immediate & 0xfffffffc) & 0x03)));
        break;
        case 0x23: /* lw (I) */
            state->regs[ifmt.rt] = loadmemw(mem, state->regs[ifmt.rs] + ifmt.immediate);
        break;
        case 0x24: /* lbu (I) */
            state->regs[ifmt.rt] = loadmembu(mem, state->regs[ifmt.rs] + ifmt.immediate);
        break;
        case 0x25: /* lhu (I) */
            state->regs[ifmt.rt] = loadmemhu(mem, state->regs[ifmt.rs] + ifmt.immediate);
        break;
        case 0x26: /* lwr (I) */
            state->regs[ifmt.rt] = (loadmemw(mem, (state->regs[ifmt.rs] + ifmt.immediate & 0xfffffffc)) >> (8 * ((state->regs[ifmt.rs] + ifmt.immediate & 0xfffffffc) & 0x03)));
        break;
        case 0x28: /* sb (I) */
            storememb(mem, state->regs[ifmt.rs] + ifmt.immediate, state->regs[ifmt.rt]);
        break;
        case 0x29: /* sh (I) */
            storememh(mem, state->regs[ifmt.rs] + ifmt.immediate, state->regs[ifmt.rt]);
        break;
        case 0x2b: /* sw (I) */
            storememw(mem, state->regs[ifmt.rs] + ifmt.immediate, state->regs[ifmt.rt]);
        break;
    }

    /* Coprocessor 0 parsing */
    if ((state->cp0regs[12] & 0x01) == 0)
    {
        int interrupts = 0;
        for (interrupts = 0; interrupts < 8; interrupts++)
            state->interrupts[interrupts] = 0xff;
    }
    else
    {
        int interrupts = 0;
        for (interrupts = 0; interrupts < 8; interrupts++)
            state->interrupts[interrupts] = 0x00;
    }

    state->mode = (state->cp0regs[12] & 0x05) >> 4;

    state->cp0regs[9] = count;

    /* Check timer */
    if (state->cp0regs[9] == state->cp0regs[11])
    {
        /* if timer count == timer compare then interrupt */
        interrupt_handler(state, 8, 0);
    }

    /* Check for exceptions */
    if (state->exception != 0)
    {
        /* exception occured */
        interrupt_handler(state, 0, state->exception);
    }

    state->regs[0] = 0;

    state->pc += 4;

    return instruction;
}

#endif
