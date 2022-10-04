#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/mman.h>


enum   //registers
{
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC, // program counter
    
    R_COND,
    R_COUNT
};

enum
{
    FL_POS =  1<<0, 
    FL_ZRO =  1<<1,
    FL_NEG =  1<<2,
};
enum
{
    OP_BR  = 0 , 
    OP_ADD,
    OP_LD,
    OP_ST,
    OP_JSR,
    OP_AND,
    OP_LDR,
    OP_STR,
    OP_RTI,
    OP_NOT,
    OP_LDI,
    OP_STI,
    OP_JMP,
    OP_RES,
    OP_LEA,
    OP_TRAP   
};

enum 
{
    MR_KBSR = 0xFE00,
    MR_KBDR = 0xFE02
};

enum
{
    TRAP_GETC = 0x20,
    TRAP_OUT  = 0x21,
    TRAP_PUTS = 0x22,
    TRAP_IN = 0x23,
    TRAP_PUTSP  = 0x24,
    TRAP_HALT = 0x25
};

#define MEMORY_MAX (1<<16)
uint16_t memory[MEMORY_MAX];
uint16_t reg[R_COUNT];

struct termios original_tio;

void disable_input_buffering()
{
    tcgetattr(STDIN_FILENO,&original_tio);
    struct termios new_tio = original_tio;
    new_tio.c_lflag &= ~ICANON & ~ECHO;
    tcsetattr(STDIN_FILENO,TCSANOW,&new_tio);    
}

void restore_input_buffering()
{
    tcsetattr(STDIN_FILENO,TCSANOW,&original_tio);
}

uint16_t check_key() 
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO,&readfds);

    struct timeval timeout;
    timeout.tv_sec = 0 ;
    timeout.tv_usec = 0;
    return select(1,&readfds,NULL,NULL,&timeout) != 0;
}

void handle_interrupt(int signal)
{
    restore_input_buffering();
    printf("\n");
    exit(-2);
}

uint16_t sign_extend(uint16_t x,int bit_count)
{
    if((x>>(bit_count-1))&1){
        x |=(0xFFFF << bit_count);
    }
    
    return x ;
}

uint16_t swap16(uint16_t x) 
{
    return (x<<8)|(x>>8)
}

void update_flags(uint16_t r) 
{
    if(reg[r] == 0)
    {
        reg[R_COND] = FL_ZRO;
    }
    else if (reg[r]>>15)
    {
        reg[R_COND] = FL_NEG;
    }
    else
    {
        reg[R_COND] = FL_POS;
    }
}

void  read_image_file(FILE* file)
{
    uint16_t origin;
    fread(&origin,sizeof(origin),1,file);
    origin = swap16(origin);

    uint16_t max_read = MEMORY_MAX - origin;
    uint16_t* p = memory + origin;
    size_t read = fread(p,sizeof(uint16_t),max_read,file);

    while(read--> 0)
    {
        *p = swap16(*p);
        ++p;
    }
}

int read_image(const char* image_path)
{
    FILE* file  = fopen(image_path,"rb");
    if (!file) {return 0;};
    read_image_file(file);
    fclose(file);
    return 1 ;
}

void mem_write(uint16_t address,uint16_t val)
{
    memory[address] = val;
}

uint16_t mem_read(uint16_t address)
{
    if(address == MR_KBSR)
    {
        if (check_key())
        {
            memory[MR_KBSR] = (1 << 15 );
            memory[MR_KBDR] = getchar();
        }
        else
        {
            memory[MR_KBSR] = 0 ;
        }
    }
    return memory[address];
} 