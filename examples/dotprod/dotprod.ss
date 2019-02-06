movb r15, '\n'

#Array A starts at @0
movw r0, 0

#Array B starts at @1000
movw r1, 1000

#Init value base for B
movw r2, 100

#Init value base for A
movw r3, 0

#Number of elements
movw r4, 100

#### Init A ####
.lblinit_A

#Store r3 in A
storw 0(r0, r3), r3

#Loop control
inc r3
cmp r3, r4
jlt lblinit_A

movw r3, 0

#### Init B ####

.lblinit_B

#Next memory location and value
dec r2

#Store r2 in B
storw 0(r1, r2), r2

#Loop control
inc r3
cmp r3, r4
jlt lblinit_B

#### Dot product ####

#Array index
xor r7, r7, r7

#Reduction variable
xor r5, r5, r5 

.lbl_Dotprod

#Load from array A
loadw r2, 0(r0, r7)

#Load from array B
loadw r3, 0(r1, r7)

#r5 += r2 * r3
fma r5, r2, r3

inc r7
cmp r7, r4
jlt lbl_Dotprod

@ _ "The dotprod result is: "

prnts _
prntw r5
prntb r15

hlt
