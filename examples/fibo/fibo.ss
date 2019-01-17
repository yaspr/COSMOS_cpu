movw r0, 1
movw r1, 1
movw r3, 0
movw r4, 50

.lbl

add r2, r0, r1

prntw r3

movb r6, ' '
prntb r6

prntw r2

movb r5, '\n'
prntb r5

mov r0, r1
mov r1, r2

inc r3

cmp r3, r4
jlt lbl

hlt
