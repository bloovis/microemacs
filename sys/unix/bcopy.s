/*----------------------------------------------------------------------
 *	bcopy - byte string operations for 4.2 BSD
 */
	.text

/* BCOPY - move BYTEs from source to destination

 * bcopy(source,dest,count)
 * BYTE *source, *dest;
 * WORD count;
 * {
 *     while (count-- != 0) *dest++ = *source++;
 * }
 */

	.globl	_bcopy
_bcopy:
	.word	0x0040		# use register 6
	movl	4(ap),r1	# source
	movl	8(ap),r3	# dest
	movl	12(ap),r6	# count
	jbr	2f
1:
	subl2	r0,r6
	movc3	r0,(r1),(r3)
2:
	movzwl	$65535,r0
	cmpl	r6,r0
	jgtr	1b
	movc3	r6,(r1),(r3)
	ret

/*----------------------------------------------------------------------
 * BCOPYR - move BYTEs from source to destination, start at right end

 * bcopyr(source,dest,count)
 * BYTE *source, *dest;
 * WORD count;
 * {
 *     source += count;
 *     dest += count;
 *     while (count-- != 0) *--dest = *--source;
 * }
 */

	
	.globl	_bcopyr
_bcopyr:
	.word	0x0040		# use register 6
	movl	4(ap),r1	# source
	movl	8(ap),r3	# dest
	movl	12(ap),r6	# count
3:
	addl2	r6,r1		# point at end of source
	addl2	r6,r3		# point at end of dest
	movzwl	$65535,r0	# maximum count is 64K-1 in one "movc3"
	jbr	5f		# go check if count >= 64K
4:				# come here if count >= 64K
	subl2	r0,r6		# count -= 65535
	subl2	r0,r1		# source -= 65535
	subl2	r0,r3		# dest -= 65535
	movc3	r0,(r1),(r3)	# copy 65535 bytes
	movzwl	$65535,r0	# restore r0 from effects of movc3
	subl2	r0,r1		# restore r1 from effects of movc3
	subl2	r0,r3		# restore r3 from effects of movc3
5:
	cmpl	r6,r0		# is count > 65535?
	jgtr	4b		# yes - go copy 65535 bytes
	subl2	r6,r1		# point at start of source
	subl2	r6,r3		# point at start of dest
	movc3	r6,(r1),(r3)	# copy count bytes
	ret

/*----------------------------------------------------------------------
 * BFILL - fill a buffer with a single BYTE value

 * bfill(c,buffer,count)
 * BYTE c,*buffer;
 * WORD count;
 * {
 *     while (count-- != 0) *buffer++ = c;
 * }
 */

	.globl	_bfill
_bfill:
	.word	0			# don't zap any C reg variables
	movl	8(ap),r3		# get buffer address
	jbr	7f			# go check if count > 65535
6:					# come here if count > 65535
	subl2	r0,12(ap)		# count -= 65535
	movc5	$0,(r3),4(ap),r0,(r3)	# fill that buffer
7:
	movzwl	$65535,r0		# maximum count in movc5 is 65535
	cmpl	12(ap),r0		# is desired count > 65535?
	jgtr	6b			# yes - go fill 65535 bytes
	movc5	$0,(r3),4(ap),12(ap),(r3)	# no - just use count
	ret
