


inline static void
cycle_bench(int start) 
{
    static volatile unsigned int start_cycle;

    if (start) {
	asm volatile
	    ("pushl %%edx	\n\t"
	     "rdtsc         	\n\t"
	     "movl %%eax,%0  	\n\t"
	     "cld            	\n\t"
	     "popl %%edx	\n\t"
	     "nop            	\n\t"
	     "nop            	\n\t"
	     "nop            	\n\t"
	     "nop            	\n\t"
	     "nop            	\n\t"
	     "nop       	\n\t"
	     "nop            	\n\t"
	     "nop    	        \n\t"
	     : "=m" (start_cycle) : : "eax", "edx");
    } else {
	volatile int end;

	asm volatile
	    ("pushl %%edx	\n\t"
	     "clc            	\n\t"
	     "rdtsc          	\n\t"
	     "movl %%eax, %0	\n\t"
	     "popl %%edx	\n\t"
	     : "=m" (end) : : "eax", "edx");
	
	printf("Cycle count = %u\n", end - start_cycle - 68);
    }
}



#if 0
// seems linux doesnt allow user progs to exec rdpcm..
inline static void
cache_bench(int start) 
{
    static int start_cycle;
    
    if (start) {
	asm volatile(
		"movl $1,%%ecx	\n\t"
		"rdpmc         \n\t"
		"movl %%eax,%0  \n\t"
                 : "=m" (start_cycle));
    } else {
	int end;
	
	asm volatile(
		"movl $1,%%ecx	\n\t"
                "rdpmc          \n\t"
                "movl %%eax,%0"
                 : "=m" (end));
	
	printf("Cache reloads counted = %i\n", end - start_cycle);
    }
}

#endif
