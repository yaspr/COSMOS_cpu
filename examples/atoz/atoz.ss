movw r0, 65
movw r2, 26
movb r3, '\n'

add r1, r0, r2

.lbl

prntb r0
prntb r3

inc r0
cmp r0, r1
jlt lbl

hlt
