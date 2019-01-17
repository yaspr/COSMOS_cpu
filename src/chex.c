#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAGIC 0x00D3C0DE

//
unsigned check_magic(unsigned m, unsigned char *csec, unsigned char *ssec)
{
  unsigned magic_number = (m & 0x00FFFFFF);
  
  *csec = (m & 0x80000000) >> 31;
  *ssec = (m & 0x40000000) >> 30;
  
  return !(MAGIC ^ magic_number);
}

//
int main(int argc, char **argv)
{
  if (argc < 2)
    return printf("OUPS! %s [COSMOS binary]\n", argv[0]), -1;

  //
  FILE *fd = fopen(argv[1], "rb");
  unsigned char csec, ssec, _byte0_, _byte1_;
  unsigned addr = 0, _4bytes_ = 0, nb_strings = 0;
  
  if (fd)
    {
      //
      fread(&_4bytes_, sizeof(unsigned), 1, fd); //HEADER
      printf("[_HEADER_]\n\n\t0x%08x(%10u): 0x%08x\n\n", addr, addr, _4bytes_); addr += 4;
      
      check_magic(_4bytes_, &csec, &ssec);
      
      //
      fread(&_4bytes_, sizeof(unsigned), 1, fd); //CODE_OFFSET
      printf("[_CODE_OFFSET_]\n\n\t0x%08x(%10u): 0x%08x\n\n", addr, addr, _4bytes_); addr += 4;
      
      if (ssec)
	{	  
	  //
	  fread(&nb_strings, sizeof(unsigned), 1, fd); //NB STRINGS
	  printf("[_STRING_SECTION_]\n\n\t0x%08x(%10u): 0x%08x\n\n", addr, addr, nb_strings); addr += 4;
	  
	  //
	  for (unsigned j = 0; j < nb_strings; j++)
	    {
	      //
	      fread(&_byte0_, sizeof(unsigned char), 1, fd);
	      printf("\t0x%08x(%10u): %u\n", addr, addr, (unsigned)_byte0_); addr += 1;

	      //
	      printf("\t0x%08x(%10u): '", addr, addr);
	      
	      for (unsigned i = 0; i < _byte0_; i++)
		{
		  fread(&_byte1_, sizeof(unsigned char), 1, fd); 
		  printf("%c", _byte1_); addr += 1;
		}
	      printf("'");

	      printf("\n");
	    }

	  printf("\n");
	}
            
      //
      printf("[_CODE_SECTION_]\n\n");
      
      while (fread(&_4bytes_, sizeof(unsigned), 1, fd))
	{
	  printf("\t0x%08x(%10u): 0x%08x\n", addr, addr, _4bytes_);
	  addr += 4;
	}
      
      printf("\n");
    }
  else
    return printf("Cannot open file: %s\n", argv[0]), -2;
  
  return 0;
}
