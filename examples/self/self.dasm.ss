#[_CODE_SECTION_]
._0x00000000
	movw	r0, 0
._0x00000002
	movw	r1, 1
._0x00000004
	movw	r2, 100
._0x00000006
	movb	r3, '\n'
._0x00000008
	movw	r4, 1
._0x0000000a
	movw	r14, 19
._0x0000000c
	xor	r11, r11, r11
._0x0000000d
	movw	r13, 333448228
._0x0000000f
	movw	r12, 31458336
._0x00000011
	prntw	r1
._0x00000012
	prntb	r3
._0x00000013
	add	r1, r1, r0
._0x00000014
	and	r5, r1, r4
._0x00000015
	cmp	r5, r4
._0x00000016
	jeq	_0x0000001a
._0x00000018
	jmp	_0x0000001d
._0x0000001a
	storc	0(r14, r11), r12
._0x0000001b
	jmp	_0x0000001e
._0x0000001d
	storc	0(r14, r11), r13
._0x0000001e
	inc	r0
._0x0000001f
	cmp	r0, r2
._0x00000020
	jle	_0x00000011
._0x00000022
	hlt