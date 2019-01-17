
xor r0, r0, r0
movw r4, 4
movw r1, 100

movb r31, '\n'

# Array A @ 0
xor r2, r2, r2

movw r3, 1

#
.init_A

#bank 0 array A
storw 0(r2, r0), r0

#bank 1 array B
storw 1(r2, r0), r3

inc r3

inc r0
cmp r0, r1
jlt init_A

xor r0, r0, r0

#
.reduc_AB

loadv v0, 0(r2, r0)
loadv v1, 1(r2, r0)

add r0, r0, r4

redv r5, v0
redv r6, v1

cmp r0, r1
jlt reduc_AB

@ _A "ReductionA: "

prnts _A
prntw r5
prntb r15

@ _B "ReductionB: "

prnts _B
prntw r6
prntb r15

hlt
