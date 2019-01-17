#movw r3, x00010003
movw r3, x80000000

mov r5, r3

xor r0, r0, r0
xor r4, r4, r4
movw r1, 1
xor r2, r2, r2
movb r15, '\n'

prntx r5
prntb r15

#Check if 0 while loop style

cmp r3, r4
jeq print

.lbl

shr r3, r3, r1

inc r2
cmp r3, r4
jne lbl

#
lmb r0, r5

.print

@ _1 "Log2( "
@ _2 " ) = "

prnts _1
prntx r5
prnts _2
prntw r2
prntb r15

prnts _1
prntx r5
prnts _2
prntw r0
prntb r15


hlt
