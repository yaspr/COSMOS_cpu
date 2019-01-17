# r += B * C

# Number of elements
movw r0, 100

#Init array A @ = 0, bank = 0
xor r2, r2, r2
xor r1, r1, r1

.init_A

storw 0(r2, r1), r1

inc r1
cmp r1, r0
jlt init_A

xor r1, r1, r1

#Init array B @ = 0, bank = 1

.init_B

storw 1(r2, r1), r1

inc r1
cmp r1, r0
jlt init_B

#
xor r1, r1, r1
xorv v0, v0, v0

# 100 / 4 = 25, 4 is vector size
movw r4, 4

.dotprod

loadv v1, 0(r2, r1)
loadv v2, 1(r2, r1)

fmav v0, v1, v2

add r1, r1, r4

cmp r1, r0
jlt dotprod

xor r1, r1, r1

#Temporary 4 elements array
movw r4, 100

storv 0(r4, r1), v0

loadw r3, 0(r4, r1)
inc r1
loadw r5, 0(r4, r1)
inc r1
add r3, r3, r5
loadw r5, 0(r4, r1)
inc r1
add r3, r3, r5
loadw r5, 0(r4, r1)
add r3, r3, r5

movb r15, '\n'

@ _ "Dotprod: "

prnts _
prntw r3
prntb r15

hlt
