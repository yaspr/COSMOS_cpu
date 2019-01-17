#define LEN_OPCODE  16

#define NB_OPCODES  69

#define MAX_OPCODES 256

//W --> 4 bytes     -->  32 bits (W = word)
//B --> 1 byte      -->   8 bits (B = byte)
//V --> 4 x 4 bytes --> 128 bits (V = vector) 

//Opcodes
enum opcode_e {
	       HLT   ,
	       ADD   , SUB   , MUL   , DIV  , MOD   , INC  , DEC   , FMA   ,	      
	       ADDV  , SUBV  , MULV  , DIVV , MODV  , FMAV , AND   , OR    ,
	       XOR   , NOT   , SHL   , SHR  , ROL   , ROR  , PCNT  , LMB   ,
	       INV   , REV   , ANDV  , ORV  , XORV  , NOTV , SHLV  , SHRV  ,
	       ROLV  , RORV  , CMP   , JMP  , JEQ   , JNE  , JGT   , JLT   ,
	       JGE   , JLE   , LOADW , STORW, LOADB , STORB, LOADV , STORV ,
	       MOVW  , MOVB  , MOV   , PRNTW, INPUTW, PRNTB, INPUTB, PRNTS ,
	       INPUTS, PRNTX , RAND  , REDV , RDBLK , WRBLK, TSC   , DELAY ,
	       FMS   , FMSV  , LOADC , STORC
};

//
char _opcodes_table_upper_[MAX_OPCODES][LEN_OPCODE] = {
						       "HLT"   , 
						       "ADD"   , "SUB"   , "MUL"   , "DIV"  , "MOD"   , "INC"  , "DEC"   , "FMA"  ,	      
						       "ADDV"  , "SUBV"  , "MULV"  , "DIVV" , "MODV"  , "FMAV" , "AND"   , "OR"   ,
						       "XOR"   , "NOT"   , "SHL"   , "SHR"  , "ROL"   , "ROR"  , "PCNT"  , "LMB"  ,
						       "INV"   , "REV"   , "ANDV"  , "ORV"  , "XORV"  , "NOTV" , "SHLV"  , "SHRV" ,
						       "ROLV"  , "RORV"  , "CMP"   , "JMP"  , "JEQ"   , "JNE"  , "JGT"   , "JLT"  ,
						       "JGE"   , "JLE"   , "LOADW" , "STORW", "LOADB" , "STORB", "LOADV" , "STORV",
						       "MOVW"  , "MOVB"  , "MOV"   , "PRNTW", "INPUTW", "PRNTB", "INPUTB", "PRNTS",
						       "INPUTS", "PRNTX" , "RAND"  , "REDV" , "RDBLK" , "WRBLK", "TSC"   , "DELAY",
						       "FMS"   , "FMSV"  , "LOADC" , "STORC"
};

//
char _opcodes_table_lower_[MAX_OPCODES][LEN_OPCODE] = {
						       "hlt"   , 
						       "add"   , "sub"   , "mul"   , "div"  , "mod"   , "inc"  , "dec"   , "fma"  ,	      
						       "addv"  , "subv"  , "mulv"  , "divv" , "modv"  , "fmav" , "and"   , "or"   ,
						       "xor"   , "not"   , "shl"   , "shr"  , "rol"   , "ror"  , "pcnt"  , "lmb"  ,
						       "inv"   , "rev"   , "andv"  , "orv"  , "xorv"  , "notv" , "shlv"  , "shrv" ,
						       "rolv"  , "rorv"  , "cmp"   , "jmp"  , "jeq"   , "jne"  , "jgt"   , "jlt"  ,
						       "jge"   , "jle"   , "loadw" , "storw", "loadb" , "storb", "loadv" , "storv",
						       "movw"  , "movb"  , "mov"   , "prntw", "inputw", "prntb", "inputb", "prnts",
						       "inputs", "prntx" , "rand"  , "redv" , "rdblk" , "wrblk", "tsc"   , "delay",
						       "fms"   , "fmsv"  , "loadc" , "storc"
};
