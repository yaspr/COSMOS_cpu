all: a c d h

a:
	gcc -std=c99 -O2 -g3 casm.c -o ../bin/casm

c:
	gcc -std=c99 -O2 -g3 ccpu_x86.c -o ../bin/ccpu_x86

d:
	gcc -std=c99 -O2 -g3 dasm.c -o ../bin/dasm

h:
	gcc -std=c99 -O2 -g3 chex.c -o ../bin/chex

clean:
	rm -Rf *~

cleanall:
	rm -Rf ../bin/*
