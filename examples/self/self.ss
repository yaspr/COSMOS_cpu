#Example of a selfmodifiying code

movw r0, 0
movw r1, 1
movw r2, 100
movb r3, '\n'
movw r4, 1

#19 is the address
#of the target instruction
movw r14, 19

xor r11, r11, r11

#shift r1, r1, r4
movw  r13, x13e00424

#add r1, r1, r0
movw  r12, x01e00420

#patch the code at runtime
.loop_entry

prntw r1
prntb r3

add r1, r1, r0

and r5, r1, r4
cmp r5, r4
jeq odd
jmp even

.odd
storc 0(r14, r11), r12
jmp loop_end

.even
storc 0(r14, r11), r13

.loop_end
inc r0
cmp r0, r2
jle loop_entry

hlt