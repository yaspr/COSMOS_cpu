
xor r2, r2, r2
movw r0, 10

movb r14, ' '
movb r15, '\n'

.lbl

rand r1

prntw r1
prntb r14

inc r2
cmp r2, r0
jlt lbl

prntb r15

hlt
