/* x86_convert.c - convert RImage to XImage with x86 optimizations
 * 
 *  Raster graphics library
 *
 *  Copyright (c) 2000 Alfredo K. Kojima 
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *  
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *  
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>


#ifdef ASM_X86_MMX

int
x86_check_mmx()
{
    static int result = 1;

    if (result >= 0)
	return result;

    result = 0;
#if 0
    asm volatile
	("pushfl		\n" // check whether cpuid supported
	 "pop %%eax		\n"
	 "movl %%eax, %%ebx	\n"
	 "xorl 1<<21, %%eax	\n"
	 "pushfl %%eax		\n"
	 "popfd			\n"
	 "pushfl		\n"
	 "popl %%eax		\n"
	 "xorl %%eax, %%ebx	\n"
	 "andl 1<<21, %%eax	\n"
	 "jz .NotPentium	\n"
	 "xorl %%eax, %%eax	\n"
	 
	 "movl $1, %%eax	\n"
	 "cpuid			\n"
	 "test 1<<23, %%edx	\n"
	 "jz .NotMMX		\n"
	 "movl $1, %%0		\n"

	 ".NotMMX:		\n"
	 ".Bye:			\n"
	 ".NotPentium:		\n"

	 : "=rm" (result));
#endif
    return result;
}


void
x86_TrueColor_32_to_16(unsigned char *image, // 8
		       unsigned short *ximage, // 12
		       short *err, // 16
		       short *nerr, // 20
		       short *rtable, // 24
		       short *gtable, // 28
		       short *btable, // 32
		       int dr, // 36
		       int dg, // 40
		       int db, // 44
		       unsigned int roffs, // 48
		       unsigned int goffs, // 52
		       unsigned int boffs, // 56
		       int width, // 60
		       int height, // 64
		       int line_offset) // 68
{
    /*
     int x; //-4
     long long rrggbbaa;// -16
     long long pixel; //-24
     short *tmp_err; //-32
     short *tmp_nerr; //-36
     */

    asm volatile
	(
	 "subl $64, %esp		\n" // alloc some more stack

	 "pusha				\n"

	 // pack dr, dg and db into mm6
	 "movl 	36(%ebp), %eax		\n"
	 "movl  40(%ebp), %ebx		\n"
	 "movw  %ax, -16(%ebp)		\n"
	 
	 "movw  %bx, -14(%ebp)		\n"
	 "movl  44(%ebp), %eax		\n"
	 "movw  $0, -10(%ebp)		\n"
	 "movw  %ax, -12(%ebp)		\n"

	 "movq  -16(%ebp), %mm6		\n" // dr dg db 0

	 // pack 4|4|4|4 into mm7, for shifting (/16)
	 "movl $0x00040004, -16(%ebp)	\n"
	 "movl $0x00040004, -12(%ebp)	\n"
	 "movq -16(%ebp), %mm7		\n"
	 
	 // store constant values for using with mmx when dithering
	 "movl $0x00070007, -16(%ebp)	\n"
	 "movl $0x00070007, -12(%ebp)	\n"
	 "movq -16(%ebp), %mm5		\n"

	 "movl $0x00050005, -16(%ebp)	\n"
	 "movl $0x00050005, -12(%ebp)	\n"
	 "movq -16(%ebp), %mm4		\n"

	 "movl $0x00030003, -16(%ebp)	\n"
	 "movl $0x00030003, -12(%ebp)	\n"
	 "movq -16(%ebp), %mm3		\n"

	 // process 1 pixel / cycle, each component treated as 16bit
	 "movl 8(%ebp), %esi		\n" // esi = image->data

".LoopY:				\n"
	 "movl 60(%ebp), %eax		\n"
	 "movl %eax, -4(%ebp)		\n" // x = width

	 "movl 64(%ebp), %eax		\n"
	 "decl %eax			\n" // y--
	 "movl %eax, 64(%ebp)		\n"
	 "js .End			\n" // if y < 0, goto end
	 "andl $1, %eax			\n"
	 "jz .LoopY_1			\n" // if (y&1) goto LoopY_1

".LoopY_0:				\n"

	 "movl 16(%ebp), %ebx		\n" // ebx = err
	 "movl %ebx, -36(%ebp)		\n" // [-36] = err
	 "movl 20(%ebp), %eax		\n" //
	 "movl %eax, -32(%ebp)		\n" // [-32] = nerr

	 "jmp .LoopX			\n"

".LoopY_1:				\n"

	 "movl 20(%ebp), %ebx		\n" // ebx = nerr
	 "movl %ebx, -36(%ebp)		\n" // [-36] = nerr
	 "movl 16(%ebp), %eax		\n" //
	 "movl %eax, -32(%ebp)		\n" // [-32] = eerr


".LoopX:				\n"

	 // calculate errors and pixel components
	 
	 // depend on ebx, esi, mm6
	 "movq (%ebx), %mm1		\n" // mm1 = error[0..3]
	 "punpcklbw (%esi), %mm0	\n" // mm0 = image->data[0..3]
	 "psrlw $8, %mm0		\n" // fixup mm0
	 "paddusb %mm1, %mm0		\n" // mm0 = mm0 + mm1 (sat. to 255)
	 "movq %mm0, -24(%ebp)		\n" // save the pixel

	 "movzwl -24(%ebp), %ecx	\n" // ecx = pixel.red
	 "movl 24(%ebp), %edi		\n" // edi = rtable
	 "leal (%edi, %ecx, 2), %eax	\n" // eax = &rtable[pixel.red]
	 "movl (%eax), %edx		\n" // edx = rtable[pixel.red]
	 "movw %dx, -16(%ebp)		\n" // save rr

	 "movzwl -22(%ebp), %ecx	\n" // ecx = pixel.green
	 "movl 28(%ebp), %edi		\n" // edi = gtable
	 "leal (%edi, %ecx, 2), %eax	\n" // eax = &gtable[pixel.green]
	 "movl (%eax), %edx		\n" // ebx = gtable[pixel.green]
	 "movw %dx, -14(%ebp)		\n" // save gg

	 "movzwl -20(%ebp), %ecx	\n" // ecx = pixel.blue
	 "movl 32(%ebp), %edi		\n" // ebx = btable
	 "leal (%edi, %ecx, 2), %eax	\n" // eax = &btable[pixel.blue]
	 "movl (%eax), %edx		\n" // ecx = btable[pixel.blue]
	 "movw %dx, -12(%ebp)		\n" // save bb

	 "movw $0, -10(%ebp)		\n" // save dummy aa

	 "movq -16(%ebp), %mm1		\n" // load mm1 with rrggbbaa
	 "pmullw %mm6, %mm1		\n" // mm1 = rr*dr|...
	 "psubsw %mm1, %mm0		\n" // error = pixel - mm1

	 
	 // distribute the error

	 // depend on mm0, mm7, mm3, mm4, mm5

	 "movl -36(%ebp), %ebx 		\n"

	 "movq %mm0, %mm1		\n"
	 "pmullw %mm5, %mm1		\n" // mm1 = mm1*7
	 "psrlw %mm7, %mm1		\n" // mm1 = mm1/16
	 "paddw	8(%ebx), %mm1		\n"
	 "movq %mm1, 8(%ebx)		\n" // err[x+1,y] = rer*7/16


	 "movl -32(%ebp), %ebx 		\n"

	 "movq %mm0, %mm1		\n"
	 "pmullw %mm4, %mm1		\n" // mm1 = mm1*5
	 "psrlw %mm7, %mm1		\n" // mm1 = mm1/16
	 "paddw -8(%ebx), %mm1		\n"
	 "movq %mm1, -8(%ebx)		\n" // err[x-1,y+1] += rer*3/16

	 "movq %mm0, %mm1		\n"
	 "pmullw %mm3, %mm1		\n" // mm1 = mm1*3
	 "psrlw %mm7, %mm1		\n" // mm1 = mm1/16
	 "paddw 8(%ebx), %mm1		\n"
	 "movq %mm1, (%ebx)		\n" // err[x,y+1] += rer*5/16
	 
	 "psrlw %mm7, %mm0		\n" // mm0 = mm0/16
	 "movq %mm0, 8(%ebx)		\n" // err[x+1,y+1] = rer/16


	 // calculate final pixel value and store
	 "movl 48(%ebp), %ecx		\n"
	 "movw -16(%ebp), %ax		\n"
	 "shlw %cl, %ax			\n" //NP* ax = r<<roffs
	 
	 "movl 52(%ebp), %ecx		\n"
	 "movw -14(%ebp), %bx		\n"
	 "shlw %cl, %bx			\n" //NP*
	 "orw %bx, %ax			\n"
	 
	 "movl 56(%ebp), %ecx		\n"
	 "movw -12(%ebp), %bx		\n"
	 "shlw %cl, %bx			\n" //NP*
	 "orw %bx, %ax			\n"

	 "movl 12(%ebp), %edx		\n"
	 "movw %ax, (%edx)		\n"
	 "addl $2, %edx			\n" // increment ximage
	 "movl %edx, 12(%ebp)		\n"
	 
	 // prepare for next iteration on X

	 "addl $8, -32(%ebp)		\n" // nerr += 8

	 "movl -36(%ebp), %ebx		\n"
	 "addl $8, %ebx			\n"
	 "movl %ebx, -36(%ebp)		\n" // ebx = err += 8
	 
	 
	 // Note: in the last pixel, this would cause an invalid memory access
	 // because, punpcklbw is used (which reads 8 bytes) and the last
	 // pixel is only 4 bytes. This is no problem because the image data
	 // was allocated with extra 4 bytes when created.
	 "addl $4, %esi			\n" // image->data += 4
	 

	 "decl -4(%ebp)			\n" // x--
	 "jnz .LoopX			\n" // if x>0, goto .LoopX


	 // depend on edx
	 "addl 68(%ebp), %edx		\n" // add extra offset to ximage
	 "movl %edx, 12(%ebp)		\n"
	 

	 "jmp .LoopY			\n"

".End:					\n" // THE END
	 
	 "emms				\n"

	 "popa				\n"
	 );
}



#endif /* ASM_X86_MMX */

