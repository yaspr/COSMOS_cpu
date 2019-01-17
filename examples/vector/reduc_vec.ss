# Temporary 4 elmeents array at @ = 0
xor r11, r11, r11 

# Base memory at @ = 4, bank0 - Reduction array
movw r0, 4

# Number of elements in the array
movw r2, 100

xor r3, r3, r3
xor r1, r1, r1
movw r4, 1

movb r14, ' '
movb r15, '\n'

#
.lbl_init

# Init array A 
storw 0(r0, r1), r4

inc r4
inc r1
cmp r1, r2
jlt lbl_init

xor r1, r1, r1
movw r4, 4

#Init vector v2 to 0
#loadv v2, 0(r11, r3)
xorv v2, v2, v2

tsc r13

# Vector reduction
.lbl_reduc

loadv v0, 0(r0, r1)

addv v2, v2, v0

add r1, r1, r4
cmp r1, r2
jlt lbl_reduc

tsc r12

sub r12, r12, r13

#15
@ __ "Vector cycles: "

prnts __
prntw r12
prntb r15

xor r1, r1, r1

storv 0(r11, r1), v2 

loadw r4, 0(r11, r1)
inc r1
loadw r3, 0(r11, r1)
inc r1
add r4, r4, r3
loadw r3, 0(r11, r1)
inc r1
add r4, r4, r3
loadw r3, 0(r11, r1)
add r4, r4, r3

#18
@ _ "Vector reduction: "

prnts _
prntw r4
prntb r15

prntb r15

xor r1, r1, r1
xor r3, r3, r3

tsc r13

# Sequential reduction
.lbl_seq_reduc

loadw r4, 0(r0, r1)

add r3, r3, r4

inc r1
cmp r1, r2
jlt lbl_seq_reduc

tsc r14

sub r14, r14, r13

#19
@ _1 "Sequential cycles: "

prnts _1
prntw r14
prntb r15

#22
@ _2 "Sequential reduction: "

prnts _2
prntw r3
prntb r15

div r14, r14, r12

#16
@ _3 "Vector speedup: "

prnts _3
prntw r14
prntb r15

hlt
