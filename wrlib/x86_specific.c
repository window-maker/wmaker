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

#ifdef ASM_X86


#ifdef ASM_X86_MMX

int
x86_check_mmx()
{
    static int result = -1;

    if (result >= 0)
	return result;

    result = 0;

    asm volatile
        ("pushal		\n" // please don't forget this in any asm
         "pushfl		\n" // check whether cpuid supported
	 "pop %%eax		\n"
	 "movl %%eax, %%ebx	\n"
	 "xorl $(1<<21), %%eax	\n"
	 "pushl %%eax		\n"
	 "popfl			\n"
	 "pushfl		\n"
	 "popl %%eax		\n"
	 "xorl %%ebx, %%eax	\n"
	 "andl $(1<<21), %%eax	\n"
	 "jz .NotPentium	\n"
	 "xorl %%eax, %%eax	\n" // no eax effect because of the movl below
	                            // except reseting flags. is it needed?
	 "movl $1, %%eax	\n"
	 "cpuid			\n"
	 "test $(1<<23), %%edx	\n"
	 "jz .NotMMX		\n"

         "popal			\n" // popal needed because the address of
         "movl $1, %0		\n" // variable %0 may be kept in a register
         "jmp .noPop		\n"

	 ".NotMMX:		\n"
	 ".NotPentium:		\n"
         "popal			\n"
         ".noPop:		\n"

	 : "=m" (result));

    return result;
}


/*
 * TODO:
 * 		32/8	24/8	32/16	24/16	32/24	24/24
 * PPlain	YES	YES
 * MMX				DONE
 *
 * 
 * - try to align stack (local variable space) into quadword boundary
 */




void
x86_mmx_TrueColor_32_to_16(unsigned char *image, // 8
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
	 "subl $128, %esp		\n" // alloc some more stack

	 "pushal       			\n"

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

".LoopYa:				\n"
	 "movl 60(%ebp), %eax		\n"
	 "movl %eax, -4(%ebp)		\n" // x = width

	 "movl 64(%ebp), %eax		\n"
	 "decl %eax			\n" // y--
	 "movl %eax, 64(%ebp)		\n"
	 "js .Enda			\n" // if y < 0, goto end
	 "andl $1, %eax			\n"
	 "jz .LoopY_1a			\n" // if (y&1) goto LoopY_1

".LoopY_0a:				\n"

	 "movl 16(%ebp), %ebx		\n" // ebx = err
	 "movl %ebx, -36(%ebp)		\n" // [-36] = err
	 "movl 20(%ebp), %eax		\n" //
	 "movl %eax, -32(%ebp)		\n" // [-32] = nerr

	 "jmp .LoopXa			\n"

".LoopY_1a:				\n"

	 "movl 20(%ebp), %ebx		\n" // ebx = nerr
	 "movl %ebx, -36(%ebp)		\n" // [-36] = nerr
	 "movl 16(%ebp), %eax		\n" //
	 "movl %eax, -32(%ebp)		\n" // [-32] = eerr

	 ".align 16			\n"
".LoopXa:				\n"

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
	 "movw (%eax), %dx		\n" // dx = rtable[pixel.red]
	 "movw %dx, -16(%ebp)		\n" // save rr

	 "movzwl -22(%ebp), %ecx	\n" // ecx = pixel.green
	 "movl 28(%ebp), %edi		\n" // edi = gtable
	 "leal (%edi, %ecx, 2), %eax	\n" // eax = &gtable[pixel.green]
	 "movw (%eax), %dx		\n" // dx = gtable[pixel.green]
	 "movw %dx, -14(%ebp)		\n" // save gg

	 "movzwl -20(%ebp), %ecx	\n" // ecx = pixel.blue
	 "movl 32(%ebp), %edi		\n" // ebx = btable
	 "leal (%edi, %ecx, 2), %eax	\n" // eax = &btable[pixel.blue]
	 "movw (%eax), %dx		\n" // dx = btable[pixel.blue]
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
	 "jnz .LoopXa			\n" // if x>0, goto .LoopX


	 // depend on edx
	 "addl 68(%ebp), %edx		\n" // add extra offset to ximage
	 "movl %edx, 12(%ebp)		\n"
	 

	 "jmp .LoopYa			\n"

".Enda:					\n" // THE END
	 
	 "emms				\n"

	 "popal				\n"
	 );
}


#endif /* ASM_X86_MMX */



void
x86_PseudoColor_to_8(unsigned char *image, // 8
		     unsigned char *ximage, // 12
		     char *err, // 16
		     char *nerr, // 20
		     short *ctable, // 24
		     int dr, // 28
		     int dg, // 32
		     int db, // 36
		     unsigned long *pixels, // 40
		     int cpc, // 44
		     int width, // 48
		     int height, // 52
		     int bytesPerPixel, // 56
		     int line_offset) // 60
{
    /*
     * int x; -4
     * int cpcpc; -8
     * 
     * int rr; -12
     * int gg; -16
     * int bb; -20
     * 
     * char ndr; -21
     * char ndg; -22
     * char ndb; -23
     * 
     * char *err; -32
     * char *nerr; -36
     * 
     */
    asm volatile
	(
	 "subl $128, %esp		\n" // alloc some stack space
	 "pushal       			\n"

	 "movl 44(%ebp), %eax		\n"
	 "mulb 44(%ebp)			\n"
	 "movl %eax, -8(%ebp)		\n" // cpcpc = cpc*cpc
	 
	 // eax will always be <= 0xffff

	 // process 1 pixel / cycle, each component treated as 16bit
	 "movl 8(%ebp), %esi		\n" // esi = image->data

".LoopYb:				\n"
	 "movl 48(%ebp), %ecx		\n"
	 "movl %ecx, -4(%ebp)		\n" // x = width

	 "movl 52(%ebp), %ecx		\n"
	 "decl %ecx			\n" // y--
	 "movl %ecx, 52(%ebp)		\n"
	 "js .Endb			\n" // if y < 0, goto end
	 "andl $1, %ecx			\n"
	 "jz .LoopY_1b			\n" // if (y&1) goto LoopY_1

".LoopY_0b:				\n"

	 "movl 16(%ebp), %ebx		\n" // ebx = err
//	 "movl %ebx, -36(%ebp)		\n" // [-36] = err
	 "movl 20(%ebp), %ecx		\n" //
	 "movl %ecx, -32(%ebp)		\n" // [-32] = nerr

	 "movl $0, (%ecx)		\n" // init error of nerr[0] to 0

	 "jmp .LoopXb			\n"

".LoopY_1b:				\n"

	 "movl 20(%ebp), %ebx		\n" // ebx = nerr
//	 "movl %ebx, -36(%ebp)		\n" // [-36] = nerr
	 "movl 16(%ebp), %ecx		\n" //
	 "movl %ecx, -32(%ebp)		\n" // [-32] = err

	 "movl $0, (%ecx)		\n" // init error of nerr[0] to 0

	 
	 ".align 16			\n"
".LoopXb:				\n"
	 

	 "movl 24(%ebp), %edi		\n" // edi = ctable
	 "xorl %edx, %edx		\n" // zero the upper word on edx

	 // RED

	 // depends on ebx==err, esi==image->data, edi
	 "movzbw (%esi), %dx		\n" // dx = image->data[0]
	 "movsbw (%ebx), %ax		\n" // ax = error[0]
	 "addw %ax, %dx			\n" // pixel.red = data[0] + error[0]

	 "testb %dh, %dh		\n" // test if pixel.red < 0 or > 255
	 "jz .OKRb			\n" // 0 <= pixel.red <= 255
	 "js .NEGRb			\n" // pixel.red < 0
	 "movw $0xff, %dx		\n" // pixel.red > 255
	 "jmp .OKRb			\n"
".NEGRb:				\n"
	 "xorw %dx, %dx			\n"
".OKRb:					\n"
	 "leal (%edi, %edx, 2), %ecx	\n" // ecx = &ctable[pixel.red]
	 "movl (%ecx), %eax		\n" // ax = ctable[pixel.red]
	 "movw %ax, -12(%ebp)		\n" // save rr

	 "mulb 28(%ebp)			\n" // ax = rr*dr
	 "subw %ax, %dx			\n" // rer = dx = dx - rr*dr

	 "movswl %dx, %eax		\n" // save rer

	 // distribute error
	 "leal (, %eax, 8), %ecx	\n"
	 "subw %dx, %cx			\n" // cx = rer * 7
	 "sarw $4, %cx			\n" // cx = rer * 7 / 16
	 "addb %cl, 4(%ebx)		\n" // err[x+1] += rer * 7 / 16
	 
	 "movl -32(%ebp), %ecx		\n" // ecx = nerr
	 
	 "leaw (%eax, %eax, 4), %dx	\n" // dx = rer * 5
	 "sarw $4, %dx			\n" // dx = rer * 5 / 16
	 "addb %dl, (%ecx)		\n" // nerr[x] += rer * 5 / 16

	 "leaw (%eax, %eax, 2), %dx	\n" // dx = rer * 3
	 "sarw $4, %dx			\n" // dx = rer * 3 / 16
	 "addb %dl, -4(%ecx)		\n" // nerr[x-1] += rer * 3 / 16

	 "sarw $4, %ax			\n" // ax = rer / 16
	 "movb %al, 4(%ecx)		\n" // nerr[x+1] = rer / 16

	 
	 // GREEN

	 // depends on ebx, esi, edi
	 "movzbw 1(%esi), %dx		\n" // dx = image->data[1]
	 "movsbw 1(%ebx), %ax		\n" // ax = error[1]
	 "addw %ax, %dx			\n" // pixel.grn = data[1] + error[1]

	 "testb %dh, %dh		\n" // test if pixel.grn < 0 or > 255
	 "jz .OKGb			\n" // 0 <= pixel.grn <= 255
	 "js .NEGGb			\n" // pixel.grn < 0
	 "movw $0xff, %dx		\n" // pixel.grn > 255
	 "jmp .OKGb			\n"
".NEGGb:				\n"
	 "xorw %dx, %dx			\n"
".OKGb:					\n"
	 "leal (%edi, %edx, 2), %ecx	\n" // ecx = &ctable[pixel.grn]
	 "movw (%ecx), %ax		\n" // ax = ctable[pixel.grn]
	 "movw %ax, -16(%ebp)		\n" // save gg

	 "mulb 28(%ebp)			\n" // ax = gg*dg
	 "subw %ax, %dx			\n" // ger = dx = dx - gg*dg

	 "movswl %dx, %eax		\n" // save ger

	 // distribute error

	 "leal (, %eax, 8), %ecx	\n"
	 "subw %dx, %cx			\n" // cx = ger * 7
	 "sarw $4, %cx			\n" // cx = ger * 7 / 16
	 "addb %cl, 5(%ebx)		\n" // err[x+1] += ger * 7 / 16
	 
	 "movl -32(%ebp), %ecx		\n" // ecx = nerr
	 
	 "leaw (%eax, %eax, 4), %dx	\n" // dx = ger * 5
	 "sarw $4, %dx			\n" // dx = ger * 5 / 16
	 "addb %dl, 1(%ecx)		\n" // nerr[x] += ger * 5 / 16

	 "leaw (%eax, %eax, 2), %dx	\n" // dx = ger * 3
	 "sarw $4, %dx			\n" // dx = ger * 3 / 16
	 "addb %dl, -3(%ecx)		\n" // nerr[x-1] += ger * 3 / 16

	 "sarw $4, %ax			\n" // ax = ger / 16
	 "movb %al, 5(%ecx)		\n" // nerr[x+1] = ger / 16

	 
	 // BLUE

	 // depends on ebx, esi
	 "movzbw 2(%esi), %dx		\n" // dx = image->data[2]
	 "movsbw 2(%ebx), %ax		\n" // ax = error[2]
	 "addw %ax, %dx			\n" // pixel.grn = data[2] + error[2]
	 
	 "testb %dh, %dh		\n" // test if pixel.blu < 0 or > 255
	 "jz .OKBb			\n" // 0 <= pixel.blu <= 255
	 "js .NEGBb			\n" // pixel.blu < 0
	 "movw $0xff, %dx		\n" // pixel.blu > 255
	 "jmp .OKBb			\n"
".NEGBb:				\n"
	 "xorw %dx, %dx			\n"
".OKBb:					\n"
	 "leal (%edi, %edx, 2), %ecx	\n" // ecx = &ctable[pixel.blu]
	 "movw (%ecx), %ax		\n" // ax = ctable[pixel.blu]
	 "movw %ax, -20(%ebp)		\n" // save bb

	 "mulb 28(%ebp)			\n" // ax = bb*db
	 "subw %ax, %dx			\n" // ber = dx = dx - bb*db
	 "movswl %dx, %eax		\n" // save ber
	 
	 // distribute error
	 "leal (, %eax, 8), %ecx	\n"
	 "subw %dx, %cx			\n" // cx = ber * 7
	 "sarw $4, %cx			\n" // cx = ber * 7 / 16
	 "addb %cl, 6(%ebx)		\n" // err[x+1] += ber * 7 / 16
	 
	 "movl -32(%ebp), %ecx		\n" // ecx = nerr
	 
	 "leaw (%eax, %eax, 4), %dx	\n" // dx = ber * 5
	 "sarw $4, %dx			\n" // dx = ber * 5 / 16
	 "addb %dl, 2(%ecx)		\n" // nerr[x] += ber * 5 / 16

	 "leaw (%eax, %eax, 2), %dx	\n" // dx = ber * 3
	 "sarw $4, %dx			\n" // dx = ber * 3 / 16
	 "addb %dl, -4(%ecx)		\n" // nerr[x-1] += ber * 3 / 16

	 "sarw $4, %ax			\n" // ax = ber / 16
	 "movb %al, 6(%ecx)		\n" // nerr[x+1] = ber / 16

	 "andl $0xffff, %eax		\n"
	 // depends on eax & 0xffff0000 == 0
	 // calculate the index of the value of the pixel
	 "movw -12(%ebp), %ax		\n" // ax = rr
	 "mulb -8(%ebp)			\n" // ax = cpcpc*rr
	 "movw %ax, %cx			\n"
	 "movw -16(%ebp), %ax		\n" // ax = gg
	 "mulb 44(%ebp)			\n" // ax = cpc*gg
	 "addw %cx, %ax			\n" // ax = cpc*gg + cpcpc*rr
	 "addw -20(%ebp), %ax		\n" // ax = cpcpc*rr + cpc*gg + bb

	 "movl 40(%ebp), %ecx		\n"
	 "leal (%ecx, %eax, 4), %edx	\n"
	 "movb (%edx), %cl		\n" // cl = pixels[ax]

	 // store the pixel
	 "movl 12(%ebp), %eax		\n"
	 "movb %cl, (%eax)		\n" // *ximage = cl
	 "incl 12(%ebp)			\n" // ximage++

	 // prepare for next iteration on X
	 
	 "addl $4, -32(%ebp)		\n" // nerr += 4
	 "addl $4, %ebx			\n" // err += 4

	 "addl 56(%ebp), %esi		\n" // image->data += bpp

	 "decl -4(%ebp)			\n" // x--
	 "jnz .LoopXb			\n" // if x>0, goto .LoopX

	 
	 "movl 60(%ebp), %eax		\n"
	 "addl %eax, 12(%ebp)		\n" // add extra offset to ximage
	 
	 "jmp .LoopYb			\n"

".Endb:					\n"
	 
	 "emms				\n"
	 "popal				\n"
	 );
    
    
}

#endif /* ASM_X86 */
