#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emmintrin.h>

#include "rdtsc.h"

#include "insn.h"

#define MAGIC 0x00D3C0DE

#define MAX_INSN 32 * 1024

//
#define EXEC_DONE  0

#define EXEC_HALT  1
#define UNKN_INSN  2

#define MAX_STR_LEN 256

//
typedef unsigned char byte;

//
typedef unsigned mem32; //32bit type

//Header
mem32 _header_;
mem32 _cs_offset_;
mem32 _insn_addr_;

//Code memory (SRAM0)
mem32 _code_mem_bank_[MAX_INSN];

//Instruction pointer
mem32 IP;
mem32 nb_strings;

//
mem32 check_magic(mem32 m, byte *csec, byte *ssec)
{
  mem32 magic_number = (m & 0x00FFFFFF);
  
  *csec = (m & 0x80000000) >> 31;
  *ssec = (m & 0x40000000) >> 30;
  
  return !(MAGIC ^ magic_number);
}

//
mem32 insn_decode(mem32 insn, byte *opcode, byte *opd1, byte *opd2, byte *opd3, byte *mbank)
{  
 *opcode = (insn & 0xFF000000) >> 24;

 *mbank = (insn & 0x00080000) >> 19;
 *opd1  = (insn & 0x00003C00) >> 10;
 *opd2  = (insn & 0x000003E0) >>  5;
 *opd3  = (insn & 0x0000001F);
 
 return 0;
}


//
mem32 insn_execute(mem32 opcode, byte opd1, byte opd2, byte opd3, byte mbank)
{
  switch (opcode)
    {      
      //3 registers
      //Arithmetic
    case ADD:      
    case SUB:
    case MUL:
    case DIV:
    case MOD:
    case FMA:
    case FMS:

      //Logic
    case OR:
    case AND:
    case XOR:    
    case SHL:
    case SHR:
    case ROL:
    case ROR:
      printf("._0x%08x\n\t%s\tr%u, r%u, r%u\n", IP, _opcodes_table_lower_[opcode], opd1, opd2, opd3);
      IP++;
      return EXEC_DONE;

      //3 vector registers 
      //Vector 
    case ADDV:
    case SUBV:
    case DIVV:
    case MODV:
    case FMAV:
    case FMSV:
    case ANDV:      
    case ORV:
    case XORV:
    case SHLV:
    case SHRV:
    case ROLV:
    case RORV:
      printf("._0x%08x\n\t%s\tv%u, v%u, v%u\n", IP, _opcodes_table_lower_[opcode], opd1, opd2, opd3);
      IP++;
      return EXEC_DONE;

      //2 vector registers
    case NOTV:
      printf("._0x%08x\n\t%s\tv%u, v%u\n", IP, _opcodes_table_lower_[opcode], opd1, opd2);
      IP++;
      return EXEC_DONE;


      //Single register
    case TSC:
    case INC:
    case DEC:
    case RAND:
    case PRNTW:
    case PRNTB:
    case PRNTX:
      printf("._0x%08x\n\t%s\tr%u\n", IP, _opcodes_table_lower_[opcode], opd1);
      IP++;
      return EXEC_DONE;

      //2 registers  
    case MOV:
    case NOT:
    case LMB:
    case INV:      
    case REV:
    case CMP: 
    case PCNT:      
      printf("._0x%08x\n\t%s\tr%u, r%u\n", IP, _opcodes_table_lower_[opcode], opd1, opd2);
      IP++;
      return EXEC_DONE;

      //Control
    case JMP:
    case JEQ:
    case JNE:
    case JGT:
    case JLT:
    case JGE:
    case JLE:
      printf("._0x%08x\n\t%s\t_0x%08x\n", IP, _opcodes_table_lower_[opcode], _code_mem_bank_[IP + 1]);
      IP += 2;
      return EXEC_DONE;

    case LOADB:
    case LOADW:
    case LOADC:
      printf("._0x%08x\n\t%s\tr%u, %u(r%u, r%u)\n", IP, _opcodes_table_lower_[opcode], opd1, mbank, opd2, opd3);
      IP++;
      return EXEC_DONE;

    case STORB:
    case STORW:
    case STORC:
      printf("._0x%08x\n\t%s\t%u(r%u, r%u), r%u\n", IP, _opcodes_table_lower_[opcode], mbank, opd1, opd2, opd3);
      IP++;
      return EXEC_DONE;

    case LOADV:
      printf("._0x%08x\n\t%s\tv%u, %u(r%u, r%u)\n", IP, _opcodes_table_lower_[opcode], opd1, mbank, opd2, opd3);
      IP++;
      return EXEC_DONE;
      
    case STORV:
      printf("._0x%08x\n\t%s\t%u(r%u, r%u), v%u\n", IP, _opcodes_table_lower_[opcode], mbank, opd1, opd2, opd3);
      IP++;
      return EXEC_DONE;
    
    case MOVW:
      printf("._0x%08x\n\t%s\tr%u, %u\n", IP, _opcodes_table_lower_[opcode], opd1, _code_mem_bank_[IP + 1]);
      IP += 2;
      return EXEC_DONE;

    case MOVB:
      if ((byte)_code_mem_bank_[IP + 1] == '\n')
	printf("._0x%08x\n\t%s\tr%u, '\\n'\n", IP, _opcodes_table_lower_[opcode], opd1, (byte)_code_mem_bank_[IP + 1]);
      else
	if ((byte)_code_mem_bank_[IP + 1] == '\t')
	  printf("._0x%08x\n\t%s\tr%u, '\\t'\n", IP, _opcodes_table_lower_[opcode], opd1, (byte)_code_mem_bank_[IP + 1]);
	else
	  if ((byte)_code_mem_bank_[IP + 1] == '\\')
	    printf("._0x%08x\n\t%s\tr%u, '\\'\n", IP, _opcodes_table_lower_[opcode], opd1, (byte)_code_mem_bank_[IP + 1]);
	  else
	    if ((byte)_code_mem_bank_[IP + 1] == '\"')
	      printf("._0x%08x\n\t%s\tr%u, '\\t'\n", IP, _opcodes_table_lower_[opcode], opd1, (byte)_code_mem_bank_[IP + 1]);
	    else
	      if ((byte)_code_mem_bank_[IP + 1] == '\'')
		printf("._0x%08x\n\t%s\tr%u, '\''\n", IP, _opcodes_table_lower_[opcode], opd1, (byte)_code_mem_bank_[IP + 1]);
	      else
		printf("._0x%08x\n\t%s\tr%u, '%c'\n", IP, _opcodes_table_lower_[opcode], opd1, (byte)_code_mem_bank_[IP + 1]);
      
      IP += 2;
      return EXEC_DONE;
      
    case PRNTS:
      printf("._0x%08x\n\t%s\t_%u\n", IP, _opcodes_table_lower_[opcode], _code_mem_bank_[IP + 1], _code_mem_bank_[IP + 1]);
      IP += 2;
      return EXEC_DONE;
      
    case REDV:
      printf("._0x%08x\n\t%s\tr%u, r%v\n", IP, _opcodes_table_lower_[opcode], opd1, opd2);
      IP++;
      return EXEC_DONE;
      
    case HLT:
      printf("._0x%08x\n\t%s\n", IP, _opcodes_table_lower_[opcode]);
      return EXEC_HALT;
            
    default:
      return UNKN_INSN;
    }
}

//Clock + Dispatch unit
mem32 run_code()
{
  mem32 insn;
  mem32 ret_dec, ret_exe;
  byte opcode, opd1, opd2, opd3, mbank;

  printf("#[_CODE_SECTION_]\n");

  do
    {
      insn = _code_mem_bank_[IP];
      
      ret_dec = insn_decode(insn, &opcode, &opd1, &opd2, &opd3, &mbank);
      
      ret_exe = insn_execute(opcode, opd1, opd2, opd3, mbank);
    }
  while (ret_exe == EXEC_DONE);

  return ret_exe;
}

//Strings table loader
mem32 load_strings(FILE *fd)
{
  byte len;
  byte string[MAX_STR_LEN];
  
  fread(&nb_strings, sizeof(mem32), 1, fd);
  printf("#[_STRINGS_SECTION_]\n");
  printf("#Number of strings: %u\n\n", nb_strings); 
  
  for (mem32 i = 0; i < nb_strings; i++)
    {
      fread(&len, sizeof(byte), 1, fd);
      fread(string, sizeof(byte), len, fd);
      string[len] = 0;
      printf("#%u\tlen:%u str:\"%s\"\n", i, len, string);

      printf("@ _%u \"%s\"\n\n", i, string);
    }

  printf("\n");
  
  return 0;
}
	
//Code loader 
mem32 load_code(FILE *fd)
{ return fread(_code_mem_bank_, sizeof(mem32), MAX_INSN, fd); }

//Header loader: 1 byte for flags & 3 bytes magic number
mem32 load_header(FILE *fd)
{ return fread(&_header_, sizeof(mem32), 1, fd); }

//Entry point
int main(int argc, char **argv)
{  
  IP = 0;
  _insn_addr_ = 0;
  
  if (argc < 2)
    return printf("OUPS: %s [binary file]\n", argv[0]), -1;
  
  byte csec, ssec;
  FILE *fd = fopen(argv[1], "rb");
  
  if (fd == NULL)
    return printf("OUPS: File %s cannot be loaded!\n", argv[1]), -2;
  
  load_header(fd);
  
  if (!check_magic(_header_, &csec, &ssec))
    return printf("Incompatible binary header!\n"), -3;

  //Read CODE SCETION OFFSET
  fread(&_cs_offset_, sizeof(mem32), 1, fd);

  if (ssec)
    load_strings(fd);
  
  if (csec)
    load_code(fd);
  
  fclose(fd);
      
  //
  run_code();
    
  return 0;
}
