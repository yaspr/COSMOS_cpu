# Example reduction code  
movw r0, 0
xor r3, r3, r3
movw r4, 100
movw r2, 1
movb r6, '\n'

# Initializing memory with 
.lbl_init

storw 0(r0, r3), r2
inc r2

inc r3
cmp r3, r4
jlt lbl_init

xor r3, r3, r3
xor r1, r1, r1

# Performing reduction
.lbl_reduc

loadw r2, 0(r0, r3)

add r1, r1, r2

inc r3
cmp r3, r4
jlt lbl_reduc

#Defining a string
@ _ "The result of the reduction is: "

prnts _
prntw r1
prntb r6

hlt
