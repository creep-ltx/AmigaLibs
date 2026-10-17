| hot.s - fuzzy.library's two hottest loops, by hand, in plain 68000
| instructions (they run on every processor). C stands in for them on the PC
| (match.c, index.c): the tests compare the two.
|
| Called from C (bebbo's gcc): arguments on the stack, d0-d1/a0-a1 free,
| the rest kept.

	.text

| ULONG fz_copymask(const UBYTE *src, UBYTE *dst, ULONG *mk, const ULONG *mtab,
|                   LONG n, ULONG m)
| Copies n bytes from src to dst and, for each, ors its fingerprint bit
| (mtab[byte]) into m and stores m in mk[] - a path's new bytes as an index
| entry is read. Returns the last m. n is at most 1024 (MAXPATH).
	.globl	_fz_copymask
_fz_copymask:
	movem.l	d2/a2-a3,-(sp)		| 12 bytes pushed
	move.l	16(sp),a0		| src
	move.l	20(sp),a1		| dst
	move.l	24(sp),a2		| mk
	move.l	28(sp),a3		| mtab
	move.l	32(sp),d1		| n
	move.l	36(sp),d0		| m
	subq.w	#1,d1
	bmi.s	2f
1:	moveq	#0,d2
	move.b	(a0)+,d2
	move.b	d2,(a1)+
	lsl.w	#2,d2			| a ULONG each
	or.l	0(a3,d2.w),d0
	move.l	d0,(a2)+
	dbf	d1,1b
2:	movem.l	(sp)+,d2/a2-a3
	rts

| const UBYTE *fz_scan(const UBYTE *q, const UBYTE *qe, const UBYTE *fold,
|                      ULONG want)
| The first place from q (before qe) whose byte, through fold[], is want;
| qe if there is none. qe - q is at most 65535 (FuzzyScore cuts texts there).
	.globl	_fz_scan
_fz_scan:
	move.l	d2,-(sp)		| 4 bytes pushed
	move.l	8(sp),a0		| q
	move.l	12(sp),d2		| qe
	move.l	16(sp),a1		| fold
	move.l	20(sp),d0		| want (the low byte)
	sub.l	a0,d2			| how many
	ble.s	3f
	subq.l	#1,d2
	moveq	#0,d1			| only its low byte changes below
1:	move.b	(a0)+,d1
	cmp.b	0(a1,d1.w),d0
	dbeq	d2,1b
	bne.s	2f			| ran out
	subq.l	#1,a0			| on the byte that matched
	move.l	a0,d0
	move.l	(sp)+,d2
	rts
2:	move.l	a0,d0			| = qe
	move.l	(sp)+,d2
	rts
3:	move.l	12(sp),d0		| nothing to look at: qe
	move.l	(sp)+,d2
	rts
