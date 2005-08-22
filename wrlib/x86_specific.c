/* x86_convert.c - convert RImage to XImage with x86 optimizations
 *
 * Raster graphics library
 *
 * Copyright (c) 2000-2003 Alfredo K. Kojima
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
        ("pushal                \n\t" // please dont forget this in any asm
         "pushfl                \n\t" // check whether cpuid supported
         "pop %%eax             \n\t"
         "movl %%eax, %%ebx     \n\t"
         "xorl $(1<<21), %%eax  \n\t"
         "pushl %%eax           \n\t"
         "popfl                 \n\t"
         "pushfl                \n\t"
         "popl %%eax            \n\t"
         "xorl %%ebx, %%eax     \n\t"
         "andl $(1<<21), %%eax  \n\t"
         "jz .NotPentium        \n\t"
         "xorl %%eax, %%eax     \n\t" // no eax effect because of the movl below
                                      // except reseting flags. is it needed?
         "movl $1, %%eax        \n\t"
         "cpuid                 \n\t"
         "test $(1<<23), %%edx  \n\t"
         "jz .NotMMX            \n\t"

         "popal                 \n\t" // popal needed because the address of
         "movl $1, %0           \n\t" // variable %0 may be kept in a register
         "jmp .noPop            \n"

".NotMMX:                       \n"
".NotPentium:                   \n\t"
         "popal                 \n"
".noPop:                        \n\t"

         : "=m" (result));

    return result;
}


/*
 * TODO:
 *              32/8    24/8    32/16   24/16   32/24   24/24
 * PPlain       YES     YES
 * MMX                          DONE
 *
 *
 * - try to align stack (local variable space) into quadword boundary
 */
void
x86_mmx_TrueColor_32_to_16(unsigned char *image,
                           unsigned short *ximage,
                           short *err,
                           short *nerr,
                           unsigned short *rtable,
                           unsigned short *gtable,
                           unsigned short *btable,
                           int dr,
                           int dg,
                           int db,
                           unsigned int roffs,
                           unsigned int goffs,
                           unsigned int boffs,
                           int width,
                           int height,
                           int line_offset)
{
    union {
        long long rrggbbaa;
        struct {
            short int rr, gg, bb, aa;
        } words;
    } rrggbbaa;

    union {
        long long pixel;
        struct {
            short int rr, gg, bb, aa;
        } words;
    } pixel;

    short *tmp_err;
    short *tmp_nerr;
    int x;

    asm volatile
        (
         "pushl %%ebx                   \n\t"

         // pack dr, dg and db into mm6
         "movl  %7, %%eax               \n\t"
         "movl  %8, %%ebx               \n\t"
         "movl  %9, %%ecx               \n\t"
         "movw  %%ax, %16               \n\t"
         "movw  %%bx, %17               \n\t"
         "movw  %%cx, %18               \n\t"
         "movw  $0,  %19                \n\t"

         "movq  %16, %%mm6              \n\t" // dr dg db 0

         // pack 4|4|4|4 into mm7, for shifting (/16)
         "movl $0x00040004, %16         \n\t"
         "movl $0x00040004, %18         \n\t"
         "movq %16, %%mm7               \n\t"

         // store constant values for using with mmx when dithering
         "movl $0x00070007, %16         \n\t"
         "movl $0x00070007, %18         \n\t"
         "movq %16, %%mm5               \n\t"

         "movl $0x00050005, %16         \n\t"
         "movl $0x00050005, %18         \n\t"
         "movq %16, %%mm4               \n\t"

         "movl $0x00030003, %16         \n\t"
         "movl $0x00030003, %18         \n\t"
         "movq %16, %%mm3               \n\t"

         // process 1 pixel / cycle, each component treated as 16bit
         "movl %0, %%esi                \n"   // esi = image->data

".LoopYa:                               \n\t"
         "movl %13, %%eax               \n\t"
         "movl %%eax, %26               \n\t" // x = width

         "movl %14, %%eax               \n\t"
         "decl %%eax                    \n\t" // y--
         "movl %%eax, %14               \n\t"
         "js .Enda                      \n\t" // if y < 0, goto end
         "andl $1, %%eax                \n\t"
         "jz .LoopY_1a                  \n"   // if (y&1) goto LoopY_1

".LoopY_0a:                             \n\t"

         "movl %2, %%ebx                \n\t" // ebx = err
         "movl %%ebx, %25               \n\t" // [-36] = err
         "movl %3, %%eax                \n\t" //
         "movl %%eax, %24               \n\t" // [-32] = nerr

         "jmp .LoopXa                   \n"

".LoopY_1a:                             \n\t"

         "movl %3, %%ebx                \n\t" // ebx = nerr
         "movl %%ebx, %25               \n\t" // [-36] = nerr
         "movl %2, %%eax                \n\t" //
         "movl %%eax, %24               \n\t" // [-32] = eerr

         ".align 16                     \n"
".LoopXa:                               \n\t"

         // calculate errors and pixel components

         // depend on ebx, esi, mm6
         "movq (%%ebx), %%mm1           \n\t" // mm1 = error[0..3]
         "punpcklbw (%%esi), %%mm0      \n\t" // mm0 = image->data[0..3]
         "psrlw $8, %%mm0               \n\t" // fixup mm0
         "paddusb %%mm1, %%mm0          \n\t" // mm0 = mm0 + mm1 (sat. to 255)
         "movq %%mm0, %20               \n\t" // save the pixel

         "movzwl %20, %%ecx             \n\t" // ecx = pixel.red
         "movl %4, %%edi                \n\t" // edi = rtable
         //agi
         "leal (%%edi, %%ecx, 2), %%eax \n\t" // eax = &rtable[pixel.red]
         // agi
         "movw (%%eax), %%dx            \n\t" // dx = rtable[pixel.red]
         "movw %%dx, %16                \n\t" // save rr

         "movzwl %21, %%ecx             \n\t" // ecx = pixel.green
         "movl %5, %%edi                \n\t" // edi = gtable
         //agi
         "leal (%%edi, %%ecx, 2), %%eax \n\t" // eax = &gtable[pixel.green]
         //agi
         "movw (%%eax), %%dx            \n\t" // dx = gtable[pixel.green]
         "movw %%dx, %17                \n\t" // save gg

         "movzwl %22, %%ecx             \n\t" // ecx = pixel.blue
         "movl %6, %%edi                \n\t" // ebx = btable
         //agi
         "leal (%%edi, %%ecx, 2), %%eax \n\t" // eax = &btable[pixel.blue]
         //agi
         "movw (%%eax), %%dx            \n\t" // dx = btable[pixel.blue]
         "movw %%dx, %18                \n\t" // save bb

         "movw $0, %19                  \n\t" // save dummy aa

         "movq %16, %%mm1               \n\t" // load mm1 with rrggbbaa
         "pmullw %%mm6, %%mm1           \n\t" // mm1 = rr*dr|...
         "psubsw %%mm1, %%mm0           \n\t" // error = pixel - mm1


         // distribute the error

         // depend on mm0, mm7, mm3, mm4, mm5

         "movl %25, %%ebx               \n\t"

         "movq %%mm0, %%mm1             \n\t"
         "pmullw %%mm5, %%mm1           \n\t" // mm1 = mm1*7
         "psrlw %%mm7, %%mm1            \n\t" // mm1 = mm1/16
         "paddw 8(%%ebx), %%mm1         \n\t"
         "movq %%mm1, 8(%%ebx)          \n\t" // err[x+1,y] = rer*7/16


         "movl %24, %%ebx               \n\t"

         "movq %%mm0, %%mm1             \n\t"
         "pmullw %%mm4, %%mm1           \n\t" // mm1 = mm1*5
         "psrlw %%mm7, %%mm1            \n\t" // mm1 = mm1/16
         "paddw -8(%%ebx), %%mm1        \n\t"
         "movq %%mm1, -8(%%ebx)         \n\t" // err[x-1,y+1] += rer*3/16

         "movq %%mm0, %%mm1             \n\t"
         "pmullw %%mm3, %%mm1           \n\t" // mm1 = mm1*3
         "psrlw %%mm7, %%mm1            \n\t" // mm1 = mm1/16
         "paddw 8(%%ebx), %%mm1         \n\t"
         "movq %%mm1, (%%ebx)           \n\t" // err[x,y+1] += rer*5/16

         "psrlw %%mm7, %%mm0            \n\t" // mm0 = mm0/16
         "movq %%mm0, 8(%%ebx)          \n\t" // err[x+1,y+1] = rer/16


         // calculate final pixel value and store
         "movl %10, %%ecx               \n\t"
         "movw %16, %%ax                \n\t"
         "shlw %%cl, %%ax               \n\t" //NP* ax = r<<roffs

         "movl %11, %%ecx               \n\t"
         "movw %17, %%bx                \n\t"
         "shlw %%cl, %%bx               \n\t" //NP*
         "orw %%bx, %%ax                \n\t"

         "movl %12, %%ecx               \n\t"
         "movw %18, %%bx                \n\t"
         "shlw %%cl, %%bx               \n\t" //NP*
         "orw %%bx, %%ax                \n\t"

         "movl %1, %%edx                \n\t"
         "movw %%ax, (%%edx)            \n\t"
         "addl $2, %%edx                \n\t" // increment ximage
         "movl %%edx, %1                \n\t"

         // prepare for next iteration on X

         "addl $8, %24                  \n\t" // nerr += 8

         "movl %25, %%ebx               \n\t"
         "addl $8, %%ebx                \n\t"
         "movl %%ebx, %25               \n\t" // ebx = err += 8


         // Note: in the last pixel, this would cause an invalid memory access
         // because, punpcklbw is used (which reads 8 bytes) and the last
         // pixel is only 4 bytes. This is no problem because the image data
         // was allocated with extra 4 bytes when created.
         "addl $4, %%esi                \n\t" // image->data += 4


         "decl %26                      \n\t" // x--
         "jnz .LoopXa                   \n\t" // if x>0, goto .LoopX


         // depend on edx
         "addl %15, %%edx               \n\t" // add extra offset to ximage
         "movl %%edx, %1                \n\t"


         "jmp .LoopYa                   \n"

".Enda:                                 \n\t" // THE END
         "emms                          \n\t"
         "popl %%ebx                    \n\t"
         :
         :
         "m" (image),                      // %0
         "m" (ximage),                     // %1
         "m" (err),                        // %2
         "m" (nerr),                       // %3
         "m" (rtable),                     // %4
         "m" (gtable),                     // %5
         "m" (btable),                     // %6
         "m" (dr),                         // %7
         "m" (dg),                         // %8
         "m" (db),                         // %9
         "m" (roffs),                      // %10
         "m" (goffs),                      // %11
         "m" (boffs),                      // %12
         "m" (width),                      // %13
         "m" (height),                     // %14
         "m" (line_offset),                // %15
         "m" (rrggbbaa.words.rr),          // %16 (access to rr)
         "m" (rrggbbaa.words.gg),          // %17 (access to gg)
         "m" (rrggbbaa.words.bb),          // %18 (access to bb)
         "m" (rrggbbaa.words.aa),          // %19 (access to aa)
         "m" (pixel.words.rr),             // %20 (access to pixel.r)
         "m" (pixel.words.gg),             // %21 (access to pixel.g)
         "m" (pixel.words.bb),             // %22 (access to pixel.b)
         "m" (pixel.words.aa),             // %23 (access to pixel.a)
         "m" (tmp_err),                    // %24
         "m" (tmp_nerr),                   // %25
         "m" (x)                           // %26
         : "eax", "ecx", "edx", "esi", "edi"
        );
}


void
x86_mmx_TrueColor_24_to_16(unsigned char *image,
                           unsigned short *ximage,
                           short *err,
                           short *nerr,
                           short *rtable,
                           short *gtable,
                           short *btable,
                           int dr,
                           int dg,
                           int db,
                           unsigned int roffs,
                           unsigned int goffs,
                           unsigned int boffs,
                           int width,
                           int height,
                           int line_offset)
{
    union {
        long long rrggbbaa;
        struct {
            short int rr, gg, bb, aa;
        } words;
    } rrggbbaa;
    
    union {
        long long pixel;
        struct {
            short int rr, gg, bb, aa;
        } words;
    } pixel;

    short *tmp_err;
    short *tmp_nerr;

    int x;
    int w1;
    int w2;

    asm volatile
        (
         "pushl %%ebx                   \n\t"

         "movl %13, %%eax               \n\t" // eax = width
         "movl %%eax, %%ebx             \n\t"
         "shrl $2, %%eax                \n\t"
         "movl %%eax, %27               \n\t" // w1 = width / 4
         "andl $3, %%ebx                \n\t"
         "movl %%ebx, %28               \n"   // w2 = width %% 4


".LoopYc:                               \n\t"
         "movl %13, %%eax               \n\t"
         "movl %%eax, %26               \n\t" // x = width

         "decl %14                      \n\t" // height--
         "js .Endc                      \n\t" // if height < 0 then end

         "movl %14, %%eax               \n\t"
         "decl %%eax                    \n\t" // y--
         "movl %%eax, %14               \n\t"
         "js .Endc                      \n\t" // if y < 0, goto end
         "andl $1, %%eax                \n\t"
         "jz .LoopY_1c                  \n"   // if (y&1) goto LoopY_1

".LoopY_0c:                             \n\t"

         "movl %2, %%ebx                \n\t" // ebx = err
         "movl %%ebx, %25               \n\t" // [-36] = err
         "movl %3, %%eax                \n\t" //
         "movl %%eax, %24               \n\t" // [-32] = nerr

         "jmp .LoopX_1c                 \n"

".LoopY_1c:                             \n\t"

         "movl %3, %%ebx                \n\t" // ebx = nerr
         "movl %%ebx, %25               \n\t" // [-36] = nerr
         "movl %2, %%eax                \n\t" //
         "movl %%eax, %24               \n\t" // [-32] = eerr

         ".align 16                     \n\t"

         "movl %%eax, %26               \n"   // x = w1
".LoopX_1c:                             \n\t"
         "decl %26                      \n\t" // x--
         "js .Xend1_c                   \n\t" // if x < 0 then end

         // do conversion of 4 pixels
         "movq %2, %%mm0                \n\t" // mm0 = err




         "jmp .LoopX_1c                 \n"
".Xend1_c:                              \n\t"

         "movl %28, %%eax               \n\t"
         "movl %%eax, %26               \n"   // x = w2
".LoopX_2c:                             \n\t"
         "decl %26                      \n\t" // x--
         "js .Xend2_c                   \n\t" //
         // do conversion
         "jmp .LoopX_2c                 \n"
".Xend2_c:                              \n\t"

         "movl %27, %%eax               \n\t"
         "jmp .LoopYc                   \n"

".Endc:                                 \n\t" // THE END
         "emms                          \n\t"
         "popl %%ebx                    \n\t"
         :
         :
         "m" (image),                      // %0
         "m" (ximage),                     // %1
         "m" (err),                        // %2
         "m" (nerr),                       // %3
         "m" (rtable),                     // %4
         "m" (gtable),                     // %5
         "m" (btable),                     // %6
         "m" (dr),                         // %7
         "m" (dg),                         // %8
         "m" (db),                         // %9
         "m" (roffs),                      // %10
         "m" (goffs),                      // %11
         "m" (boffs),                      // %12
         "m" (width),                      // %13
         "m" (height),                     // %14
         "m" (line_offset),                // %15
         "m" (rrggbbaa.words.rr),          // %16 (access to rr)
         "m" (rrggbbaa.words.gg),          // %17 (access to gg)
         "m" (rrggbbaa.words.bb),          // %18 (access to bb)
         "m" (rrggbbaa.words.aa),          // %19 (access to aa)
         "m" (pixel.words.rr),             // %20 (access to pixel.r)
         "m" (pixel.words.gg),             // %21 (access to pixel.g)
         "m" (pixel.words.bb),             // %22 (access to pixel.b)
         "m" (pixel.words.aa),             // %23 (access to pixel.a)
         "m" (tmp_err),                    // %24
         "m" (tmp_nerr),                   // %25
         "m" (x),                          // %26
         "m" (w1),                         // %27
         "m" (w2)                          // %28
         : "eax", "ecx", "edx", "esi", "edi"
        );
}



#endif /* ASM_X86_MMX */



void
x86_PseudoColor_32_to_8(unsigned char *image,
                        unsigned char *ximage,
                        char *err,
                        char *nerr,
                        short *ctable,
                        int dr,
                        int dg,
                        int db,
                        unsigned long *pixels,
                        int cpc,
                        int width,
                        int height,
                        int bytesPerPixel,
                        int line_offset)
{
    int x;
    int cpcpc;

    int rr;
    int gg;
    int bb;

    char *tmp_err;
    char *tmp_nerr;

    char ndr; // aparently not used
    char ndg; // aparently not used
    char ndb; // aparently not used

    asm volatile
        (
         "pushal                        \n\t"

         "movl %9, %%eax                \n\t"
         "mulb %9                       \n\t"
         "movl %%eax, %15               \n\t" // cpcpc = cpc*cpc

         // eax will always be <= 0xffff

         // process 1 pixel / cycle, each component treated as 16bit
         "movl %0, %%esi                \n"   // esi = image->data

".LoopYb:                               \n\t"
         "movl %10, %%ecx               \n\t"
         "movl %%ecx, %14               \n\t" // x = width

         "movl %11, %%ecx               \n\t"
         "decl %%ecx                    \n\t" // y--
         "movl %%ecx, %11               \n\t"
         "js .Endb                      \n\t" // if y < 0, goto end
         "andl $1, %%ecx                \n\t"
         "jz .LoopY_1b                  \n"   // if (y&1) goto LoopY_1

".LoopY_0b:                             \n\t"

         "movl %2, %%ebx                \n\t" // ebx = err
//useless "movl %%ebx, %20              \n\t" // [-36] = err
         "movl %3, %%ecx                \n\t" //
         "movl %%ecx, %19               \n\t" // [-32] = nerr

         "movl $0, (%%ecx)              \n\t" // init error of nerr[0] to 0

         "jmp .LoopXb                   \n"

".LoopY_1b:                             \n\t"

         "movl %3, %%ebx                \n\t" // ebx = nerr
//useless "movl %%ebx, %20              \n\t" // [-36] = nerr
         "movl %2, %%ecx                \n\t" //
         "movl %%ecx, %19               \n\t" // [-32] = err

         "movl $0, (%%ecx)              \n\t" // init error of nerr[0] to 0


         ".align 16                     \n"
".LoopXb:                               \n\t"


         "movl %4, %%edi                \n\t" // edi = ctable
         "xorl %%edx, %%edx             \n\t" // zero the upper word on edx

         // RED

         // depends on ebx==err, esi==image->data, edi
         "movzbw (%%esi), %%dx          \n\t" // dx = image->data[0]
         "movsbw (%%ebx), %%ax          \n\t" // ax = error[0]
         "addw %%ax, %%dx               \n\t" // pixel.red = data[0] + error[0]

         "testb %%dh, %%dh              \n\t" // test if pixel.red < 0 or > 255
         "jz .OKRb                      \n\t" // 0 <= pixel.red <= 255
         "js .NEGRb                     \n\t" // pixel.red < 0
         "movw $0xff, %%dx              \n\t" // pixel.red > 255
         "jmp .OKRb                     \n"
".NEGRb:                                \n\t"
         "xorw %%dx, %%dx               \n"
".OKRb:                                 \n\t"
         //partial reg
         "leal (%%edi, %%edx, 2), %%ecx \n\t" // ecx = &ctable[pixel.red]
         //agi
         "movl (%%ecx), %%eax           \n\t" // ax = ctable[pixel.red]
         "movw %%ax, %16                \n\t" // save rr

         "mulb %5                       \n\t" // ax = rr*dr
         "subw %%ax, %%dx               \n\t" // rer = dx = dx - rr*dr

         "movswl %%dx, %%eax            \n\t" // save rer

         // distribute error
         "leal (, %%eax, 8), %%ecx      \n\t"
         "subw %%dx, %%cx               \n\t" // cx = rer * 7
         "sarw $4, %%cx                 \n\t" // cx = rer * 7 / 16
         "addb %%cl, 4(%%ebx)           \n\t" // err[x+1] += rer * 7 / 16

         "movl %19, %%ecx               \n\t" // ecx = nerr

         "leaw (%%eax, %%eax, 4), %%dx  \n\t" // dx = rer * 5
         "sarw $4, %%dx                 \n\t" // dx = rer * 5 / 16
         "addb %%dl, (%%ecx)            \n\t" // nerr[x] += rer * 5 / 16

         "leaw (%%eax, %%eax, 2), %%dx  \n\t" // dx = rer * 3
         "sarw $4, %%dx                 \n\t" // dx = rer * 3 / 16
         "addb %%dl, -4(%%ecx)          \n\t" // nerr[x-1] += rer * 3 / 16

         "sarw $4, %%ax                 \n\t" // ax = rer / 16
         "movb %%al, 4(%%ecx)           \n\t" // nerr[x+1] = rer / 16


         // GREEN

         // depends on ebx, esi, edi
         "movzbw 1(%%esi), %%dx         \n\t" // dx = image->data[1]
         "movsbw 1(%%ebx), %%ax         \n\t" // ax = error[1]
         "addw %%ax, %%dx               \n\t" // pixel.grn = data[1] + error[1]

         "testb %%dh, %%dh              \n\t" // test if pixel.grn < 0 or > 255
         "jz .OKGb                      \n\t" // 0 <= pixel.grn <= 255
         "js .NEGGb                     \n\t" // pixel.grn < 0
         "movw $0xff, %%dx              \n\t" // pixel.grn > 255
         "jmp .OKGb                     \n"
".NEGGb:                                \n\t"
         "xorw %%dx, %%dx               \n"
".OKGb:                                 \n\t"
         // partial reg
         "leal (%%edi, %%edx, 2), %%ecx \n\t" // ecx = &ctable[pixel.grn]
         //agi
         "movw (%%ecx), %%ax            \n\t" // ax = ctable[pixel.grn]
         "movw %%ax, %17                \n\t" // save gg

         "mulb %6                       \n\t" // ax = gg*dg
         "subw %%ax, %%dx               \n\t" // ger = dx = dx - gg*dg

         "movswl %%dx, %%eax            \n\t" // save ger

         // distribute error

         "leal (, %%eax, 8), %%ecx      \n\t"
         "subw %%dx, %%cx               \n\t" // cx = ger * 7
         "sarw $4, %%cx                 \n\t" // cx = ger * 7 / 16
         "addb %%cl, 5(%%ebx)           \n\t" // err[x+1] += ger * 7 / 16

         "movl %19, %%ecx               \n\t" // ecx = nerr

         "leaw (%%eax, %%eax, 4), %%dx  \n\t" // dx = ger * 5
         "sarw $4, %%dx                 \n\t" // dx = ger * 5 / 16
         "addb %%dl, 1(%%ecx)           \n\t" // nerr[x] += ger * 5 / 16

         "leaw (%%eax, %%eax, 2), %%dx  \n\t" // dx = ger * 3
         "sarw $4, %%dx                 \n\t" // dx = ger * 3 / 16
         "addb %%dl, -3(%%ecx)          \n\t" // nerr[x-1] += ger * 3 / 16

         "sarw $4, %%ax                 \n\t" // ax = ger / 16
         "movb %%al, 5(%%ecx)           \n\t" // nerr[x+1] = ger / 16


         // BLUE

         // depends on ebx, esi
         "movzbw 2(%%esi), %%dx         \n\t" // dx = image->data[2]
         "movsbw 2(%%ebx), %%ax         \n\t" // ax = error[2]
         "addw %%ax, %%dx               \n\t" // pixel.grn = data[2] + error[2]

         "testb %%dh, %%dh              \n\t" // test if pixel.blu < 0 or > 255
         "jz .OKBb                      \n\t" // 0 <= pixel.blu <= 255
         "js .NEGBb                     \n\t" // pixel.blu < 0
         "movw $0xff, %%dx              \n\t" // pixel.blu > 255
         "jmp .OKBb                     \n"
".NEGBb:                                \n\t"
         "xorw %%dx, %%dx               \n"
".OKBb:                                 \n\t"
         //partial reg
         "leal (%%edi, %%edx, 2), %%ecx \n\t" // ecx = &ctable[pixel.blu]
         //agi
         "movw (%%ecx), %%ax            \n\t" // ax = ctable[pixel.blu]
         "movw %%ax, %18                \n\t" // save bb

         "mulb %7                       \n\t" // ax = bb*db
         "subw %%ax, %%dx               \n\t" // ber = dx = dx - bb*db
         "movswl %%dx, %%eax            \n\t" // save ber

         // distribute error
         "leal (, %%eax, 8), %%ecx      \n\t"
         "subw %%dx, %%cx               \n\t" // cx = ber * 7
         "sarw $4, %%cx                 \n\t" // cx = ber * 7 / 16
         "addb %%cl, 6(%%ebx)           \n\t" // err[x+1] += ber * 7 / 16

         "movl %19, %%ecx               \n\t" // ecx = nerr

         "leaw (%%eax, %%eax, 4), %%dx  \n\t" // dx = ber * 5
         "sarw $4, %%dx                 \n\t" // dx = ber * 5 / 16
         "addb %%dl, 2(%%ecx)           \n\t" // nerr[x] += ber * 5 / 16

         "leaw (%%eax, %%eax, 2), %%dx  \n\t" // dx = ber * 3
         "sarw $4, %%dx                 \n\t" // dx = ber * 3 / 16
         "addb %%dl, -4(%%ecx)          \n\t" // nerr[x-1] += ber * 3 / 16

         "sarw $4, %%ax                 \n\t" // ax = ber / 16
         "movb %%al, 6(%%ecx)           \n\t" // nerr[x+1] = ber / 16

         "andl $0xffff, %%eax           \n\t"
         // depends on eax & 0xffff0000 == 0
         // calculate the index of the value of the pixel
         "movw %16, %%ax                \n\t" // ax = rr
         "mulb %15                      \n\t" // ax = cpcpc*rr
         "movw %%ax, %%cx               \n\t"
         "movw %17, %%ax                \n\t" // ax = gg
         "mulb %9                       \n\t" // ax = cpc*gg
         "addw %%cx, %%ax               \n\t" // ax = cpc*gg + cpcpc*rr
         "addw %18, %%ax                \n\t" // ax = cpcpc*rr + cpc*gg + bb

         "movl %8, %%ecx                \n\t"
         //agi
         "leal (%%ecx, %%eax, 4), %%edx \n\t"
         //agi
         "movb (%%edx), %%cl            \n\t" // cl = pixels[ax]

         // store the pixel
         "movl %1, %%eax                \n\t"
         "movb %%cl, (%%eax)            \n\t" // *ximage = cl
         "incl %1                       \n\t" // ximage++

         // prepare for next iteration on X

         "addl $4, %19                  \n\t" // nerr += 4
         "addl $4, %%ebx                \n\t" // err += 4

         "addl %12, %%esi               \n\t" // image->data += bpp

         "decl %14                      \n\t" // x--
         "jnz .LoopXb                   \n\t" // if x>0, goto .LoopX


         "movl %13, %%eax               \n\t"
         "addl %%eax, %1                \n\t" // add extra offset to ximage

         "jmp .LoopYb                   \n"

".Endb:                                 \n\t"
         "emms                          \n\t"
         "popal                         \n\t"
         :
         :
         "m" (image),         // %0
         "m" (ximage),        // %1
         "m" (err),           // %2
         "m" (nerr),          // %3
         "m" (ctable),        // %4
         "m" (dr),            // %5
         "m" (dg),            // %6
         "m" (db),            // %7
         "m" (pixels),        // %8
         "m" (cpc),           // %9
         "m" (width),         // %10
         "m" (height),        // %11
         "m" (bytesPerPixel), // %12
         "m" (line_offset),   // %13
         "m" (x),             // %14
         "m" (cpcpc),         // %15
         "m" (rr),            // %16
         "m" (gg),            // %17
         "m" (bb),            // %18
         "m" (tmp_err),       // %19
         "m" (tmp_nerr),      // %20
         "m" (ndr),           // %21
         "m" (ndg),           // %22
         "m" (ndb)            // %23
        );
}

#endif /* ASM_X86 */

