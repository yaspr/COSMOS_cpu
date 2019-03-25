#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emmintrin.h>

#include "rdtsc.h"

#include "insn.h"

#define MAGIC 0x00D3C0DE

#define MAX_R  32
#define MAX_V  512 //(4 x 4B = 16B = 128b - register length) * 32 registers = 512B

#define VEC_LEN_I 4

//128KB
#define MAX_INSN  32 * 1024 
#define MAX_DATA  32 * 1024

//
#define EXEC_DONE  0

#define EXEC_HALT  1
#define UNKN_INSN  2
#define ZDIV_ERR   3
#define MEM_OOB    4 //Memory out of bounds

#define MAX_STR_LEN 256
#define MAX_NB_STR  1024

//
typedef unsigned char byte;

//
typedef unsigned mem32; //32bit type

//Header
mem32 _header_;
mem32 _cs_offset_;

//Code memory (SRAM0)
mem32 _code_mem_bank_[MAX_INSN];

//Data memory (SRAM1 & SRAM2)
mem32 _data_mem_bank0_[MAX_DATA]; //128KB
mem32 _data_mem_bank1_[MAX_DATA]; //128KB

//General purpose registers
mem32 R[MAX_R];     //32 x 32bit registers
mem32 V[MAX_V];     //32 x 128bit registers (32bits * 4 * 32) 

//String table
mem32 nb_strings;
byte S[MAX_NB_STR][MAX_STR_LEN];

//Instruction pointer
mem32 IP;

//Flags file
byte FF; //IF GE LE GT LT ZF CF ET 

//
mem32 randxy(mem32 x, mem32 y)
{ return (x + rand()) % y; }

//
byte is_p2(mem32 v)
{
  return v && !(v & (v - 1));
}

//
mem32 _rev_(mem32 v)
{
  v = ((v >> 1) & 0x55555555) | ((v & 0x55555555) << 1);
  v = ((v >> 2) & 0x33333333) | ((v & 0x33333333) << 2);
  v = ((v >> 4) & 0x0F0F0F0F) | ((v & 0x0F0F0F0F) << 4);
  v = ((v >> 8) & 0x00FF00FF) | ((v & 0x00FF00FF) << 8);
  v = ( v >> 16             ) | ( v               << 16);
  
  return v;
}

//
mem32 _popcnt_(mem32 v)
{
  v = (v & 0x55555555) + ((v >>  1) & 0x55555555); 
  v = (v & 0x33333333) + ((v >>  2) & 0x33333333); 
  v = (v & 0x0F0F0F0F) + ((v >>  4) & 0x0F0F0F0F); 
  v = (v & 0x00FF00FF) + ((v >>  8) & 0x00FF00FF); 
  v = (v & 0x0000FFFF) + ((v >> 16) & 0x0000FFFF);
  
  return v;
}

//
mem32 _lmb_(mem32 v)
{
  mem32 n = v;
  mem32 pos = 0;
  
  while (n)
    n >>= 1, pos++;
  
  return pos;
}

//
mem32 _rotl_(mem32 value, mem32 shift)
{
  if ((shift &= ((sizeof(value) << 3) - 1)) == 0)
    return value;
  
  return (value << shift) | (value >> ((sizeof(value) << 3) - shift));
}

//
mem32 _rotr_(mem32 value, mem32 shift)
{
  if ((shift &= ((sizeof(value) << 3) - 1)) == 0)
    return value;
  
  return (value >> shift) | (value << ((sizeof(value) << 3) - shift));
}

//
mem32 _vadd_(byte opd1, byte opd2, byte opd3)
{  
  for (mem32 i = 0; i < VEC_LEN_I; i++)
    V[(opd1 << 2) + i] = V[(opd2 << 2) + i] + V[(opd3 << 2) + i];
  
  return 0;
}

//
mem32 _vsub_(byte opd1, byte opd2, byte opd3)
{  
  for (mem32 i = 0; i < VEC_LEN_I; i++)
    V[(opd1 << 2) + i] = V[(opd2 << 2) + i] - V[(opd3 << 2) + i];
  
  return 0;
}

//
mem32 _vmul_(byte opd1, byte opd2, byte opd3)
{  
  for (mem32 i = 0; i < VEC_LEN_I; i++)
    V[(opd1 << 2) + i] = V[(opd2 << 2) + i] * V[(opd3 << 2) + i];
  
  return 0;
}

//
mem32 _vdiv_(byte opd1, byte opd2, byte opd3)
{  
  for (mem32 i = 0; i < VEC_LEN_I; i++)
    V[(opd1 << 2) + i] = V[(opd2 << 2) + i] / V[(opd3 << 2) + i];
  
  return 0;
}

//
mem32 _vmod_(byte opd1, byte opd2, byte opd3)
{  
  for (mem32 i = 0; i < VEC_LEN_I; i++)
    V[(opd1 << 2) + i] = V[(opd2 << 2) + i] % V[(opd3 << 2) + i];
  
  return 0;
}

//
mem32 _vfma_(byte opd1, byte opd2, byte opd3)
{  
  for (mem32 i = 0; i < VEC_LEN_I; i++)
    V[(opd1 << 2) + i] += V[(opd2 << 2) + i] * V[(opd3 << 2) + i];
  
  return 0;
}

//
mem32 _vfms_(byte opd1, byte opd2, byte opd3)
{  
  for (mem32 i = 0; i < VEC_LEN_I; i++)
    V[(opd1 << 2) + i] += V[(opd2 << 2) + i] * V[(opd3 << 2) + i];
  
  return 0;
}

//
mem32 _vand_(byte opd1, byte opd2, byte opd3)
{  
  for (mem32 i = 0; i < VEC_LEN_I; i++)
    V[(opd1 << 2) + i] = V[(opd2 << 2) + i] & V[(opd3 << 2) + i];
  
  return 0;
}

//
mem32 _vor_(byte opd1, byte opd2, byte opd3)
{  
  for (mem32 i = 0; i < VEC_LEN_I; i++)
    V[(opd1 << 2) + i] = V[(opd2 << 2) + i] | V[(opd3 << 2) + i];
  
  return 0;
}

//
mem32 _vxor_(byte opd1, byte opd2, byte opd3)
{  
  for (mem32 i = 0; i < VEC_LEN_I; i++)
    V[(opd1 << 2) + i] = V[(opd2 << 2) + i] ^ V[(opd3 << 2) + i];
  
  return 0;
}

//
mem32 _vnot_(byte opd1, byte opd2)
{  
  for (mem32 i = 0; i < VEC_LEN_I; i++)
    V[(opd1 << 2) + i] = !V[(opd2 << 2) + i];
  
  return 0;
}

//
mem32 _vshl_(byte opd1, byte opd2, byte opd3)
{  
  for (mem32 i = 0; i < VEC_LEN_I; i++)
    V[(opd1 << 2) + i] = V[(opd2 << 2) + i] << V[(opd3 << 2) + i];
  
  return 0;
}

//
mem32 _vshr_(byte opd1, byte opd2, byte opd3)
{  
  for (mem32 i = 0; i < VEC_LEN_I; i++)
    V[(opd1 << 2) + i] = V[(opd2 << 2) + i] >> V[(opd3 << 2) + i];
  
  return 0;
}

//
mem32 _vrol_(byte opd1, byte opd2, byte opd3)
{  
  for (mem32 i = 0; i < VEC_LEN_I; i++)
    V[(opd1 << 2) + i] = _rotl_(V[(opd2 << 2) + i], V[(opd3 << 2) + i]);
  
  return 0;
}

//
mem32 _vror_(byte opd1, byte opd2, byte opd3)
{  
  for (mem32 i = 0; i < VEC_LEN_I; i++)
    V[(opd1 << 2) + i] = _rotr_(V[(opd2 << 2) + i], V[(opd3 << 2) + i]);
  
  return 0;
}

//
mem32 _redv_(byte opd1, byte opd2)
{
  R[opd1] += V[(opd2 << 2)];
  R[opd1] += V[(opd2 << 2) + 1];
  R[opd1] += V[(opd2 << 2) + 2];
  R[opd1] += V[(opd2 << 2) + 3];
  
  return 0;
}

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
      
      //Arithmetic
    case ADD:
      R[opd1] = R[opd2] + R[opd3];
      IP++;
      return EXEC_DONE;
      
    case SUB:
      R[opd1] = R[opd2] - R[opd3];
      IP++;
      return EXEC_DONE;
      
    case MUL:
      R[opd1] = R[opd2] * R[opd3];
      IP++;
      return EXEC_DONE;

    case DIV:
      if (R[opd3])
	{
	  R[opd1] = R[opd2] / R[opd3];
	  IP++;
	  return EXEC_DONE;
	}
      else
	{
	  FF |= (0x80);  //Set interrupt flag
	  return ZDIV_ERR;
	}
      
    case MOD:
      if (R[opd3])
	{
	  R[opd1] = R[opd2] % R[opd3];
	  IP++;
	  return EXEC_DONE;
	}
      else
	{
	  FF |= (0x80);  //Set interrupt flag
	  return ZDIV_ERR;
	}
      
    case INC:
      R[opd1] = R[opd1] + 1;
      IP++;
      return EXEC_DONE;

    case DEC:
      R[opd1] = R[opd1] - 1;
      IP++;
      return EXEC_DONE;

    case FMA:
      R[opd1] += R[opd2] * R[opd3];
      IP++;
      return EXEC_DONE;

    case FMS:
      R[opd1] -= R[opd2] * R[opd3];
      IP++;
      return EXEC_DONE;

      //Vector
    case ADDV:
      _vadd_(opd1, opd2, opd3);
      IP++;
      return EXEC_DONE;
      
    case SUBV:
      _vsub_(opd1, opd2, opd3);
      IP++;
      return EXEC_DONE;
      
    case DIVV:
      _vdiv_(opd1, opd2, opd3);
      IP++;
      return EXEC_DONE;
      
    case MODV:
      _vmod_(opd1, opd2, opd3);
      IP++;
      return EXEC_DONE;

    case FMAV:
      _vfma_(opd1, opd2, opd3);
      IP++;
      return EXEC_DONE;

    case FMSV:
      _vfms_(opd1, opd2, opd3);
      IP++;
      return EXEC_DONE;

      //Logic
    case AND:
      R[opd1] = R[opd2] & R[opd3];
      IP++;
      return EXEC_DONE;
      
    case OR:
      R[opd1] = R[opd2] | R[opd3];
      IP++;
      return EXEC_DONE;
      
    case XOR:
      R[opd1] = R[opd2] ^ R[opd3];
      IP++;
      return EXEC_DONE;
      
    case NOT:
      R[opd1] = !R[opd2];
      IP++;
      return EXEC_DONE;

    case SHL:
      R[opd1] = R[opd2] << R[opd3];
      IP++;
      return EXEC_DONE;

    case SHR:
      R[opd1] = R[opd2] >> R[opd3];
      IP++;
      return EXEC_DONE;

    case ROL:
      R[opd1] = _rotl_(R[opd2], R[opd3]);
      IP++;
      return EXEC_DONE;
      
    case ROR:
      R[opd1] = _rotr_(R[opd2], R[opd3]);
      IP++;
      return EXEC_DONE;

    case PCNT:
      R[opd1] = _popcnt_(R[opd2]);
      IP++;
      return EXEC_DONE;
      
    case LMB:
      R[opd1] = _lmb_(R[opd2]);
      IP++;
      return EXEC_DONE;

    case INV:
      R[opd1] = ~R[opd2];
      IP++;
      return EXEC_DONE;
      
    case REV:
      R[opd1] = _rev_(R[opd2]);
      IP++;
      return EXEC_DONE;

      //Logic vector
    case ANDV:      
      _vand_(opd1, opd2, opd3);
      IP++;
      return EXEC_DONE;
      
    case ORV:
      _vor_(opd1, opd2, opd3);
      IP++;
      return EXEC_DONE;
      
    case XORV:
      _vxor_(opd1, opd2, opd3);
      IP++;
      return EXEC_DONE;

    case NOTV:
      _vnot_(opd1, opd2);
      IP++;
      return EXEC_DONE;

    case SHLV:
      _vshl_(opd1, opd2, opd3);
      IP++;
      return EXEC_DONE;

    case SHRV:
      _vshr_(opd1, opd2, opd3);
      IP++;
      return EXEC_DONE;

    case ROLV:
      _vrol_(opd1, opd2, opd3);
      IP++;
      return EXEC_DONE;

    case RORV:
      _vror_(opd1, opd2, opd3);
      IP++;
      return EXEC_DONE;

      //Control
    case CMP:  //IF GE LE GT LT EF CF VT
      FF = 0x00;
      FF |= (byte)((R[opd1] >= R[opd2])) << 6;
      FF |= (byte)((R[opd1] <= R[opd2])) << 5;
      FF |= (byte)((R[opd1] > R[opd2]))  << 4;
      FF |= (byte)((R[opd1] < R[opd2]))  << 3;
      FF |= (byte)((R[opd1] == R[opd2])) << 2;
      IP++;
      return EXEC_DONE;

    case JMP:
      IP = _code_mem_bank_[IP + 1];
      return EXEC_DONE;

    case JEQ:
      if ((FF & 0x04))
	IP = _code_mem_bank_[IP + 1];
      else
	IP += 2; //Jump over the 32bit immediate address
      
      return EXEC_DONE;

    case JNE:
      if (!(FF & 0x04))
	IP = _code_mem_bank_[IP + 1];
      else
	IP += 2;
      
      return EXEC_DONE; //Jump over the 32bit immediate address

    case JGT:
      if ((FF & 0x10))
	IP = _code_mem_bank_[IP + 1];
      else
	IP += 2;
      
      return EXEC_DONE; //Jump over the 32bit immediate address
      
    case JLT:
      if ((FF & 0x08))
	IP = _code_mem_bank_[IP + 1];
      else
	IP += 2;
      
      return EXEC_DONE; //Jump over the 32bit immediate address

    case JGE:
      if ((FF & 0x40))
	IP = _code_mem_bank_[IP + 1];
      else
	IP += 2;
      
      return EXEC_DONE; //Jump over the 32bit immediate address

    case JLE:
      if ((FF & 0x20))
	IP = _code_mem_bank_[IP + 1];
      else
	IP += 2;
      
      return EXEC_DONE; //Jump over the 32bit immediate address

    case LOADW:
      
      if ((R[opd2] + R[opd3]) > MAX_DATA)
	return MEM_OOB;

      if (mbank)
	R[opd1] = _data_mem_bank1_[R[opd2] + R[opd3]];
      else
	R[opd1] = _data_mem_bank0_[R[opd2] + R[opd3]];

      IP++;
      return EXEC_DONE;

    case STORW:

      if ((R[opd1] + R[opd2]) > MAX_DATA)
	return MEM_OOB;

      if (mbank)
	_data_mem_bank1_[R[opd1] + R[opd2]] = R[opd3];
      else
	_data_mem_bank0_[R[opd1] + R[opd2]] = R[opd3];
      
      IP++;
      return EXEC_DONE;
      
    case LOADB:

      if ((R[opd2] + R[opd3]) > MAX_DATA)
	return MEM_OOB;

      if (mbank)
	R[opd1] = (byte)_data_mem_bank1_[R[opd2] + R[opd3]];
      else
	R[opd1] = (byte)_data_mem_bank0_[R[opd2] + R[opd3]];

      IP++;
      return EXEC_DONE;

    case STORB:

      if ((R[opd1] + R[opd2]) > MAX_DATA)
	return MEM_OOB;

      if (mbank)
	_data_mem_bank1_[R[opd1] + R[opd2]] = (byte)R[opd3];
      else
	_data_mem_bank0_[R[opd1] + R[opd2]] = (byte)R[opd3];

      IP++;
      return EXEC_DONE;

    case LOADC:

      if ((R[opd2] + R[opd3]) > MAX_DATA)
	return MEM_OOB;

      R[opd1] = _code_mem_bank_[R[opd2] + R[opd3]];
      
      IP++;
      return EXEC_DONE;

    case STORC:

      if ((R[opd1] + R[opd2]) > MAX_DATA)
	return MEM_OOB;

      _code_mem_bank_[R[opd1] + R[opd2]] = R[opd3];
      
      IP++;
      return EXEC_DONE;
      
    case LOADV:
      
      if ((R[opd2] + R[opd3]) > MAX_DATA)
	return MEM_OOB;
      
      if (mbank)
	{
	  for (mem32 i = 0; i < VEC_LEN_I; i++)
	    V[(opd1 << 2) + i] = _data_mem_bank1_[R[opd2] + R[opd3] + i];
	}
      else
	{
	  for (mem32 i = 0; i < VEC_LEN_I; i++)
	    V[(opd1 << 2) + i] = _data_mem_bank0_[R[opd2] + R[opd3] + i];
	}
      
      IP++;
      return EXEC_DONE;
      
    case STORV:
      
      if ((R[opd1] + R[opd2]) > MAX_DATA)
	return MEM_OOB;
      
      if (mbank)
	{
	  for (mem32 i = 0; i < VEC_LEN_I; i++)
	    _data_mem_bank1_[R[opd1] + R[opd2] + i] = V[(opd3 << 2) + i];
	}
      else
	{
	  for (mem32 i = 0; i < VEC_LEN_I; i++)
	    _data_mem_bank0_[R[opd1] + R[opd2] + i] = V[(opd3 << 2) + i];
	}
      
      IP++;
      return EXEC_DONE;
    
    case MOV:
      R[opd1] = R[opd2];
      IP++;
      return EXEC_DONE;
      
    case MOVW:
      R[opd1] = _code_mem_bank_[IP + 1];
      IP += 2;
      return EXEC_DONE;

    case MOVB:
      R[opd1] = (byte)_code_mem_bank_[IP + 1];
      IP += 2;
      return EXEC_DONE;
      
    case PRNTW:
      printf("%u", R[opd1]);
      IP++;
      return EXEC_DONE;

    case PRNTB:
      printf("%c", R[opd1]);
      IP++;
      return EXEC_DONE;

    case PRNTS:
      printf("%s", S[_code_mem_bank_[IP + 1]]);
      IP += 2;
      return EXEC_DONE;

    case PRNTX:
      printf("0x%08x", R[opd1]);
      IP++;
      return EXEC_DONE;
      
    case RAND:
      R[opd1] = randxy(0, 4294967295);
      IP++;
      return EXEC_DONE;
      
    case REDV:
      _redv_(opd1, opd2);
      IP++;
      return EXEC_DONE;
      
    case HLT:
      return EXEC_HALT;
      
    case TSC:
      R[opd1] = (mem32)rdtsc();
      IP++;
      return EXEC_DONE;
      
    default:
      FF |= (0x80);
      return UNKN_INSN;
    }
}

//Clock + Dispatch unit
mem32 run_code()
{
  mem32 insn;
  mem32 ret_dec, ret_exe;
  byte opcode, opd1, opd2, opd3, mbank;
  
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
  
  fread(&nb_strings, sizeof(mem32), 1, fd);
  
  for (mem32 i = 0; i < nb_strings; i++)
    {
      fread(&len, sizeof(byte), 1, fd);
      fread(&S[i], sizeof(byte), len, fd);
    }

  return 0;
}
	
//Code loader 
mem32 load_code(FILE *fd)
{ return fread(_code_mem_bank_, sizeof(mem32), MAX_INSN, fd); }

//Header loader: 1 byte for flags & 3 bytes magic number
mem32 load_header(FILE *fd)
{ return fread(&_header_, sizeof(mem32), 1, fd); }

//
void print_logo()
{
  printf("   _____ ____   _____ __  __  ____   _____ \n"
	 "  / ____/ __ \\ / ____|  \\/  |/ __ \\ / ____|\n"
	 " | |   | |  | | (___ | \\  / | |  | | (___  \n"
	 " | |   | |  | |\\___ \\| |\\/| | |  | |\\___ \\ \n"
	 " | |___| |__| |____) | |  | | |__| |____) |\n"
	 "  \\_____\\____/|_____/|_|  |_|\\____/|_____/ \n\n");
}

//Entry point
int main(int argc, char **argv)
{
  print_logo();

  srand(time(NULL));
  
  IP = 0;

  mem32 ret;
  
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
  ret = run_code();

  if (ret == MEM_OOB)
    return printf("ERROR: Memory out of bounds!\n"), -ret;
  else
    if (ret == UNKN_INSN)
      return printf("ERROR: Unknown instruction!\n"), -ret;
    else
      if (ret == ZDIV_ERR)
	return printf("ERROR: Division by 0!\n"), -ret;
  
  return 0;
}
