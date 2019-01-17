# Example code for computing
# the pocount of r5

# Goofing around with a binary value

movw r5, b111001100101
mov r7, r5

#Setting up return character
movb r6, '\n'

@ _    "Initial value: "  

prnts _
prntx r5
prntb r6

movw r2, 3

rol r5, r5, r2

@ __ "Left rotate of 3: "

prnts __
prntw r5
prntb r6

movw r4, 0
movw r0, 0
movw r1, 1

.lbl

sub r3, r5, r1
and r5, r5, r3

inc r0

cmp r5, r4
jne lbl

@ pop1 "Popcount1: "

prnts pop1 
prntw r0
prntb r6

pcnt r10, r7

@ pop2 "Popcount2: "

prnts pop2 
prntw r10
prntb r6

hlt
