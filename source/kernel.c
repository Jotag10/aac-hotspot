#include "kernel.h"
#include <cstdio>
#include <stdlib.h>
void volatile kernel(float *result, float *temp, float *power, size_t c_start, size_t size, size_t col, size_t r,
					  float Cap_1, float Rx_1, float Ry_1, float Rz_1, float amb_temp)
{
	//DUVIDAS
	// "memory"- diz que é usado quando se faz leituras ou escritas para itens nao listados como inputs ou outputs, no exemplo nao ha esses casos.
	// %[]
	// ifs, como resolver loads nao paralelos
	//"sub x3, x2, %[col]\n\t", nao reconhece o registo, diz que esta a usar como valor imediato
	
#if defined(NEON)

	#if defined (NEON_UNRO12)
	
		#define NEON_STRIDE 8
		
	#elif defined (NEON_UNRO13)
	
		#define NEON_STRIDE 16
		
	#else
		
		#define NEON_STRIDE 4
		
	#endif
    size_t iter = 0, rem = 0;
    if(size < NEON_STRIDE)
    {
        for ( int c = c_start; c < c_start + size; ++c ) 
        {
            /* Update Temperatures */
            result[r*col+c] =temp[r*col+c]+ ( Cap_1 * (power[r*col+c] + 
				(temp[(r+1)*col+c] + temp[(r-1)*col+c] - 2.f*temp[r*col+c]) * Ry_1 + 
                (temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c]) * Rx_1 + 
                (amb_temp - temp[r*col+c]) * Rz_1));
        }
        
         return;
    }
	//float *teste = (float *) calloc (4, sizeof(float));
    iter = (size+c_start) / NEON_STRIDE * NEON_STRIDE;

	#if defined (NEON_UNROl)
	
	//NEON V2
		asm volatile (
         
			 "lsl x1, %[c], #2 \n\t"				//iterador c=c_start
			 "lsl x2, %[r], #2 \n\t"
			 "ld1r { v0.4s } , [%[Rx]]\n\t"
			 "ld1r { v1.4s } , [%[Ry]]\n\t"
			 "ld1r { v2.4s } , [%[Rz]]\n\t"
			 "ld1r { v3.4s } , [%[amb]]\n\t"
			 "ld1r { v4.4s } , [%[ca]]\n\t"
			 "fmov v9.4s , #2\n\t"
			 "madd x2, x2, %[col], x1\n\t"			//(r*col+c)

			 "add x3, %[temp], x2\n\t"				//*temp[r*col+c]
			 "add x4, %[pow], x2\n\t"				//*power[r*col+c]
			 "add x5, %[res], x2\n\t"				//*result[r*col+c]
			 "add x2, %[sz], x3\n\t"				//* last temp[r*col+c]					
			 
			 ".loop_neon:\n\t"
			 "mov x6, x3\n\t"						//cópia de *temp[r*col+c]
			 "ld1 { v5.4s }, [x3]\n\t"				//temp[r*col+c]
			 "fsub v6.4s, v3.4s, v5.4s\n\t"			//v6 auxiliar, (amb_temp - temp[r*col+c])
			 "fmul v7.4s, v6.4s, v2.4s\n\t"			//v7 acumulador
			 "sub x6, x3, #4\n\t"					//*temp[r*col+c-1]
			 "ld1 { v8.4s }, [x6]\n\t"				//v8 auxiliar, temp[r*col+c-1]
			 "add x6, x3, #4 \n\t"					//*temp[r*col+c+1]
			 "ld1 { v6.4s }, [x6]\n\t"				//v6 auxiliar, temp[r*col+c+1]
			 "fadd v6.4s, v6.4s, v8.4s\n\t"			//v6 auxiliar, temp[r*col+c+1]+temp[r*col+c-1]
			 "fmls v6.4s, v5.4s, v9.4s\n\t"			//v6 auxiliar, (temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c])
			 "fmla v7.4s, v6.4s, v0.4s\n\t"			//v7 acumulador 
			 "add x6, x3, %[col], LSL #2\n\t"		//*temp[(r+1)*col+c+1]
			 "ld1 { v6.4s }, [x3]\n\t"				//v6 auxiliar, temp[(r+1)*col+c]
			 "sub x6, x3, %[col], LSL #2\n\t"		//*temp[(r-1)*col+c+1]
			 "add x3, x3, #16\n\t"					//*temp[r*col+c+4]
			 "ld1 { v8.4s }, [x6]\n\t"				//v8 auxiliar, temp[(r-1)*col+c]
			 "fadd v6.4s, v6.4s, v8.4s\n\t"			//v6 auxiliar, temp[(r+1)*col+c]+temp[(r-1)*col+c]
			 "fmls v6.4s, v5.4s, v9.4s\n\t"			//v6 auxiliar, (temp[(r+1)*col+c]+temp[(r-1)*col+c] - 2.f*temp[r*col+c])
			 "fmla v7.4s, v6.4s, v1.4s\n\t"			//v7 acumulador
			 "ld1 { v6.4s }, [x4], #16\n\t"			//v6 auxiliar, power[r*col+c]
			 "fadd v8.4s, v6.4s, v7.4s\n\t"			//v8 auxiliar, acumulador(v7)+power[r+*col+c]
			 "fmla v5.4s, v8.4s, v4.4s\n\t"			//result[r*col+c]
			 "st1 { v5.4s }, [x5], #16\n\t"
			 "cmp x3, x2\n\t"
			 "b.lt .loop_neon\n\t"

			 : [res] "+r" (result)
			 : [c] "r" (c_start), [Rx] "r" (&Rx_1), [Ry] "r" (&Ry_1), [Rz] "r" (&Rz_1), [amb] "r" (&amb_temp), [ca] "r" (&Cap_1), [temp] "r" (temp),
			 [pow] "r" (power), [r] "r" (r), [col] "r" (col), [sz] "r" (iter*4)
			 : "x1", "x2", "x3", "x4", "x5", "x6", "memory", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9"
		);
	
	#elif (NEON_UNROl2)
	
		asm volatile (
         
			 "lsl x1, %[c], #2 \n\t"				//iterador c=c_start
			 "lsl x2, %[r], #2 \n\t"
			 "ld1r { v0.4s } , [%[Rx]]\n\t"
			 "ld1r { v1.4s } , [%[Ry]]\n\t"
			 "ld1r { v2.4s } , [%[Rz]]\n\t"
			 "ld1r { v3.4s } , [%[amb]]\n\t"
			 "ld1r { v4.4s } , [%[ca]]\n\t"
			 "fmov v9.4s , #2\n\t"
			 "madd x2, x2, %[col], x1\n\t"			//(r*col+c)

			 "add x3, %[temp], x2\n\t"				//*temp[r*col+c]
			 "add x4, %[pow], x2\n\t"				//*power[r*col+c]
			 "add x5, %[res], x2\n\t"				//*result[r*col+c]
			 "add x2, %[sz], x3\n\t"				//* last temp[r*col+c]					
			 
			 ".loop_neon:\n\t"
			 "mov x6, x3\n\t"						//cópia de *temp[r*col+c]
			 "ld1 { v5.4s, v6.4s }, [x3]\n\t"		//v5 e v6 temp[r*col+c]
			 "fsub v10.4s, v3.4s, v5.4s\n\t"		//v10 auxiliar, (amb_temp - temp[r*col+c])
			 "fsub v14.4s, v3.4s, v6.4s\n\t"		//v14 auxiliar,(amb_temp - temp[r*col+c])
			 "fmul v15.4s, v10.4s, v2.4s\n\t"		//v15 acumulador
			 "fmul v20.4s, v14.4s, v2.4s\n\t"		//v20 acumulador
			 "sub x6, x3, #4\n\t"					//*temp[r*col+c-1]
			 "ld1 { v16.4s, v17.4s }, [x6]\n\t"		//v16, v17 auxiliar, temp[r*col+c-1]
			 "add x6, x3, #4 \n\t"					//*temp[r*col+c+1]
			 "ld1 { v10.4s, v11.4s }, [x6]\n\t"		//v10, v11 auxiliar, temp[r*col+c+1]
			 "fadd v10.4s, v10.4s, v16.4s\n\t"		//v10 auxiliar, temp[r*col+c+1]+temp[r*col+c-1]
			 "fadd v11.4s, v11.4s, v17.4s\n\t"		//14 auxiliar, temp[r*col+c+1]+temp[r*col+c-1]
			 "fmls v10.4s, v5.4s, v9.4s\n\t"		//v10 auxiliar, (temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c])
			 "fmls v11.4s, v6.4s, v9.4s\n\t"		//v11 auxiliar, (temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c])
			 "fmla v15.4s, v10.4s, v0.4s\n\t"		//v15 acumulador 
			 "fmla v20.4s, v11.4s, v0.4s\n\t"		//v20 acumulador 
			 "add x6, x3, %[col], LSL #2\n\t"		//*temp[(r+1)*col+c+1]
			 "ld1 { v10.4s, v11.4s }, [x3]\n\t"		//v10, v11 auxiliar, temp[(r+1)*col+c]
			 "sub x6, x3, %[col], LSL #2\n\t"		//*temp[(r-1)*col+c+1]
			 "add x3, x3, #32\n\t"					//*temp[r*col+c+4]
			 "ld1 { v16.4s, v17.4s }, [x6]\n\t"		//v16, v17 auxiliar, temp[(r-1)*col+c]
			 "fadd v10.4s, v10.4s, v16.4s\n\t"		//v10 auxiliar, temp[(r+1)*col+c]+temp[(r-1)*col+c]
			 "fadd v11.4s, v11.4s, v17.4s\n\t"		//v11 auxiliar, temp[(r+1)*col+c]+temp[(r-1)*col+c]
			 "fmls v10.4s, v5.4s, v9.4s\n\t"		//v10 auxiliar, (temp[(r+1)*col+c]+temp[(r-1)*col+c] - 2.f*temp[r*col+c])
			 "fmls v11.4s, v6.4s, v9.4s\n\t"		//v11 auxiliar, (temp[(r+1)*col+c]+temp[(r-1)*col+c] - 2.f*temp[r*col+c])
			 "fmla v15.4s, v10.4s, v1.4s\n\t"		//v15 acumulador
			 "fmla v20.4s, v11.4s, v1.4s\n\t"		//v20 acumulador
			 "ld1 { v10.4s, v11.4s }, [x4], #32\n\t"//v10 auxiliar, power[r*col+c]
			 "fadd v16.4s, v10.4s, v15.4s\n\t"		//v16 auxiliar, acumulador(v15)+power[r+*col+c]
			 "fadd v17.4s, v11.4s, v20.4s\n\t"		//v17 auxiliar, acumulador(v15)+power[r+*col+c]
			 "fmla v5.4s, v16.4s, v4.4s\n\t"			//result[r*col+c]
			 "fmla v6.4s, v17.4s, v4.4s\n\t"		//result[r*col+c]
			 
			 "st1 { v5.4s, v6.4s }, [x5], #32\n\t"
			 "cmp x3, x2\n\t"
			 "b.lt .loop_neon\n\t"

			 : [res] "+r" (result)
			 : [c] "r" (c_start), [Rx] "r" (&Rx_1), [Ry] "r" (&Ry_1), [Rz] "r" (&Rz_1), [amb] "r" (&amb_temp), [ca] "r" (&Cap_1), [temp] "r" (temp),
			 [pow] "r" (power), [r] "r" (r), [col] "r" (col), [sz] "r" (iter*4)
			 : "x1", "x2", "x3", "x4", "x5", "x6", "memory", "v1", "v2", "v3", "v4", "v5", "v6", "v9", "v10", "v11", "v14", "v15", "v16", "v17", "v20"
		);

	#else
		 asm volatile (
			 
			 "lsl x1, %[c], #2 \n\t"				//iterador c=c_start
			 "lsl x2, %[r], #2 \n\t"
			 "ld1r { v0.4s } , [%[Rx]]\n\t"
			 "ld1r { v1.4s } , [%[Ry]]\n\t"
			 "ld1r { v2.4s } , [%[Rz]]\n\t"
			 "ld1r { v3.4s } , [%[amb]]\n\t"
			 "ld1r { v4.4s } , [%[ca]]\n\t"
			 "fmov v9.4s , #2\n\t"
			 "madd x2, x2, %[col], x1\n\t"			//(r*col+c)
					 
			 ".loop_neon:\n\t"
			 "ldr q5, [%[temp], x2]\n\t"			//temp[r*col+c]
			 "fsub v6.4s, v3.4s, v5.4s\n\t"			//v6 auxiliar, (amb_temp - temp[r*col+c])
			 "fmul v7.4s, v6.4s, v2.4s\n\t"			//v7 acumulador
			 "sub x3, x2, #4\n\t"					//r*col+c-1
			 "ldr q8, [%[temp], x3]\n\t"			//v8 auxiliar, temp[r*col+c-1]
			 "add x3, x2, #4 \n\t"					//r*col+c+1
			 "ldr q6, [%[temp], x3]\n\t"			//v6 auxiliar, temp[r*col+c+1]
			 "fadd v6.4s, v6.4s, v8.4s\n\t"			//v6 auxiliar, temp[r*col+c+1]+temp[r*col+c-1]
			 "fmls v6.4s, v5.4s, v9.4s\n\t"			//v6 auxiliar, (temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c])
			 "fmla v7.4s, v6.4s, v0.4s\n\t"			//v7 acumulador 
			 "add x3, x2, %[col], LSL #2\n\t"		//(r+1)*col+c= (r*col+c)*4+col*4=4(r*col+c+col)
			 "ldr q6, [%[temp], x3]\n\t"			//v6 auxiliar, temp[(r+1)*col+c]
			 "sub x3, x2, %[col], LSL #2\n\t"		//(r-1)*col+c
			 "ldr q8, [%[temp], x3]\n\t"			//v8 auxiliar, temp[(r-1)*col+c]
			 "fadd v6.4s, v6.4s, v8.4s\n\t"			//v6 auxiliar, temp[(r+1)*col+c]+temp[(r-1)*col+c]
			 "fmls v6.4s, v5.4s, v9.4s\n\t"			//v6 auxiliar, (temp[(r+1)*col+c]+temp[(r-1)*col+c] - 2.f*temp[r*col+c])
			 "fmla v7.4s, v6.4s, v1.4s\n\t"			//v7 acumulador
			 "ldr q6, [%[pow], x2]\n\t"				//v6 auxiliar, power[r*col+c]
			 "fadd v8.4s, v6.4s, v7.4s\n\t"			//v8 auxiliar, acumulador(v7)+power[r+*col+c]
			 "fmla v5.4s, v8.4s, v4.4s\n\t"			//result[r*col+c]
			 "str q5, [%[res], x2]\n\t"
			 "add x2, x2, #16\n\t"					//r*col+c+4
			 "add x1, x1, #16\n\t"					//c+4
			 "cmp x1, %[sz]\n\t"
			 "b.lt .loop_neon\n\t"
			
			 : [res] "+r" (result)
			 : [c] "r" (c_start), [Rx] "r" (&Rx_1), [Ry] "r" (&Ry_1), [Rz] "r" (&Rz_1), [amb] "r" (&amb_temp), [ca] "r" (&Cap_1), [temp] "r" (temp),
			 [pow] "r" (power), [r] "r" (r), [col] "r" (col), [sz] "r" (iter*4)
			 : "x1", "x2", "x3", "memory", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9"
		);
	#endif
	
	/*CHECK VALUES */
	
	for ( int c = c_start; c < iter; ++c ) 
	{
		float teste1 =temp[r*col+c]+ ( Cap_1 * (power[r*col+c] + 
			(temp[(r+1)*col+c] + temp[(r-1)*col+c] - 2.f*temp[r*col+c]) * Ry_1 + 
			(temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c]) * Rx_1 + 
			(amb_temp - temp[r*col+c]) * Rz_1));
		
		printf("normal: %f, new: %f\n", teste1, result[r*col+c]);
	}
	
    rem = (size+c_start) % NEON_STRIDE;
    
    for ( int c = iter; c < rem + iter; ++c ) 
    {
        result[r*col+c] =temp[r*col+c]+ ( Cap_1 * (power[r*col+c] + 
            (temp[(r+1)*col+c] + temp[(r-1)*col+c] - 2.f*temp[r*col+c]) * Ry_1 + 
            (temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c]) * Rx_1 + 
            (amb_temp - temp[r*col+c]) * Rz_1));
    }

#elif defined(SVE)
/*
    asm volatile (
        "mov x4, #0\n\t"
        "whilelt p0.s, x4, %[sz]\n\t"
        "ld1rw z0.s, p0/z, %[a]\n\t"
        ".loop_sve:\n\t"
        "ld1w z1.s, p0/z, [%[x], x4, lsl #2]\n\t"
        "ld1w z2.s, p0/z, [%[y], x4, lsl #2]\n\t"
        "fmla z2.s, p0/m, z1.s, z0.s\n\t"
        "st1w z2.s, p0, [%[y], x4, lsl #2]\n\t"
        "incw x4\n\t"
        "whilelt p0.s, x4, %[sz]\n\t"
        "b.first .loop_sve\n\t"
        : [y] "+r" (y)
        : [sz] "r" (size-1), [a] "m" (A), [x] "r" (x)
        : "x4", "x5", "x6", "memory", "p0", "z0", "z1", "z2"
    );
*/
#else

    for ( int c = c_start; c < c_start + size; ++c ) 
    {
        /* Update Temperatures */
        result[r*col+c] =temp[r*col+c]+ ( Cap_1 * (power[r*col+c] + 
            (temp[(r+1)*col+c] + temp[(r-1)*col+c] - 2.f*temp[r*col+c]) * Ry_1 + 
            (temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c]) * Rx_1 + 
            (amb_temp - temp[r*col+c]) * Rz_1));
    }
#endif
}
