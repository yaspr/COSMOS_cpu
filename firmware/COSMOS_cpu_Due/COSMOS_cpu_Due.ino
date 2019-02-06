//Arduino Due version - 512KB Flash + 96KB SRAM

//TODO: handle GPIO 
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "insn.h"

//Header magic number
#define MAGIC 0x00D3C0DE

//Binary instructions transmission done
#define EXEC_CMD  0x5F455845 //_EXEC
#define EXEC_BLNK 0x5F424C4E //_BLN - board instruction
#define EXEC_PING 0x5F504E47  //_PNG

#define MAX_R  32   //32 x 4B = 128B
#define MAX_V  512 //(4B x 4 = 16B  - register length)x 32 registers = 32 x 16B = 512B

#define VEC_LEN_I 4

//Local serial buffer length (4 bytes - 32 bits)
#define MAX_BUFF 4 

//128KB
#define MAX_INSN  2 * 1024 //4K x 4B of memory (4 byte unsigned)
#define MAX_DATA  2 * 1024 //4K x 4B of memory (4 byte unsigned)

//
#define EXEC_DONE  0
#define ZDIV_ERR   1
#define UNKN_INSN  2
#define EXEC_HALT  3
#define MEM_OOB    4 //Memory out of bounds

#define MAX_STR_LEN 64
#define MAX_NB_STR  32

//
typedef unsigned char byte;

//
typedef unsigned long mem32; //32bit type

//Header
mem32 _header_;
mem32 _cs_offset_ = 0;

//
mem32 nb_insn = 0;
//Code memory (SRAM0) 
mem32 _code_mem_bank_ptr_[MAX_INSN];   //4KB   

//Instruction list entry (after header & strings)
mem32 *_code_mem_bank_  = NULL;

//Strings entry: nb_strings (4B) len0 (1B) string len1 (1B) string ...
byte *_code_mem_bank_str_ = NULL;

//Data memory (SRAM1 & SRAM2)
mem32 _data_mem_bank0_[MAX_DATA]; //8KB
mem32 _data_mem_bank1_[MAX_DATA]; //8KB

//General purpose registers
mem32 R[MAX_R];     //32 x 32bit registers = 128B
mem32 V[MAX_V];     //4 x 128bit registers ((32bits * 4) * 16) = 256B

//String table
mem32 nb_strings = 0;
char S[MAX_NB_STR][MAX_STR_LEN]; // 32 * 64 = 2^11 = 2048 = 2KB 

//Instruction pointer
mem32 IP;

//
char buff[MAX_BUFF];

//Flags file
byte FF; //IF GE LE GT LT ZF CF ET 

//Is power of 2
byte is_p2(mem32 v)
{ return v && !(v & (v - 1)); }

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
  byte i = 0;
  
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
      
    case DECR:
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
      Serial.flush();
      Serial.print((mem32)R[opd1]);
      Serial.flush();
      IP++;
      return EXEC_DONE;

    case PRNTB:
      Serial.flush();
      Serial.print((char)R[opd1]);
      Serial.flush();
      IP++;
      return EXEC_DONE;

    case PRNTS:
      Serial.flush();
      Serial.print((char *)S[_code_mem_bank_[IP + 1]]); 
      Serial.flush();
      IP += 2;
      return EXEC_DONE;

    case PRNTX:
      Serial.flush();
      Serial.print((mem32)R[opd1], HEX);
      Serial.flush();
      IP++;
      return EXEC_DONE;
      
    case RAND:
      R[opd1] = random(0, 1000000000);
      IP++;
      return EXEC_DONE;

    case REDV:
      _redv_(opd1, opd2);
      IP++;
      return EXEC_DONE;
      
    case HLT:
      return EXEC_HALT;
      
    case TSC:
      R[opd1] = (mem32)micros();
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
mem32 load_strings()
{
  mem32 idx = 0, k;
     
  nb_strings =  _code_mem_bank_ptr_[2];
        
  for (mem32 i = 0; i < nb_strings; i++)
    {
      k = 0;
      //Read string length
      byte nb_chars = _code_mem_bank_str_[idx++];
         
      for (byte j = idx; j < nb_chars + idx; j++, k++)
	S[i][k] = (char)_code_mem_bank_str_[j];
   
      S[i][k] = 0; //End of the string
         
      idx += nb_chars;
    }
}
  
//Header loader: 1 byte for flags & 3 bytes magic number
mem32 load_header()
{  _header_ = _code_mem_bank_ptr_[0]; }

//Code section offset loader
//Pointer madness
mem32 load_cs_offset()
{   
  _cs_offset_ = _code_mem_bank_ptr_[1]; 
  
  //_cs_offset_ / offset is in bytes ==> convert, displace, convert back
  _code_mem_bank_ = (mem32 *)((byte *)_code_mem_bank_ptr_ + _cs_offset_); 
  _code_mem_bank_str_ = (byte *)(_code_mem_bank_ptr_ + 3);
}

//
mem32 serial_read32()
{
  mem32 tmp = 0;
  Serial.readBytes(buff, 4);
    
  tmp =                        buff[0];
  tmp = (tmp << 8) |buff[1];
  tmp = (tmp << 8) |buff[2];
  tmp = (tmp << 8) |buff[3];

  Serial.flush();
    
  return tmp;
}

//
void COSMOS_blink()
{
  //------ - --- -- ------ --- - - - - --
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);

  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(200);

  digitalWrite(LED_BUILTIN, HIGH);
  delay(200);

  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(200);

  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);

  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);

  for (mem32 i = 0; i < 50; i++)
    {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(50);
      digitalWrite(LED_BUILTIN, LOW);
      delay(50);
    }
}

//
void setup() 
{
  //
  pinMode(LED_BUILTIN, OUTPUT);
  
  //Set baudrate: 9600bps
  Serial.begin(9600);

  Serial.println(F("## COSMOS Board Ready (Due) ##"));
    
  Serial.flush();

  randomSeed(analogRead(0));
}

//
void loop() 
{            
  //Read bytes Serial buffer 
  while (Serial.available())
    {
      Serial.flush();
      mem32 _4bytes_ = serial_read32();
          
      if (_4bytes_ == EXEC_CMD)
	{    
	  //Serial.println(F("EXEC RECEIVED"));
                  
	  //
	  byte csec, ssec;
            
	  IP = 0;
                  
	  load_header();
	  load_cs_offset();

	  // Serial.print("Offset: ");
	  //Serial.println(_cs_offset_);
                   
	  if (!check_magic(_header_, &csec, &ssec))
	    {
	      Serial.println(F("Wrong magic number!"));
	      break;
	    }
                                    
	  if (ssec)
	    load_strings(); 

	  if (csec)
	    {
	      mem32 ret = run_code();
                          
	      if (ret != EXEC_HALT)
		Serial.print(F("EXECUTION ERROR: #")), Serial.println(ret);   
	    }

	  IP = 0;
	  nb_insn = 0;
	}
      else
	if (_4bytes_ == EXEC_BLNK)
	  COSMOS_blink();
   else
       if (_4bytes_ == EXEC_PING)
            Serial.println("---- ## COSMOS Board Ready ## ----");
	  else //Code loader
	      if (nb_insn < MAX_INSN)
	    _code_mem_bank_ptr_[nb_insn++] = _4bytes_;   
    }
}
