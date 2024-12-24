#include <stdio.h>
#include <stdlib.h>
#include <signal.h> /* POSIX only! */
#include <sys/time.h> /* Linux only! */

unsigned int ldmem(size_t size, unsigned char *m, unsigned int a);
void stmem(size_t size, unsigned char *m, unsigned int a, unsigned int d);

#define LDMEM(r, s, m, a) r = ldmem(s, m, a)
#define STMEM(s, m, a, d) stmem(s, m, a, d)
#define CPRINT(c) printf("%c", c)
#define SPRINT(s) {int i=0; while (mem[i+s]!='\0') {printf("%c", mem[i+s]); i++;}}
/* #define SPRINT(s) printf("%x", s) */
#define IPRINT(i) printf("%u", i)
#define SREAD(b, l) fread((char *)b, 1, l, stdin)
#define IREAD(i) scanf("%u", &i)
#include "mips.h"

MIPS_state state;

unsigned char *mem;

const char regname[33][5] = {"pc", "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"};
const char cp0regname[32][10] = {"cp0", "cp1", "cp2", "cp3", "cp4", "cp5", "cp6", "cp7", "cp8", "count", "cp10", "compare", "status", "cause", "epc", "cp15", "cp16", "cp17", "cp18", "cp19", "cp20", "cp21", "cp22", "cp23", "cp24", "cp25", "cp26", "cp27", "cp28", "cp29", "cp30", "cp31"};

void hexDump(char *desc, void *addr, int len) 
{
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    /* Output description if given. */
    if (desc != NULL)
        printf ("%s:\n", desc);

    /* Process every byte in the data. */
    for (i = 0; i < len; i++) {
        /* Multiple of 16 means new line (with line offset). */

        if ((i % 16) == 0) {
            /* Just don't print ASCII for the zeroth line. */
            if (i != 0)
                printf("  %s\n", buff);

            /* Output the offset. */
            printf("  %04x ", i);
        }

        /* Now the hex code for the specific character. */
        printf(" %02x", pc[i]);

        /* And store a printable ASCII character for later. */
        if ((pc[i] < 0x20) || (pc[i] > 0x7e)) {
            buff[i % 16] = '.';
        } else {
            buff[i % 16] = pc[i];
        }

        buff[(i % 16) + 1] = '\0';
    }

    /* Pad out last line if not exactly 16 characters. */
    while ((i % 16) != 0) {
        printf("   ");
        i++;
    }

    /* And print the final ASCII bit. */
    printf("  %s\n", buff);
}

void print_state()
{
    int i;

    printf("\n");

    printf("--------PROCESSOR REGISTERS--------\n");
    for (i = 0; i < 32; i++)
        printf("$%s: %u|0x%x\n", regname[i + 1], state.regs[i], state.regs[i]);
    printf("$%s: %u|0x%x\n\n", regname[0], state.pc, state.pc);
    printf("--------COPROCESSOR 0 REGISTERS--------\n");
    for (i = 0; i < 32; i++)
        printf("$%s: %u|0x%x\n", cp0regname[i], state.cp0regs[i], state.cp0regs[i]);
    
    printf("\n--------MEMORY--------\n");
    hexDump((char *)"FIRST KILOBYTE", mem + 0x20000000, 0x400);
}

void exit_handler(int sig)
{
    print_state();
}

int main(int argc, char *argv[])
{
    int i;

    if (argc < 2)
    {
        printf("Usage: %s <input file>\n", argv[0]);
        return 1;
    }

    signal(SIGINT, exit_handler);

    FILE *fp = fopen(argv[1], "rb");

    if (fp == NULL)
    {
        printf("Failed to open file: %s\n", argv[1]);
        return 1;
    }

    fseek(fp, 0, SEEK_END);

    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    mem = (unsigned char *)malloc(0x40000000); /* 512MB rom + 512MB ram */

    if (mem == NULL)
    {
        printf("Failed to allocate memory\n");
        fclose(fp);
        return 1;
    }

    fread(mem, 1, size, fp);

    fclose(fp);

    for (i = 0; i < 32; i++)
        state.regs[i] = 0;
    for (i = 0; i < 32; i++)
        state.cp0regs[i] = 0;

    state.pc = 0;
    state.regs[29] = (1 << 24) - 4096;
    state.cp0regs[11] = 0x00ff0000;
    state.cp0regs[12] = 0x0000ff01;

    unsigned int instruction = 0;
    unsigned int count = 0;

    struct timeval tv;
    gettimeofday(&tv, NULL);

    count = tv.tv_usec / 1000;

    for (i = 0; i < 0x40000000; i++)
    {
        instruction = loadmemw(mem, state.pc);

        if (execute(&state, instruction, mem, count) == 5)
            break;

        /* synchronize timer */
        count = tv.tv_usec / 1000;
        /* printf("%x\n", instruction); */
        /* printf("$%x: %x%x%x%x\n", state.pc, mem[state.pc], mem[state.pc + 1], mem[state.pc + 2], mem[state.pc + 3]); */
        /* printf("%x\n", execute(&state, instruction, mem)); */
    }

    print_state();

    free(mem);

    return 0;
}

unsigned int ldmem(size_t size, unsigned char *m, unsigned int a)
{
    if (a == 0x1000000)
        return getchar();
    return 0;
}

void stmem(size_t size, unsigned char *m, unsigned int a, unsigned int d)
{
    if (a == 0x1000000)
        printf("%c", (char)d);
}