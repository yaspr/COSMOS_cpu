#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <emmintrin.h>

#include "insn.h"

#define MAX_INSN 32 * 1024

#define MAX_STR 32

#define MAX_STR_LEN 256
#define MAX_NB_STR  1024

#define MAX_BLOCK 128 * 1024

#define MAX_LABELS 1024

#define MAX_R 32 //Number of 32 bit general purpose registers 
#define MAX_V 16 //Number of vector registers

//
typedef unsigned char byte;

//
typedef unsigned           mem32; //32bit type
typedef unsigned long long mem64; //64bit type

//
typedef struct label_s { char id[MAX_STR]; byte s_set; mem32 source; byte t_set; mem32 target; } label_t;

//
typedef struct string_entry_s { char id[MAX_STR]; byte len; char string[MAX_STR_LEN]; } string_entry_t; 
 
//
mem32 nb_lines;

//Code section offset
mem32 _cs_offset_;

//
mem32 nb_labels;
label_t _labels_table_[MAX_LABELS];

//
mem32 IP;
mem32 _code_mem_bank_[MAX_INSN];

//
mem32 nb_strings;
string_entry_t _strings_table_[MAX_NB_STR];

//
void error(char *str)
{
  printf("ERR(line: %u)#: %s\n", nb_lines - 1, str);
  exit(-1);
}

//
mem32 walk(char *str)
{
  mem32 i = 0;
  
  while (str[i] && (str[i] == ' ' || str[i] == '\t'))
    i++;

  return i;
}

//
mem32 walk_full_str(char *str)
{
  mem32 offset = 0;

  while (str[offset] && str[offset] != '\n')
    offset++;
  
  return offset;
}

//
byte is_digit(char c)
{ return (c >= '0' && c <= '9'); }

//
byte is_hex(char c)
{  return is_digit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'); }

//
byte is_low_alpha(char c)
{ return (c >= 'a' && c <= 'z'); }

//
byte is_up_alpha(char c)
{ return (c >= 'A' && c <= 'Z'); }

//
byte to_hex(char c)
{
  if (is_digit(c))
    return c - '0';
  else
    if (c >= 'A' && c <= 'F')
      return c - 55;
    else
      if (c >= 'a' && c <= 'f')
	return c - 87;
}

//
byte to_upper(char *str)
{
  char *p = str;
  
  while (*p)
    {
      (*p) -= 32;
      p++;
    }
  
  return 0;
}

//
mem32 get_low_str(char *str, char *out)
{
  mem32 i = 0, j = 0;
  
  while (str[i] && is_low_alpha(str[i]))
    out[j++] = str[i++];

  out[j] = '\0';
  
  return j;
}

//
mem32 get_up_str(char *str, char *out)
{
  mem32 i = 0, j = 0;
  
  while (str[i] && is_up_alpha(str[i]))
    out[j++] = str[i++];

  out[j] = '\0';
  
  return j;
}

//
mem32 get_quoted_str(char *str, char *out)
{
  mem32 i = 0, j = 0;

  if (str[0] == '\"')
    {
      i++; //Account for quote
      
      while (str[i] && str[i] != '\"' && str[i] != '\n')
	out[j++] = str[i++];
      
      if (str[i] != '\"')
	error("Missing ending quote! (\")");

      i++; //Account for quote
      
      out[j] = '\0';
    }
  else
    error("Missing begining quote! (\")");
  
  return i;
}

//
mem32 get_lbl_id(char *str, char *out)
{
  mem32 i = 0, j = 0;

  //Make sure first character is a-z or A-Z or _
  if (str[i] && (is_up_alpha(str[i]) || is_low_alpha(str[i]) || str[i] == '_'))
    {
      out[j++] = str[i++];
      
      while (str[i] && (is_up_alpha(str[i]) || is_low_alpha(str[i]) || is_digit(str[i]) || str[i] == '_'))
	out[j++] = str[i++];
  
      out[j] = '\0';
    }
  else
    error("Illegal character!\n");
  
  return j;
}

//
byte lookup_opcode(char *opcode)
{
  byte pos;
  byte found = 0;
  mem32 len1, len2;
    
  for (pos = 0; !found && pos < NB_OPCODES; pos++)
    {
      len1 = strlen(opcode);
      len2 = strlen(_opcodes_table_lower_[pos]);
      found = (len1 == len2) && (!strncmp(opcode, _opcodes_table_lower_[pos], len1));
    }
  
  return (found) ? pos - 1 : NB_OPCODES + 1;
}

//
mem32 lookup_label(char *label)
{
  mem32 pos;
  byte found = 0;
  mem32 len1, len2;
  
  for (pos = 0; !found && pos < nb_labels; pos++)
    {
      len1 = strlen(label);
      len2 = strlen(_labels_table_[pos].id);
      found = (len1 == len2) && (!strncmp(_labels_table_[pos].id, label, len1));
    }
  
  return (found) ? pos - 1 : MAX_LABELS + 1;
}

//
mem32 lookup_string(char *id)
{
  mem32 pos, len;
  byte found = 0;
  mem32 len1, len2;
  
  for (pos = 0; !found && pos < nb_strings; pos++)
    {
      len1 = strlen(id);
      len2 = strlen(_strings_table_[pos].id);
      found = (len1 == len2) && (!strncmp(_strings_table_[pos].id, id, len1));
    }
  
  return (found) ? pos - 1 : MAX_NB_STR + 1;
}

//
mem32 get_sep(char *str)
{
  if (str[0] == ',')
    return 1;
  else
    error("Missing separator!");
}

//
mem32 get_num(char *str, mem32 *n)
{
  mem32 offset = 0, num = 0;

  if (str[0] == 'd') //Decimal value
    {
      offset++;
      while (str[offset] && is_digit(str[offset]))
	{
	  num *= 10;
	  num += str[offset++] - '0';
	}
    }
  else
    if (str[0] == 'x') //Hex value
      {
	offset++;
	while (str[offset] && is_hex(str[offset]))
	  {
	    num <<= 4;
	    num += to_hex(str[offset++]);
	  }
      }
    else
      if (str[0] == 'b') //Binary value
	{
	  offset++;
	  while (str[offset] && (str[offset] == '0' || str[offset] == '1'))
	    {
	      num <<= 1;
	      num += str[offset++] - '0';
	    }
	}
      else
	if (str[0] == '\'') //Character value
	  {
	    offset++;

	    //ASCII character
	    if (str[offset] >= 0 && str[offset] < 256)
	      {
		if (str[offset] == '\\')
		  {
		    offset++;
		    
		    if (str[offset] == 'n')
		      num = '\n';
		    else if (str[offset] == 't')
		      num = '\t';
		    else if (str[offset] == '\\')
		      num = '\\';
		    else if (str[offset] == '\'')
		      num = '\'';
		    else if (str[offset] == '\"')
		      num = '\"';
		    else
		      error("Unknown value passed as character!");
		    
		    offset++;
		  }
		else 
		  num = str[offset++];
	      }
	    else
	      error("Unknown value passed as character!");
	    
	    if (str[offset] != '\'')
	      error("Missing terminating quote!");
	  }
	else //Decimal
	  while (str[offset] && is_digit(str[offset]))
	    {
	      num *= 10;
	      num += str[offset++] - '0';
	    }
  
  (*n) = num;
  
  return offset;
}

//
mem32 get_reg(char *str, mem32 *rid)
{
  mem32 offset = 0;
  mem32 reg_id = 0;

  //Register
  if (str[0] == 'r')
    {
      offset += get_num(str + 1, &reg_id) + 1; //+ 1 --> account for 'r'
      
      if (reg_id < MAX_R)
	(*rid) = reg_id;
      else
	error("Unknown register!");
    }
  else
    error("Register Error!");

  return offset;
}

//
mem32 get_vec(char *str, mem32 *rid)
{
  mem32 offset = 0;
  mem32 reg_id = 0;

  //Vector
  if (str[0] == 'v')
    {
      offset += get_num(str + 1, &reg_id) + 1; //+ 1 --> account for 'r'
      
      if (reg_id < MAX_V)
	(*rid) = reg_id;
      else
	error("Unknown register!");
    }
  else
    error("Register Error!");

  return offset;
}

//mbank(opd1, opd2)
mem32 get_mem(char *str, mem32 *opd1, mem32 *opd2, mem32 *mbank)
{
  mem32 pos;
  mem32 offset = 0;
  
  //
  if (str[offset] == '0' || str[offset] == '1')
    {
      *mbank = str[offset] - '0';
      offset++;

      if (str[offset] == '(')
	{
	  offset++;
	  offset += walk(str + offset);

	  offset += get_reg(str + offset, opd1);
	  offset += walk(str + offset);

	  if (str[offset] == ',')
	    {
	      offset++;
	      offset += walk(str + offset);
	      offset += get_reg(str + offset, opd2);
	      offset += walk(str + offset);

	      if (str[offset] != ')')
		error("Bandly formed memory address! ')'");

	      offset++;
	      
	      return offset;
	    }
	  else
	    error("Badly formed memory address! ','");
	  
	}
      else
	error("Bandly formed memory address! '('");
    }
  else
    error("Badly formed memory address! (memory bank)");
}

//
mem32 set_label_source(char *str)
{
  mem32 pos;
  mem32 offset = 0;
  char label[MAX_STR];
  
  offset += get_lbl_id(str, label);
  
  if ((pos = lookup_label(label)) == MAX_LABELS + 1)
    {
      strncpy(_labels_table_[nb_labels].id, label, strlen(label));
      
      _labels_table_[nb_labels].s_set    = 1; 
      _labels_table_[nb_labels++].source = IP;
    }
  else
    {
      if (!_labels_table_[nb_labels].s_set)
	{
	  _labels_table_[pos].s_set    = 1; 
	  _labels_table_[pos].source   = IP;
	}
      else
	error("Source label collision, labels must be unique!");
    }
  
  return offset;
}

//
mem32 set_label_target(char *str)
{
  mem32 pos;
  mem32 offset = 0;
  char label[MAX_STR];
  
  offset += get_lbl_id(str, label);
  
  if ((pos = lookup_label(label)) == MAX_LABELS + 1)
    {
      strncpy(_labels_table_[nb_labels].id, label, strlen(label));
      
      _labels_table_[nb_labels].t_set    = 1; 
      _labels_table_[nb_labels++].target = IP;
    }
  else
    {
      if (!_labels_table_[pos].t_set)
	{
	  _labels_table_[pos].t_set    = 1; 
	  _labels_table_[pos].target   = IP;	  
	}
      else
	error("Target label collision, labels must be unique!");
    }
  
  return offset;
}

//
mem32 insert_string(char *id, char *str)
{
  mem32 pos = lookup_string(id);

  if (pos < MAX_NB_STR)
    error("String id already used!");
  else
    {
      mem32 i_len = strlen(id);
      mem32 s_len = strlen(str);

      if (i_len >= MAX_STR_LEN && s_len >= MAX_STR_LEN)
	error("String too long!");

      _strings_table_[nb_strings].len = s_len;
      strncpy(_strings_table_[nb_strings].id, id, i_len);
      strncpy(_strings_table_[nb_strings].string, str, s_len);

      nb_strings++;

      _cs_offset_ += (1 + s_len); //1 byte (length value) + chars 
    }

  return 0;
}

//
mem32 dec_insn(char *str, mem32 opcode)
{
  mem32 offset = 0;
  char _id_[MAX_STR_LEN];
  mem32 insn, opd1, opd2, opd3, imm, format, id, mbank;
  
  switch (opcode)
    {
    case HLT:

      _code_mem_bank_[IP++] = HLT;
      
      nb_lines++;
      
      return 0;
      
      //Triple operand opcodes
    case ADD:
    case SUB:
    case MUL:
    case DIV:
    case MOD:
    case FMA:
    case FMS:
      
    case AND:
    case OR :
    case XOR:
    case SHL:
    case SHR:
    case ROL:
    case ROR:
      
      offset += get_reg(str + offset, &opd1);
      offset += walk(str + offset);          //Read first register id
      offset += get_sep(str + offset);
      offset += walk(str + offset);
      
      offset += get_reg(str + offset, &opd2);
      offset += walk(str + offset);          //Read first register id
      offset += get_sep(str + offset);
      offset += walk(str + offset);
      
      offset += get_reg(str + offset, &opd3);
      offset += walk(str + offset);
      offset++;                             //New line
      nb_lines++;
      
      opd1 = opd1 << 10; //0x00000X00
      opd2 = opd2 <<  5; //0x000000X0
      opd3 = opd3      ; //0x0000000X

      format = 0x00E00000; //Define operand format

      //Build instruction encoding
      insn = (opcode << 24) | format | opd1 | opd2 | opd3; 

      _code_mem_bank_[IP++] = insn;
      
      return offset;

      //Vector triple operands
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
      
      offset += get_vec(str + offset, &opd1);
      offset += walk(str + offset);          //Read first register id
      offset += get_sep(str + offset);
      offset += walk(str + offset);
      
      offset += get_vec(str + offset, &opd2);
      offset += walk(str + offset);          //Read first register id
      offset += get_sep(str + offset);
      offset += walk(str + offset);
      
      offset += get_vec(str + offset, &opd3);
      offset += walk(str + offset);
      offset++;                             //New line
      nb_lines++;
      
      opd1 = opd1 << 10; //0x00000X00
      opd2 = opd2 <<  5; //0x000000X0
      opd3 = opd3      ; //0x0000000X

      format = 0x00E00000; //Define operand format

      //Build instruction encoding
      insn = (opcode << 24) | format | opd1 | opd2 | opd3; 

      _code_mem_bank_[IP++] = insn;
      
      return offset;

    case NOTV:

      offset += get_vec(str + offset, &opd1);
      offset += walk(str + offset);          //Read first register id
      offset += get_sep(str + offset);
      offset += walk(str + offset);
      
      offset += get_vec(str + offset, &opd2);
      offset += walk(str + offset);
      offset++;                             //New line
      nb_lines++;
      
      opd1 = opd1 << 10; //0x00000X00
      opd2 = opd2 <<  5; //0x000000X0
      opd3 = opd3     ; //0x0000000X

      format = 0x00C00000; //Define operand format

      //Build instruction encoding
      insn = (opcode << 24) | format | opd1 | opd2 | opd3; 
      
      _code_mem_bank_[IP++] = insn;
      
      return offset;

      //Double operand opcodes

      //Register and immediate operands
    case MOVW:
    case MOVB:

      opd2 = opd3 = 0;
      
      offset += get_reg(str + offset, &opd1);
      offset += walk(str + offset);          //Read first register id
      offset += get_sep(str + offset);
      offset += walk(str + offset);
      
      offset += get_num(str + offset, &imm);
      offset += walk(str + offset);
      offset++;                              //New line
      nb_lines++;
      
      opd1 = opd1 << 10; //0x00000X00
      
      format = 0x00800000; //Define operand format

      //Build instruction encoding
      insn = (opcode << 24) | format | opd1 | opd2 | opd3; 

      _code_mem_bank_[IP] = insn;
      _code_mem_bank_[IP + 1] = imm;

      IP += 2;
      
      return offset;
      
      //Memory operands

      //Loads - opd1, mbank(opd2, opd3)
    case LOADW:
    case LOADB:
    case LOADC:
      
      //Destination register
      offset += get_reg(str + offset, &opd1);
      offset += walk(str + offset);
      offset += get_sep(str + offset);
      offset += walk(str + offset);
      
      //
      offset += get_mem(str + offset, &opd2, &opd3, &mbank);
      offset += walk(str + offset);
      offset++;
      nb_lines++;
            
      format = (mbank << 19 );

      opd1 = opd1 << 10;
      opd2 = opd2 <<  5;
      opd3 = opd3      ;      
      
      insn = (opcode << 24) | format | opd1 | opd2 |opd3;
      
      _code_mem_bank_[IP] = insn;

      IP++;
      
      return offset;
      
    case LOADV:

      //Destination vector
      offset += get_vec(str + offset, &opd1);
      offset += walk(str + offset);
      offset += get_sep(str + offset);
      offset += walk(str + offset);
      
      //
      offset += get_mem(str + offset, &opd2, &opd3, &mbank);
      offset += walk(str + offset);
      offset++;
      nb_lines++;
            
      format = (mbank << 19 );

      opd1 = opd1 << 10;
      opd2 = opd2 <<  5;
      opd3 = opd3      ;      
      
      insn = (opcode << 24) | format | opd1 | opd2 |opd3;
      
      _code_mem_bank_[IP] = insn;

      IP++;
      
      return offset;
      
      //Stores
    case STORW:
    case STORB:
    case STORC:
      
      //
      offset += get_mem(str + offset, &opd1, &opd2, &mbank);
      offset += walk(str + offset);
      offset += get_sep(str + offset);
      offset += walk(str + offset);
      
      //Destination register
      offset += get_reg(str + offset, &opd3);
      offset += walk(str + offset);
      offset++;
      nb_lines++;
      
      format = 0x00E00000 | (mbank << 19 );

      opd1 = opd1 << 10;
      opd2 = opd2 <<  5;
      opd3 = opd3      ;      
      
      insn = (opcode << 24) | format | opd1 | opd2 |opd3;
      
      _code_mem_bank_[IP] = insn;

      IP++;
      
      return offset;

    case STORV:

      //
      offset += get_mem(str + offset, &opd1, &opd2, &mbank);
      offset += walk(str + offset);
      offset += get_sep(str + offset);
      offset += walk(str + offset);
      
      //Destination vector
      offset += get_vec(str + offset, &opd3);
      offset += walk(str + offset);
      offset++;
      nb_lines++;
      
      format = 0x00E00000 | (mbank << 19 );

      opd1 = opd1 << 10;
      opd2 = opd2 <<  5;
      opd3 = opd3      ;      
      
      insn = (opcode << 24) | format | opd1 | opd2 |opd3;
      
      _code_mem_bank_[IP] = insn;

      IP++;
      
      return offset;

      //Register to register operands

    case NOT:
    case PCNT:
    case LMB:
    case INV:
    case REV:
    case CMP:

    case MOV:
      
      opd3 = 0;
      
      offset += get_reg(str + offset, &opd1);
      offset += walk(str + offset);          //Read first register id
      offset += get_sep(str + offset);
      offset += walk(str + offset);
      
      offset += get_reg(str + offset, &opd2);
      offset += walk(str + offset);
      offset++;                             //New line
      nb_lines++;
      
      opd1 = opd1 << 10; 
      opd2 = opd2 <<  5;

      format = 0x00C00000; //Define operand format

      //Build instruction encoding
      insn = (opcode << 24) | format | opd1 | opd2 | opd3; 

      _code_mem_bank_[IP++] = insn;
      
      return offset;
      
      //Single operand opcodes

      //Register operand
    case INC:
    case DEC:
    case TSC:
    case RAND:
    case PRNTX:
    case PRNTW:
    case PRNTB:
      
      opd2 = opd3 = 0;
      
      offset += get_reg(str + offset, &opd1);
      offset += walk(str + offset);
      offset++;                             //New line
      nb_lines++;
      
      opd1 = opd1 << 10; //0x00000X00
      
      format = 0x00800000; //Define operand format
      
      //Build instruction encoding
      insn = (opcode << 24) | format | opd1 | opd2 | opd3; 

      _code_mem_bank_[IP++] = insn;
      
      return offset;
      
      //
    case PRNTS:

      opd1 = opd2 = opd3 = 0;
      
      offset += get_lbl_id(str + offset, _id_);
      offset += walk(str + offset);
      offset++;
      nb_lines++;
      
      id = lookup_string(_id_);
      
      if (id == MAX_NB_STR + 1)
	error("Cannot find referenced string!");
      
      insn = (opcode << 24) | format | opd1 | opd2 | opd3;
      
      _code_mem_bank_[IP] = insn;
      _code_mem_bank_[IP + 1] = id;

      IP += 2;

      return offset;
      
      //Immediate operand
    case JMP:
    case JEQ:
    case JNE:
    case JGT:
    case JLT:
    case JGE:
    case JLE:
      
      opd1 = opd2 = opd3 = 0;
      
      offset += set_label_source(str + offset);
      offset += walk(str + offset);
      offset++;                                   //New line
      nb_lines++;

      format = 0;
      
      insn = (opcode << 24) | format | opd1 | opd2 | opd3;

      _code_mem_bank_[IP] = insn;
      //The target address will be added at label linking
      
      IP += 2;
      
      return offset;

      //Register and vector operands
    case REDV:

      opd3 = 0;
      
      offset += get_reg(str + offset, &opd1);
      offset += walk(str + offset);
      offset += get_sep(str + offset);
      offset += walk(str + offset);
      
      offset += get_vec(str + offset, &opd2);
      offset += walk(str + offset);
      offset++;                             //New line
      nb_lines++;
      
      opd1 = opd1 << 10; //0x00000X00
      opd2 = opd2 <<  5;
      
      format = 0x00C00000; //Define operand format
      
      //Build instruction encoding
      insn = (opcode << 24) | format | opd1 | opd2 | opd3; 

      _code_mem_bank_[IP++] = insn;
      
      return offset;

    default:
      error("Unknown opcode! (dec)\n");
      return offset;
    }
}

//Unmatched source isn't an issue
mem32 link_lbl()
{
  for (mem32 i = 0; i < nb_labels; i++)
    if (_labels_table_[i].s_set)
      if (_labels_table_[i].t_set) 
	_code_mem_bank_[_labels_table_[i].source + 1] = _labels_table_[i].target;
      else
	{
	  printf("Label: %u ", i); 
	  error("Unmatched target label!");
	}
  
  return 0;
}

//
mem32 dump_strings_table(FILE *fd)
{  
  //Number of strings
  fwrite(&nb_strings, sizeof(mem32), 1, fd);
  
  for (mem32 i = 0; i < nb_strings; i++)
    {      
      //Length
      fwrite(&_strings_table_[i].len, sizeof(byte), 1, fd);
      //String
      fwrite(_strings_table_[i].string, sizeof(byte), _strings_table_[i].len, fd); 
    }
  
  return 0;
}

//
mem32 parse_asm(char *str)
{
  byte opcode;
  mem32 offset = 0;
  char _opcode_[MAX_STR];
  char _id_[MAX_STR], _string_[MAX_STR];
  
  do
    {
      offset += walk(str + offset);
      
      //Check if label
      if (*(str + offset) == '.') //Label
	{
	  offset++;
	  offset += set_label_target(str + offset);
	  offset += walk(str + offset);
	  offset++;                                   //New line
	  nb_lines++;

	  opcode = 1;
	}
      else
	{
	  if (*(str + offset) == '\n') //Line
	    {	      
	      offset++;
	      nb_lines++;

	      opcode = 1;
	    }
	  else
	    if (*(str + offset) == '#') //Comment
	      {
		offset++;
		offset += walk_full_str(str + offset);
		nb_lines++;

		opcode = 1;
	      }
	    else
	      if (*(str + offset) == '@') //String declaration
		{
		  offset++;
		  offset += walk(str + offset);
		  offset += get_lbl_id(str + offset, _id_);
		  offset += walk(str + offset);
		  offset += get_quoted_str(str + offset, _string_);
		  offset += walk(str + offset);
		  
		  insert_string(_id_, _string_);
		  
		  offset++;
		  nb_lines++;

		  opcode = 1;
		}
	      else
		{
		  //Opcode
		  offset += get_low_str(str + offset, _opcode_);
		  offset += walk(str + offset);
		  
		    opcode = lookup_opcode(_opcode_);
		    
		    if (opcode == NB_OPCODES + 1)
		      error("Unknown opcode! (parse)");
		    else
		      offset += dec_insn(str + offset, opcode);
		}
	}
    }
  while (*(str + offset) && opcode != HLT);

  return 0;
}

//
int main(int argc, char **argv)
{
  if (argc < 3)
    return printf("OUPS: %s [input assembmy file] [output binary]\n", argv[0]), -1;

  IP          = 0;
  nb_lines    = 0;
  nb_labels   = 0;
  nb_strings  = 0;
  _cs_offset_ = 8; //Account for header & offset itself
  
  byte str[MAX_BLOCK];
  mem32 _header_ = 0x80D3C0DE;
  FILE *ifd = fopen(argv[1], "rb");
  FILE *ofd = fopen(argv[2], "wb");

  mem32 size_bytes = fread(str, sizeof(byte), MAX_BLOCK, ifd);
  
  parse_asm(str);
  link_lbl();

  //First bit sets code section, second bit sets string section
  if (nb_strings)
    {
      _header_ |= 0x40000000; //110000001...1
      _cs_offset_ += 4; //Account for nb_strings
    }
  
  fwrite(&_header_, sizeof(mem32), 1, ofd);
  fwrite(&_cs_offset_, sizeof(mem32), 1, ofd);
  
  if (nb_strings) dump_strings_table(ofd);
  
  fwrite(_code_mem_bank_, sizeof(mem32), IP, ofd);
  
  fclose(ifd);
  fclose(ofd);
  
  return 0;
}
