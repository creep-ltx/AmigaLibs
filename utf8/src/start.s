| utf8.library's first code: started as a program, it ends at once.
	.text
	.globl	_start
_start:
	moveq	#-1,d0
	rts

| ARexx's entry, the library's first call (offset -30): a0 = the RexxMsg,
| a6 = the library base. ARexx takes an error code in d0 and the result
| string in a register, which C cannot return, so rexx_query(base, msg,
| &result) does the work and this puts the result in a0 AND a1: OS 3.2's
| ARexx was seen reading a0 (a result in a1 alone came out as garbage
| and later a FreeMem guru, 2026-10-16); older notes say a1.
	.globl	_Utf8RexxEntry
_Utf8RexxEntry:
	subq.l	#4,sp			| the result
	move.l	sp,-(sp)		| &result
	move.l	a0,-(sp)		| msg
	move.l	a6,-(sp)		| base
	jsr	_rexx_query
	lea	12(sp),sp
	move.l	(sp)+,a0
	move.l	a0,a1
	rts
