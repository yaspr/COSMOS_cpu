COSMOS Microarchitecture 
- Yaspr 2019 - 
Cosmic Freedom License
	This document and everything it describes is open and free of rights
and charges everywhere in the cosmos at all times.

TODO:
	### Add proper error management ###
	Debugger (90%)
	Smart assembler (dead code elimination, optimizations, ?) (50%)
	High level language & compiler (20%)
	

					      Architectural details
------------------------------------------------------------------------------------------------


I ### Basic circuit components for building a board
------------------------------------------------------------------------------------------------


                CPU                    CODE SRAM
              _______                    _____
             |.  v   |                  |. v  |
             |       |                  |     |
             |       |                  |     |
             |       |                   -----
             |       |                 
             |       |                 23LC1024 (1Mbit = 128KB) SPI interface
             |       |                 
             |       |                 DATA SRAM x 2 = 256KB 
             |       |                   _____    _____
             |       |                  |. v  |  |. v  |
             |       |                  |     |  |     |
             |       |                  |     |  |     |
              -------                    -----    -----
             ATmega328p                  bank0    bank1
	      (2KB SRAM)
	     (1KB EEPROM)

	    Adding more banks ==> more bits to encode the bank ID 
	    
	    23LC1024 SRAM address is 17 bits long --> 2^17 = 128K --> maps to 128K bytes
	    The SRAM works in 2 modes: 
	                byte mode  : reading/writing byte by byte
		        burst mode : reading/writing an array at once

			
			32 bits address: [0:14b][Memory bank:1b][Address:17b]
			
		


	        _____________
	       |             \
	      _|______________|_       SPI interface SD card slot
	     |                  |      Persistent storage accessible through
	     |                  |      512B blocks. Block 0 is to be avoided.
	     |                  |      
	     |    2GB SD card   |      Write block
	     |                  |      Read  block
	     |                  |
	     |                  |
	      ------------------


	     4KB of CODE SRAM is reserved for file manipulation
	     Each file is given an initial 1KB block and others will be allocated 



II ### Instruction set: CPU side
------------------------------------------------------------------------------------------------

//Registers:

  //General purpose:
    
  R --> 32 scalar registers = 16 * 32b  = 128B
  V --> 32 vector registers = 16 * 128b = 512B

  //Special:
  IP --> instruction pointer (32bit)

  //IF GE LE GT LT ZF CF ET      
  ET --> vector element type: 1 - integer (32bit) or 0 - character (8bit)
  CF --> carry flag 
  EF --> equal flag (set to 1 after a CMP of equal values)
  LT --> strictly lower than 
  GT --> strictly greater than
  LE --> lower or equal
  GE --> greater or equal
  IF --> interrupt flag
  
//Instruction format: 

              32bit long instruction: 
	          [opcode(1B)][format(9b)][opd(5b)][opd(5b)][opd(5b)]


			       XXXXX (opd3)
			  XXXXX (opd2)
		     XXXXX (opd1)
		     
		  format:
		     bit0: set if operand 1 used 
		     bit1: set if operand 2 used
		     bit2: set if operand 3 used
		     bit3: set if operand1 is IMM
		     
		     bit4: memory bank [0 or 1] 
		     bit5: 1:vector or 0:scalar
	       	     bit6: unused
		     bit7: unused

		     bit8: unused

//Scalar Arithmetic operations:

	       ADD R, R, R
	       SUB R, R, R
	       MUL R, R, R 
	       DIV R, R, R 
	       MOD R, R, R
	       INC R 
	       DEC R                    
	       FMA R, R, R   -- Fused Multiply Add: R += R * R
	       FMS R, R, R   -- Fused Multiply Subtract: R -= R * R

//Vector Arithmetic operations:

	       ADDV V, V, V
	       SUBV V, V, V
	       MULV V, V, V 
	       DIVV V, V, V 
	       MODV V, V, V
	       FMAV V, V, V
	       FMSV V, V, V
	     
//Logic Scalar operations:

	       AND  R, R, R
	       OR   R, R, R
	       XOR  R, R, R
	       NOT  R, R
	       SHL  R, R, R
	       SHR  R, R, R
	       ROL  R, R, R 
	       ROR  R, R, R               
	       PCNT R, R
	       LMB  R, R
	       RMB  R, R
	       REV  R, R
	       
//Logic Vector operations:

             ANDV V, V, V
	       ORV  V, V, V
	       XORV V, V, V
	       NOTV V, V, V
	       SHLV V, V, V
	       SHRV V, V, V
	       ROLV V, V, V
	       RORV V, V, V
  
//Control operations:

	       CMP R, R //left R (> | >= | < | <= | ==) right R
	       JMP IMM //Label in assembly 
	       JEQ IMM //Label in assembly  
	       JNE IMM //Label in assembly  
	       JGT IMM //Label in assembly  
	       JLT IMM //Label in assembly  
	       JGE IMM //Label in assembly 
	       JLE IMM //Label in assembly      
	       HLT

//Memory operations:

	       //0(R?, R?) --> memory bank 0, base address, offset 
	       //1(R?, R?) --> memory bank 1, base address, offset

	       LOADW R, B(R, R)     //Load a 32b integer
	       STORW B(R, R), R     //Store a 32b integre
	       LOADB R, B(R, R)     //Load character
	       STORB B(R, R), R     //Store character

	       LOADV V, B(R, R)
	       STORV B(R, R), V

	       MOVW R, IMM     //Move a 32b integer  
	       MOVB R, IMM     //Move a character
	       MOV  R, R       //Move a register into another
 
//Input/Output operations:

             PRNTW R         //Print integer
	       INPTW R         //Read integer 
	       PRNTB R         //Print character
	       INPTB R         //Read character
	       PRNTS IMM       //Print string IMM is the string table ID
	       INPTS IMM       //Read string

	       -- Only on boards with SD card slot
	       
	       RDB R, MEM   //Read  512B block from HDD
	       WRB R, MEM   //Write 512B block on HDD
	    
//Timing operations:
       
             TSC   R  //Time Stamp Counter; read milliseconds or cycles (arch dependent)
	       DELAY R  //Delays in milliseconds







III ### Binary format & tools
------------------------------------------------------------------------------------------------

An emacs major mode is provided for keywords highlighting and coloration.
Open the file in emacs and run the following: M-x eval-buffer.
Open the .csms assembly file and run the following: M-x cosmos-mode.
Colors should magically appear.

Binary format:
--------------
		[32bit HEADER]
		       bit0: if set, code section present
		       bit1: if set, strings section present

		[Offset of code section]

 		[Strings section]

		       byte0: length
		       next bytes: byte0 bytes/chars

		       byte1: length
		       next bytes: byte0 bytes/chars
		       
		[Code section]

		       32bit instructions and 32 bit immediate operands

Assembly language:
------------------

	 /!\ Opcodes and registers must be lower case

	 Assembly format:
	 
	 OPCODE DST_REGISTER, SRC_REGISTER, SRC_REGISTER 
	 OPCODE DST_REGISTER, SRC_REGISTER
	 OPCODE DST_REGISTER
	 OPCODE DST_REGISTER, IMMEDIATE
	 OPCODE LABEL_ID
	 OPCODE STRING_ID
	 OPCODE DST_REGISTER, MEM_BANK_ID(ADDRESS_REGISTER, INDEX_REGISTER)
	 OPCODE MEM_BANK_ID(ADDRESS_REGISTER, INDEX_REGISTER), DST_REGISTER
	 OPCODE

	 String declaration:

	 @ STRING_ID "STRING"

	 .LABEL_ID



	 --- Assembly instructions:
	 
	       add r, r, r
	       sub r, r, r
	       mul r, r, r 
	       div r, r, r 
	       mod r, r, r
	       inc r 
	       dec r                    
	       fma r, r, r

	       vadd v, v, v
	       vsub v, v, v
	       vmul v, v, v 
	       vdiv v, v, v 
	       vmod v, v, v
	       vfma v, v, v
	     
	       and  r, r, r
	       or   r, r, r
	       xor  r, r, r
	       not  r, r
	       shl  r, r, r
	       shr  r, r, r
	       rol  r, r, r 
	       ror  r, r, r               
	       pcnt r, r
	       lmb  r, r
	       rmb  r, r
	       rev  r, r
	       
             vand v, v, v
	       vor  v, v, v
	       vxor v, v, v
	       vnot v, v, v
	       vshl v, v, v
	       vshr v, v, v
	       vrol v, v, v
	       vror v, v, v
  
	       cmp r0, r1 //r0 (> | >= | < | <= | ==) r1
	       
	       jmp label_id  
	       jeq label_id   
	       jne label_id   
	       jgt label_id   
	       jlt label_id   
	       jge label_id  
	       jle label_id       

	       hlt     //Stop execution


	       //0(R?, R?) --> memory bank 0, base address, offset 
	       //1(R?, R?) --> memory bank 1, base address, offset

	       loadw r, b(r, r)      	   //Load a 32b integer
	       storw b(r, r), r     	   //Store a 32b integre

	       loadb r, b(r, r)     	   //Load character
	       storb b(r, r), r     	   //Store character

	       loadv v, b(r, r)
	       storv b(r, r), v
	       
	       movw r, imm		   //Move a 32b integer  
	       movb r, imm     		   //Move a character
	       mov  r, r       		   //Move a register into another
		 movv v, v
 
             prntw r			   //Print an integer on stdout
	       inptw r         		   //Read an integer from stout

	       prntb r         		   //Print an character on stdout
	       inptb r         		   //Read an character from stdout

	       prnts string_id       	   //Print a string defined by string_id 

             tsc   r  		   	   //Time Stamp counter (arch dependent)
	     
Disassembler:
-------------

The disassembler (dasm) produces reassemblable code.

Hex editor:
-----------

A hex editor (chex) is provided for binary explorartion.

Debugger:
---------		
		        CPU              
   	             _________
	            |_________|        
	            |_________|        
	            |_________|       
	            |_________|               DBG co-processor
debug on IP -->     |___DBG___|                   ______
	            |_________| <-- DBG INSN --> |______|
	            |_________| <-- DBG INSN --> |______|     
	            |_________| <-- DBG INSN --> |______|
debug off IP -->    |___DBG___|
	            |_________|        
	            |_________|        
	            |_________|        
	            |_________|        
	            |_________|        
	               Code
	
		CPU debug instructions:
		
		   DBG --> toggles dbg flag
		       DBG_MODE_ACT
		       DBG_MODE_NACT

		       
		DBG co-processor instructions:

		   Reading registers
		   -----------------
		   
		   RREG   RED_ID	-- Read REG_ID content
		   RREGA		-- Read all registers content
		   RREGV  REG_ID	-- Read vector RED_ID content
		   RREGVA		-- Read all vector registers content
		   RIP			-- Read IP content

		   Writing registers
		   -----------------

		   WREG   RED_ID, VAL				-- Set REG_ID 
		   WREGV  REG_ID, (VAL, VAL, VAL, VAL)		-- Set vector RED_ID
		   WIP 		   				-- Set IP 

		   
Bugs may arise please be kind and report them to [ binary(dot)yaspr(at)gmail(dot)com ] with the following subject: [COSMOS BUG]).

:]
