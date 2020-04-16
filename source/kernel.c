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
	
#if defined(NEON)

	#define NEON_STRIDE 4
	int unroll =1;
	
	#if defined (NEON_UNROl)
	
		unroll =1;

	#elif defined (NEON_UNROl2)
	
		unroll =2;

	#elif defined (NEON_UNROl3)
	
		unroll =4;
		
	#else
		
		unroll =1;
		
	#endif
    size_t iter = 0, rem = 0;
	

	
    if(size < NEON_STRIDE*unroll)
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
	//float *teste = (float *) calloc (300, sizeof(float));
    iter = (size+c_start) / (NEON_STRIDE*unroll) * (NEON_STRIDE*unroll);

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

			 "add x3, %[temp], x2\n\t"				//x3, *temp[r*col+c]
			 "sub x11, x2, #4\n\t"					//r*col+c-1
			 "add x4, %[pow], x2\n\t"				//x4, *power[r*col+c]
			 "add x6, %[temp], x11\n\t"				//x6, *temp[r*col+c-1]
			 "add x11, x2, #4\n\t"					//r*col+c+1
			 "add x5, %[res], x2\n\t"				//x5, *result[r*col+c]				
			 "add x7, %[temp], x11\n\t"				//x7, *temp[r*col+c+1]
			 "add x11, x2, %[col], lsl #2\n\t"		//(r+1)*col+c
			 "add x9, %[temp], x11\n\t"				//x9, *temp[(r+1)*col+c]
			 "sub x11, x2, %[col], lsl #2\n\t"		//(r-1)*col+c
			 "add x10, %[temp], x11\n\t"			//x10,*temp[(r-1)*col+c]
			 "add x2, %[sz], x3\n\t"				//* last temp[r*col+c]	
			 			 
			 
			 ".loop_neon:\n\t"
			 //"prfm PLDL1STRM, [x3, #16]\n\t"
			 //"prfm PLDL1STRM, [x4, #16]\n\t"
			 //"prfm PSTL1STRM, [x5, #16]\n\t"
			 
			 "ld1 { v5.4s }, [x3], #16\n\t"			//temp[r*col+c]
			 "fsub v6.4s, v3.4s, v5.4s\n\t"			//v6 auxiliar, (amb_temp - temp[r*col+c])
			 "fmul v7.4s, v6.4s, v2.4s\n\t"			//v7 acumulador
			 "ld1 { v8.4s }, [x6], #16\n\t"			//v8 auxiliar, temp[r*col+c-1]
			 "ld1 { v6.4s }, [x7], #16\n\t"			//v6 auxiliar, temp[r*col+c+1]
			 "fadd v6.4s, v6.4s, v8.4s\n\t"			//v6 auxiliar, temp[r*col+c+1]+temp[r*col+c-1]
			 "fmls v6.4s, v5.4s, v9.4s\n\t"			//v6 auxiliar, (temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c])
			 "fmla v7.4s, v6.4s, v0.4s\n\t"			//v7 acumulador 
			 "ld1 { v6.4s }, [x9], #16\n\t"			//v6 auxiliar, temp[(r+1)*col+c]
			 "ld1 { v8.4s }, [x10], #16\n\t"		//v8 auxiliar, temp[(r-1)*col+c]
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
			 : "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x9", "x10","x11", "memory","v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9"
		);
    
    #elif defined (NEON_UNROl2)
		asm volatile (
         
			 "lsl x1, %[c], #2 \n\t"					//iterador c=c_start
			 "lsl x2, %[r], #2 \n\t"
			 "ld1r { v0.4s } , [%[Rx]]\n\t"
			 "ld1r { v1.4s } , [%[Ry]]\n\t"
			 "ld1r { v2.4s } , [%[Rz]]\n\t"
			 "ld1r { v3.4s } , [%[amb]]\n\t"
			 "ld1r { v4.4s } , [%[ca]]\n\t"
			 "fmov v9.4s , #2\n\t"
			 "madd x2, x2, %[col], x1\n\t"				//(r*col+c)
				
			 "add x3, %[temp], x2\n\t"					//x3, *temp[r*col+c]
			 "sub x11, x2, #4\n\t"						//r*col+c-1
			 "add x4, %[pow], x2\n\t"					//x4, *power[r*col+c]
			 "add x6, %[temp], x11\n\t"					//x6, *temp[r*col+c-1]
			 "add x11, x2, #4\n\t"						//r*col+c+1
			 "add x5, %[res], x2\n\t"					//x5, *result[r*col+c]				
			 "add x7, %[temp], x11\n\t"					//x7, *temp[r*col+c+1]
			 "add x11, x2, %[col], lsl #2\n\t"			//(r+1)*col+c
			 "add x9, %[temp], x11\n\t"					//x9, *temp[(r+1)*col+c]
			 "sub x11, x2, %[col], lsl #2\n\t"			//(r-1)*col+c
			 "add x10, %[temp], x11\n\t"				//x10,*temp[(r-1)*col+c]
			 "add x2, %[sz], x3\n\t"					//* last temp[r*col+c]					
			 
			 ".loop_neon:\n\t"
			 //"prfm PLDL1STRM, [x6, #28]\n\t"
			 //"prfm PLDL1STRM, [x4, #32]\n\t"
			 //"prfm PSTL1STRM, [x5, #32]\n\t"				
			 "ld1 { v5.4s, v6.4s }, [x3], #32\n\t"		//v5 e v6 temp[r*col+c]
			 "fsub v10.4s, v3.4s, v5.4s\n\t"			//v10 auxiliar, (amb_temp - temp[r*col+c])
			 "fsub v14.4s, v3.4s, v6.4s\n\t"			//v14 auxiliar,(amb_temp - temp[r*col+c])
			 "fmul v15.4s, v10.4s, v2.4s\n\t"			//v15 acumulador
			 "fmul v20.4s, v14.4s, v2.4s\n\t"			//v20 acumulador
			 "ld1 { v16.4s, v17.4s }, [x6], #32\n\t"	//v16, v17 auxiliar, temp[r*col+c-1]
			 "ld1 { v10.4s, v11.4s }, [x7], #32\n\t"	//v10, v11 auxiliar, temp[r*col+c+1]
			 "fadd v10.4s, v10.4s, v16.4s\n\t"			//v10 auxiliar, temp[r*col+c+1]+temp[r*col+c-1]
			 "fadd v11.4s, v11.4s, v17.4s\n\t"			//14 auxiliar, temp[r*col+c+1]+temp[r*col+c-1]
			 "fmls v10.4s, v5.4s, v9.4s\n\t"			//v10 auxiliar, (temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c])
			 "fmls v11.4s, v6.4s, v9.4s\n\t"			//v11 auxiliar, (temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c])
			 "fmla v15.4s, v10.4s, v0.4s\n\t"			//v15 acumulador 
			 "fmla v20.4s, v11.4s, v0.4s\n\t"			//v20 acumulador 
			 "ld1 { v10.4s, v11.4s }, [x9], #32\n\t"	//v10, v11 auxiliar, temp[(r+1)*col+c]
			 "ld1 { v16.4s, v17.4s }, [x10], #32\n\t"	//v16, v17 auxiliar, temp[(r-1)*col+c]
			 "fadd v10.4s, v10.4s, v16.4s\n\t"			//v10 auxiliar, temp[(r+1)*col+c]+temp[(r-1)*col+c]
			 "fadd v11.4s, v11.4s, v17.4s\n\t"			//v11 auxiliar, temp[(r+1)*col+c]+temp[(r-1)*col+c]
			 "fmls v10.4s, v5.4s, v9.4s\n\t"			//v10 auxiliar, (temp[(r+1)*col+c]+temp[(r-1)*col+c] - 2.f*temp[r*col+c])
			 "fmls v11.4s, v6.4s, v9.4s\n\t"			//v11 auxiliar, (temp[(r+1)*col+c]+temp[(r-1)*col+c] - 2.f*temp[r*col+c])
			 "fmla v15.4s, v10.4s, v1.4s\n\t"			//v15 acumulador
			 "fmla v20.4s, v11.4s, v1.4s\n\t"			//v20 acumulador
			 "ld1 { v10.4s, v11.4s }, [x4], #32\n\t"	//v10 auxiliar, power[r*col+c]
			 "fadd v16.4s, v10.4s, v15.4s\n\t"			//v16 auxiliar, acumulador(v15)+power[r+*col+c]
			 "fadd v17.4s, v11.4s, v20.4s\n\t"			//v17 auxiliar, acumulador(v15)+power[r+*col+c]
			 "fmla v5.4s, v16.4s, v4.4s\n\t"			//result[r*col+c]
			 "fmla v6.4s, v17.4s, v4.4s\n\t"			//result[r*col+c]
			 
			 "st1 { v5.4s, v6.4s }, [x5], #32\n\t"
			 "cmp x3, x2\n\t"
			 "b.lt .loop_neon\n\t"

			 : [res] "+r" (result)
			 : [c] "r" (c_start), [Rx] "r" (&Rx_1), [Ry] "r" (&Ry_1), [Rz] "r" (&Rz_1), [amb] "r" (&amb_temp), [ca] "r" (&Cap_1), [temp] "r" (temp),
			 [pow] "r" (power), [r] "r" (r), [col] "r" (col), [sz] "r" (iter*4)
			 : "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x9", "x10","x11", "memory", "v0", "v1",
			 "v2", "v3", "v4", "v5", "v6", "v9", "v10", "v11", "v14", "v15", "v16", "v17", "v20"
		);


	#elif defined (NEON_UNROl3)
		asm volatile (
         
			 "lsl x1, %[c], #2 \n\t"									//iterador c=c_start
			 "lsl x2, %[r], #2 \n\t"
			 "ld1r { v0.4s } , [%[Rx]]\n\t"
			 "ld1r { v1.4s } , [%[Ry]]\n\t"
			 "ld1r { v2.4s } , [%[Rz]]\n\t"
			 "ld1r { v3.4s } , [%[amb]]\n\t"
			 "ld1r { v4.4s } , [%[ca]]\n\t"
			 "fmov v9.4s , #2\n\t"
			 "madd x2, x2, %[col], x1\n\t"								//r*col+c

			 "add x3, %[temp], x2\n\t"									//x3, *temp[r*col+c]
			 "sub x11, x2, #4\n\t"										//r*col+c-1
			 "add x4, %[pow], x2\n\t"									//x4, *power[r*col+c]
			 "add x6, %[temp], x11\n\t"									//x6, *temp[r*col+c-1]
			 "add x11, x2, #4\n\t"										//r*col+c+1
			 "add x5, %[res], x2\n\t"									//x5, *result[r*col+c]				
			 "add x7, %[temp], x11\n\t"									//x7, *temp[r*col+c+1]
			 "add x11, x2, %[col], lsl #2\n\t"							//(r+1)*col+c
			 "add x9, %[temp], x11\n\t"									//x9, *temp[(r+1)*col+c]
			 "sub x11, x2, %[col], lsl #2\n\t"							//(r-1)*col+c
			 "add x10, %[temp], x11\n\t"								//x10,*temp[(r-1)*col+c]
			 "add x2, %[sz], x3\n\t"									//* last temp[r*col+c]		
			 
			 
			 ".loop_neon:\n\t"
			 "prfm PLDL1STRM, [x3, #60]\n\t"
			 "prfm PLDL1STRM, [x9, #64]\n\t"
			 "prfm PLDL1STRM, [x10, #64]\n\t"
			 "prfm PLDL1STRM, [x4, #64]\n\t"
			 "prfm PSTL1STRM, [x5, #64]\n\t"
			 "ld1 { v5.4s, v6.4s , v7.4s , v8.4s}, [x3], #64\n\t"		//v5 ,v6 ,v7 e v8 temp[r*col+c]
			 "fsub v10.4s, v3.4s, v5.4s\n\t"		                	//v10 auxiliar, (amb_temp - temp[r*col+c])
			 "fsub v14.4s, v3.4s, v6.4s\n\t"		                	//v14 auxiliar,(amb_temp - temp[r*col+c])
			 "fsub v21.4s, v3.4s, v7.4s\n\t"		                	//v21 auxiliar,(amb_temp - temp[r*col+c])
			 "fsub v22.4s, v3.4s, v8.4s\n\t"		                	//v22 auxiliar,(amb_temp - temp[r*col+c])
			 "fmul v15.4s, v10.4s, v2.4s\n\t"		                	//v15 acumulador
			 "fmul v20.4s, v14.4s, v2.4s\n\t"		                	//v20 acumulador
			 "fmul v23.4s, v21.4s, v2.4s\n\t"		                	//v23 acumulador
			 "fmul v24.4s, v22.4s, v2.4s\n\t"		                	//v24 acumulador
			 "ld1 { v16.4s, v17.4s, v18.4s, v19.4s}, [x6], #64\n\t"		//v16, v17, v18 e v19 auxiliar, temp[r*col+c-1]
			 "ld1 { v10.4s, v11.4s, v12.4s, v13.4s }, [x7], #64\n\t"	//v10, v11, v12, v13 auxiliar, temp[r*col+c+1]
			 "fadd v10.4s, v10.4s, v16.4s\n\t"		                	//v10 auxiliar, temp[r*col+c+1]+temp[r*col+c-1]
			 "fadd v11.4s, v11.4s, v17.4s\n\t"		                	//v11 auxiliar, temp[r*col+c+1]+temp[r*col+c-1]
			 "fadd v12.4s, v12.4s, v18.4s\n\t"		                	//v12 auxiliar, temp[r*col+c+1]+temp[r*col+c-1]
			 "fadd v13.4s, v13.4s, v19.4s\n\t"		                	//v13 auxiliar, temp[r*col+c+1]+temp[r*col+c-1]
			 "fmls v10.4s, v5.4s, v9.4s\n\t"		                	//v10 auxiliar, (temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c])
			 "fmls v11.4s, v6.4s, v9.4s\n\t"		                	//v11 auxiliar, (temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c])
			 "fmls v12.4s, v7.4s, v9.4s\n\t"		                	//v12 auxiliar, (temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c])
			 "fmls v13.4s, v8.4s, v9.4s\n\t"		                	//v13 auxiliar, (temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c])
			 "fmla v15.4s, v10.4s, v0.4s\n\t"		                	//v15 acumulador 
			 "fmla v20.4s, v11.4s, v0.4s\n\t"		                	//v20 acumulador 
			 "fmla v23.4s, v12.4s, v0.4s\n\t"		                	//v23 acumulador 
			 "fmla v24.4s, v13.4s, v0.4s\n\t"		                	//v24 acumulador 
			 "ld1 { v10.4s, v11.4s, v12.4s, v13.4s }, [x9], #64\n\t"	//v10, v11, v12, v13 auxiliar, temp[(r+1)*col+c]
			 "ld1 { v16.4s, v17.4s, v18.4s, v19.4s }, [x10], #64\n\t"	//v16, v17 auxiliar, temp[(r-1)*col+c]
			 "fadd v10.4s, v10.4s, v16.4s\n\t"		                	//v10 auxiliar, temp[(r+1)*col+c]+temp[(r-1)*col+c]
			 "fadd v11.4s, v11.4s, v17.4s\n\t"		                	//v11 auxiliar, temp[(r+1)*col+c]+temp[(r-1)*col+c]
			 "fadd v12.4s, v12.4s, v18.4s\n\t"		                	//v12 auxiliar, temp[(r+1)*col+c]+temp[(r-1)*col+c]
			 "fadd v13.4s, v13.4s, v19.4s\n\t"		                	//v13 auxiliar, temp[(r+1)*col+c]+temp[(r-1)*col+c]
			 "fmls v10.4s, v5.4s, v9.4s\n\t"		                	//v10 auxiliar, (temp[(r+1)*col+c]+temp[(r-1)*col+c] - 2.f*temp[r*col+c])
			 "fmls v11.4s, v6.4s, v9.4s\n\t"		                	//v11 auxiliar, (temp[(r+1)*col+c]+temp[(r-1)*col+c] - 2.f*temp[r*col+c])
			 "fmls v12.4s, v7.4s, v9.4s\n\t"		                	//v12 auxiliar, (temp[(r+1)*col+c]+temp[(r-1)*col+c] - 2.f*temp[r*col+c])
			 "fmls v13.4s, v8.4s, v9.4s\n\t"		                	//v13 auxiliar, (temp[(r+1)*col+c]+temp[(r-1)*col+c] - 2.f*temp[r*col+c])
			 "fmla v15.4s, v10.4s, v1.4s\n\t"		                	//v15 acumulador
			 "ld1 { v10.4s, v11.4s, v12.4s, v13.4s }, [x4], #64\n\t"	//v10, v11, v12, v13 auxiliar, power[r*col+c]
			 "fmla v20.4s, v11.4s, v1.4s\n\t"		                	//v20 acumulador
			 "fmla v23.4s, v12.4s, v1.4s\n\t"		                	//v20 acumulador
			 "fmla v24.4s, v13.4s, v1.4s\n\t"		                	//v20 acumulador
			 "fadd v16.4s, v10.4s, v15.4s\n\t"		                	//v16 auxiliar, acumulador(v15)+power[r+*col+c]
			 "fadd v17.4s, v11.4s, v20.4s\n\t"		                	//v17 auxiliar, acumulador(v15)+power[r+*col+c]
			 "fadd v18.4s, v12.4s, v23.4s\n\t"		                	//v18 auxiliar, acumulador(v15)+power[r+*col+c]
			 "fadd v19.4s, v13.4s, v24.4s\n\t"		                	//v19 auxiliar, acumulador(v15)+power[r+*col+c]
			 "fmla v5.4s, v16.4s, v4.4s\n\t"		                	//result[r*col+c]
			 "fmla v6.4s, v17.4s, v4.4s\n\t"		                	//result[r*col+c]
			 "fmla v7.4s, v18.4s, v4.4s\n\t"		                	//result[r*col+c]
			 "fmla v8.4s, v19.4s, v4.4s\n\t"		                	//result[r*col+c]
			 
			 "st1 { v5.4s, v6.4s, v7.4s, v8.4s }, [x5], #64\n\t"
			 "cmp x3, x2\n\t"
			 "b.lt .loop_neon\n\t"

			 : [res] "+r" (result)
			 : [c] "r" (c_start), [Rx] "r" (&Rx_1), [Ry] "r" (&Ry_1), [Rz] "r" (&Rz_1), [amb] "r" (&amb_temp), [ca] "r" (&Cap_1), [temp] "r" (temp),
			 [pow] "r" (power), [r] "r" (r), [col] "r" (col), [sz] "r" (iter*4)
			 : "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x9", "x10","x11", "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8",
			 "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24"
		);

	#else
		/*
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
			 "sub x3, x2, #4\n\t"					//r*col+c-1
			 "fmul v7.4s, v6.4s, v2.4s\n\t"			//v7 acumulador
			 "ldr q8, [%[temp], x3]\n\t"			//v8 auxiliar, temp[r*col+c-1]
			 "add x3, x2, #4 \n\t"					//r*col+c+1
			 "ldr q6, [%[temp], x3]\n\t"			//v6 auxiliar, temp[r*col+c+1]
			 "add x3, x2, %[col], LSL #2\n\t"		//(r+1)*col+c
			 "fadd v6.4s, v6.4s, v8.4s\n\t"			//v6 auxiliar, temp[r*col+c+1]+temp[r*col+c-1]
			 "fmls v6.4s, v5.4s, v9.4s\n\t"			//v6 auxiliar, (temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c])
			 "fmla v7.4s, v6.4s, v0.4s\n\t"			//v7 acumulador 
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
		*/
		
		
		asm volatile (
			 "lsl x1, %[c], #2 \n\t"				//c=c_start
			 "lsl x2, %[r], #2 \n\t"

			 "ld1r { v4.4s } , [%[ca]]\n\t"
			 "mul x2, x2, %[col]\n\t"				//r*col
			 "fmov v9.4s , #2\n\t"
			 "sub x3, x2, #4\n\t"					//r*col-1
			 "add x4, %[temp], x2\n\t"				//x4, *temp[r*col]
			 "add x5, %[pow], x2\n\t"				//x5, *power[r*col]
			 "add x6, %[temp], x3\n\t"				//x6, *temp[r*col-1]
			 "add x3, x2, #4\n\t"					//r*col+1
			 "ld1r { v1.4s } , [%[Ry]]\n\t"
			 "add x7, %[temp], x3\n\t"				//x7, *temp[r*col+1]
			 "add x3, x2, %[col], LSL #2\n\t"		//(r+1)*col
			 "ld1r { v0.4s } , [%[Rx]]\n\t"
			 "add x9, %[temp], x3\n\t"				//x9, *temp[(r+1)*col]
			 "sub x3, x2, %[col], LSL #2\n\t"		//(r-1)*col
			 "ld1r { v2.4s } , [%[Rz]]\n\t"
			 "ld1r { v3.4s } , [%[amb]]\n\t"
			 "add x10, %[temp], x3\n\t"				//x10,*temp[(r-1)*col]
			 "add x11, %[res], x2\n\t"				//x11,*result[r*col]
			 
			
			".loop_neon:\n\t"
			 "ldr q5, [x4, x1]\n\t"					//temp[r*col+c]
			 "ldr q8, [x6, x1]\n\t"					//v8 auxiliar, temp[r*col+c-1]
			 "fsub v6.4s, v3.4s, v5.4s\n\t"			//v6 auxiliar, (amb_temp - temp[r*col+c])
			 "ldr q10, [x7, x1]\n\t"				//v10 auxiliar, temp[r*col+c+1]
			 "fmul v7.4s, v6.4s, v2.4s\n\t"			//v7 acumulador
			 "fadd v6.4s, v10.4s, v8.4s\n\t"		//v6 auxiliar, temp[r*col+c+1]+temp[r*col+c-1]
			 "ldr q8, [x10, x1]\n\t"				//v8 auxiliar, temp[(r-1)*col+c]
			 "fmls v6.4s, v5.4s, v9.4s\n\t"			//v6 auxiliar, (temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c])
			 "fmla v7.4s, v6.4s, v0.4s\n\t"			//v7 acumulador 
			 "ldr q6, [x9, x1]\n\t"					//v6 auxiliar, temp[(r+1)*col+c]
			 "fadd v6.4s, v6.4s, v8.4s\n\t"			//v6 auxiliar, temp[(r+1)*col+c]+temp[(r-1)*col+c]
			 "ldr q10, [x5, x1]\n\t"				//v10 auxiliar, power[r*col+c]
			 "fmls v6.4s, v5.4s, v9.4s\n\t"			//v6 auxiliar, (temp[(r+1)*col+c]+temp[(r-1)*col+c] - 2.f*temp[r*col+c])
			 "fmla v7.4s, v6.4s, v1.4s\n\t"			//v7 acumulador
			 "fadd v8.4s, v10.4s, v7.4s\n\t"		//v8 auxiliar, acumulador(v7)+power[r+*col+c]
			 "fmla v5.4s, v8.4s, v4.4s\n\t"			//result[r*col+c]
			 "str q5, [x11, x1]\n\t"
			 "add x1, x1, #16\n\t"					//iterador+4
			 "cmp x1, %[sz]\n\t"
			 "b.ne .loop_neon\n\t"
			
			 : [res] "+r" (result)
			 : [c] "r" (c_start), [Rx] "r" (&Rx_1), [Ry] "r" (&Ry_1), [Rz] "r" (&Rz_1), [amb] "r" (&amb_temp), [ca] "r" (&Cap_1), [temp] "r" (temp),
			 [pow] "r" (power), [r] "r" (r), [col] "r" (col), [sz] "r" (iter*4)
			 : "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x9", "x10", "x11", "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10"
		);
		
		
	#endif
	
    rem = (size+c_start) % (NEON_STRIDE*unroll);
    
    for ( int c = iter; c < rem + iter; ++c ) 
    {
        result[r*col+c] =temp[r*col+c]+ ( Cap_1 * (power[r*col+c] + 
            (temp[(r+1)*col+c] + temp[(r-1)*col+c] - 2.f*temp[r*col+c]) * Ry_1 + 
            (temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c]) * Rx_1 + 
            (amb_temp - temp[r*col+c]) * Rz_1));
			
    }
	
	/*CHECK VALUES */
	/*
	for ( int c = c_start; c < size+c_start; ++c ) 
	{
		
		float teste1 =temp[r*col+c]+ ( Cap_1 * (power[r*col+c] + 
			(temp[(r+1)*col+c] + temp[(r-1)*col+c] - 2.f*temp[r*col+c]) * Ry_1 + 
			(temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c]) * Rx_1 + 
			(amb_temp - temp[r*col+c]) * Rz_1));
		
		if (teste1!=result[r*col+c])
		{
			printf("ERROR\n", teste1);
			printf("LOOP: r*col+c: %d\n", r*col+c);
			//printf("%f, %f\n", temp[(r+1)*col+c], teste[c-c_start]);
			printf("normal: %f, new: %f\n", teste1, result[r*col+c]);
		}
		
	}
	*/
	//free(teste);

#elif defined(SVE)
	
	asm volatile (
		 "mov x1, %[c] \n\t"								//iterador c=c_start
		 "whilelt p0.s, x1, %[sz]\n\t"
		 "ld1rw {z0.s}, p0/z, %[Rx]\n\t"
		 "ld1rw {z1.s}, p0/z, %[Ry]\n\t"
		 "ld1rw {z2.s}, p0/z, %[Rz]\n\t"
		 "ld1rw {z3.s}, p0/z, %[amb]\n\t"
		 "ld1rw {z4.s}, p0/z, %[ca]\n\t"
		 
		 "fmov z9.s ,p0/m, #2\n\t"
		 "madd x2, %[r], %[col], x1\n\t"					//(r*col+c)
				 
		 ".loop_sve:\n\t"
		 "ld1w { z5.s }, p0/z, [%[temp], x2, lsl #2]\n\t"	//z5, temp[r*col+c]
		 "mov z6.s, p0/m, z3.s\n\t"							//auxiliar z6
		 "fsub z6.s, p0/m, z6.s, z5.s\n\t"					//z6, (amb_temp - temp[r*col+c])
		 "fmul z6.s, p0/m, z6.s, z2.s\n\t"					//z6, (amb_temp - temp[r*col+c])*Rz_1
		 "sub x3, x2, #1\n\t"								//r*col+c-1
		 "ld1w { z7.s }, p0/z, [%[temp], x3, lsl #2]\n\t"	//z7, temp[r*col+c-1]
		 "add x3, x2, #1 \n\t"								//r*col+c+1
		 "ld1w { z8.s }, p0/z, [%[temp], x3, lsl #2]\n\t"	//z8, temp[r*col+c+1]
		 "fadd z7.s, p0/m, z7.s, z8.s\n\t"					//z7, temp[r*col+c+1]+temp[r*col+c-1]
		 "fmls z7.s, p0/m, z9.s, z5.s\n\t"					//z7, (temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c])
		 "fmla z6.s, p0/m, z7.s, z0.s\n\t"					//z6 acumulador 
		 "add x3, x2, %[col]\n\t"							//(r+1)*col+c
		 "ld1w { z7.s }, p0/z, [%[temp], x3, lsl #2]\n\t"	//z7,  temp[(r+1)*col+c]
		 "sub x3, x2, %[col]\n\t"							//(r-1)*col+c
		 "ld1w { z8.s }, p0/z, [%[temp], x3, lsl #2]\n\t"	//z8,  temp[(r-1)*col+c]
		 "fadd z7.s, p0/m, z7.s, z8.s\n\t"					//z7, temp[(r+1)*col+c]+temp[(r-1)*col+c]
		 "fmls z7.s, p0/m, z9.s, z5.s\n\t"					//z7, (temp[(r+1)*col+c]+temp[(r-1)*col+c] - 2.f*temp[r*col+c])
		 "fmla z6.s, p0/m, z7.s, z1.s\n\t"					//z6 acumulador
		 "ld1w { z8.s }, p0/z, [%[pow], x2, lsl #2]\n\t"	//z8, power[r*col+c]
		 "fadd z8.s, p0/m, z8.s, z6.s\n\t"					//z8, acumulador(z6)+power[r+*col+c]
		 "fmla z5.s, p0/m, z8.s, z4.s\n\t"					//z6 acumulador
		 "st1w z5.s, p0, [%[res], x2, lsl #2]\n\t"
		 "incw x2\n\t"
		 "incw x1\n\t"
		 "whilelt p0.s, x1, %[sz]\n\t"
		 "b.first .loop_sve\n\t"
		 
		 : [res] "+r" (result)
		 : [c] "r" (c_start), [Rx] "m" (Rx_1), [Ry] "m" (Ry_1), [Rz] "m" (Rz_1), [amb] "m" (amb_temp), [ca] "m" (Cap_1), [temp] "r" (temp),
		 [pow] "r" (power), [r] "r" (r), [col] "r" (col), [sz] "r" (c_start+size)
		 : "x1", "x2", "x3", "memory", "p0", "z0", "z1", "z2", "z3", "z4", "z5", "z6", "z7", "z8", "z9"
	);	
	
	/*CHECK VALUES */
	/*
	for ( int c = c_start; c < size+c_start; ++c ) 
	{
		
		float teste1 =temp[r*col+c]+ ( Cap_1 * (power[r*col+c] + 
			(temp[(r+1)*col+c] + temp[(r-1)*col+c] - 2.f*temp[r*col+c]) * Ry_1 + 
			(temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c]) * Rx_1 + 
			(amb_temp - temp[r*col+c]) * Rz_1));
		
		if (teste1!=result[r*col+c])
		{
			printf("ERROR\n");
			printf("LOOP: r*col+c: %d\n", r*col+c);
			//printf("%f, %f\n", temp[(r+1)*col+c], teste[c-c_start]);
			printf("normal: %f, new: %f\n", teste1, result[r*col+c]);
		}
		
	}
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

void volatile kernel_ifs(float *result, float *temp, float *power, size_t c_start, size_t size, size_t col, size_t r, size_t row,
					  float Cap_1, float Rx_1, float Ry_1, float Rz_1, float amb_temp, float *delta)
{
	
#if defined(SVE)
	
	asm volatile (
	
		 //if (r==0)
		 "cmp %[r], #0\n\t"
		 "b.eq .sve_r_0\n\t"
		 
		 
		 //if (r==row-1)
		 "sub x1, %[row], #1\n\t"
		 "cmp %[r], x1\n\t"
		 "b.eq .sve_r_end\n\t"
		 
		 //if (c ==0)
		 "mov x1, %[c]\n\t" 								//c=c_start caso c!=0
		 "cmp %[c], #0\n\t"
		 "b.ne .sve_normal\n\t"
		 
		 //c =0
		 "lsl x1, %[r], #2\n\t"								//r
		 "mul x1, x1, %[col]\n\t"							//r*col
		 "ldr s1, %[amb]\n\t"								//amb_temp
		 "ldr s5, [%[temp], x1]\n\t"						//temp[r*col]
		 "ldr s2, %[Rx]\n\t"								//Rx_1
		 "ldr s3, %[Ry]\n\t"								//Ry_1
		 "ldr s4, %[Rz]\n\t"								//Rz_1
		 "fsub s1, s1, s5\n\t"								//amb_temp - temp[r*col]
		 "add x2, x1, #4\n\t"								///r*col+1
		 "fmul s1, s4, s1\n\t"
		 "ldr s6, [%[temp], x2]\n\t"						//temp[r*col+1]
		 "fsub s6, s6, s5\n\t"								//temp[r*col+1] - temp[r*col]
		 "ldr s4, [%[pow], x1]\n\t"							//power[r*col]
		 "fmadd s1, s6, s2, s1\n\t"							//acumulador
		 "add x2, x1, %[col], lsl #2\n\t"					//(r+1)*col
		 "ldr s6, [%[temp], x2]\n\t"						//temp[(r+1)*col]
		 "sub x2, x1, %[col], lsl #2\n\t"					//(r-1)*col
		 "fmov s7, #2\n\t"
		 "ldr s2, [%[temp], x2]\n\t"						//temp[(r-1)*col]
		 "ldr s8, %[ca]\n\t"								//cap_1
		 "fadd s6, s6, s2\n\t"								//temp[(r+1)*col]+ temp[(r-1)*col]
		 "fmsub s6, s7, s5, s6\n\t"							//temp[(r+1)*col] + temp[(r-1)*col] - 2.0*temp[r*col]
		 "fmadd s1, s6, s3, s1\n\t"							//acumulador
		 "fadd s1, s4, s1\n\t"								//acumulador+power[r*col]
		 "fmul s0, s1, s8\n\t"								//delta  
		 "str s0, [%[delta]]\n\t"
		 "fadd s1, s0, s5\n\t"								//result[r*col]
		 "str s1, [%[res], x1]\n\t"
		 "mov x1, #1\n\t"									//c=1
		
		
		 ".sve_normal:\n\t"
		 //x1 iterador c=c_start || c=1
		 "whilelt p0.s, x1, %[sz]\n\t"
		 "madd x2, %[r], %[col], x1\n\t"					//(r*col+c)
		 "ld1rw {z2.s}, p0/z, [%[delta]]\n\t"				//z2, delta
		 "lastb s0, p0, z2.s\n\t"							//s0 delta, save last delta
		 //loop
		 ".loop_sve_normal:\n\t"
		 "ld1w { z1.s }, p0/z, [%[temp], x2, lsl #2]\n\t"	//z1, temp[r*col+c]
		 "fadd z1.s, p0/m, z1.s, z2.s\n\t"					//temp[r*col+c]+delta
		 "st1w z1.s, p0, [%[res], x2, lsl #2]\n\t"
		 "incw x2\n\t"
		 "incw x1\n\t"
		 "whilelt p0.s, x1, %[sz]\n\t"
		 "b.first .loop_sve_normal\n\t"
		 "sub x2, %[col], #1\n\t"
		 "cmp x1, x2\n\t"
		 "b.ne .sve_end\n\t"
		 
		 //c=col-1
		 "lsl x1, x1, #2\n\t"								//c
		 "lsl x2, %[r], #2 \n\t"							//r
		 "madd x2, x2, %[col], x1\n\t"						//r*col+c
		 "ldr s1, %[amb]\n\t"								//amb_temp
		 "ldr s5, [%[temp], x2]\n\t"						//temp[r*col+c]
		 "ldr s2, %[Rx]\n\t"								//Rx_1
		 "ldr s3, %[Ry]\n\t"								//Ry_1
		 "ldr s4, %[Rz]\n\t"								//Rz_1
		 "fsub s1, s1, s5\n\t"								//amb_temp - temp[r*col+c]
		 "sub x3, x2, #4\n\t"								///r*col+c-1
		 "fmul s1, s4, s1\n\t"
		 "ldr s6, [%[temp], x3]\n\t"						//temp[r*col+c-1]
		 "fsub s6, s6, s5\n\t"								//temp[r*col+c-1] - temp[r*col+c]
		 "ldr s4, [%[pow], x2]\n\t"							//power[r*col]
		 "fmadd s1, s6, s2, s1\n\t"							//acumulador
		 "add x3, x2, %[col], lsl #2\n\t"					//(r+1)*col+c
		 "ldr s6, [%[temp], x3]\n\t"						//temp[(r+1)*col+c]
		 "sub x3, x2, %[col], lsl #2\n\t"					//(r-1)*col+c
		 "fmov s7, #2\n\t"
		 "ldr s2, [%[temp], x3]\n\t"						//temp[(r-1)*col]
		 "ldr s8, %[ca]\n\t"								//cap_1
		 "fadd s6, s6, s2\n\t"								//temp[(r+1)*col+c] + temp[(r-1)*col+c]
		 "fmsub s6, s7, s5, s6\n\t"							//temp[(r+1)*col+c] + temp[(r-1)*col+c] - 2.0*temp[r*col+c]
		 "fmadd s1, s6, s3, s1\n\t"							//acumulador
		 "fadd s1, s4, s1\n\t"								//acumulador+power[r*col+c]
		 "fmul s0, s1, s8\n\t"								//delta  
		 "fadd s1, s0, s5\n\t"								//result[r*col+c]
		 "str s1, [%[res], x2]\n\t"
		 "b .sve_end\n\t"									//COMFIRMAR NOME
		 
		 
		 //r=0
		 ".sve_r_0:\n\t"
		 "mov x1, %[c]\n\t"									//iterador c=c_start
		 "whilelt p0.s, x1, %[sz]\n\t"
		 "ld1rw {z10.s}, p0/z, %[Rx]\n\t"
		 "ld1rw {z1.s}, p0/z, %[Ry]\n\t"
		 "ld1rw {z2.s}, p0/z, %[Rz]\n\t"
		 "ld1rw {z3.s}, p0/z, %[amb]\n\t"
		 "ld1rw {z4.s}, p0/z, %[ca]\n\t"
		 "fmov z9.s ,p0/m, #2\n\t"
		 //loop
		 ".loop_sve_r_0:\n\t"
		 "ld1w { z5.s }, p0/z, [%[temp], x1, lsl #2]\n\t"	//z5, temp[c]
		 "mov z6.s, p0/m, z3.s\n\t"							//auxiliar z6
		 "fsub z6.s, p0/m, z6.s, z5.s\n\t"					//z6, (amb_temp - temp[c])
		 "add x2, x1, %[col]\n\t"							//col+c
		 "ld1w { z7.s }, p0/z, [%[temp], x2, lsl #2]\n\t"	//z7, temp[col+c]
		 "fmul z6.s, p0/m, z6.s, z2.s\n\t"					//z6, (amb_temp - temp[c])*Rz_1
		 "fsub z7.s, p0/m, z7.s, z5.s\n\t"					//z7, temp[col+c]-temp[c]
		 "fmla z6.s, p0/m, z7.s, z1.s\n\t"					//z6, acumulador
		 "add x2, x1, #1\n\t"								//c+1
		 "ld1w { z7.s }, p0/z, [%[temp], x2, lsl #2]\n\t"	//z7, temp[c+1]
		 "sub x2, x1, #1\n\t"								//c-1
		 "ld1w { z8.s }, p0/z, [%[temp], x2, lsl #2]\n\t"	//z8, temp[c-1]
		 "fadd z7.s, p0/m, z7.s, z8.s\n\t"					//z7, temp[c+1]+temp[c-1]
		 "fmls z7.s, p0/m, z9.s, z5.s\n\t"					//z7,(temp[c+1]+temp[c-1] - 2.0*temp[c])
		 "fmla z6.s, p0/m, z7.s, z10.s\n\t"					//z6 acumulador
		 "ld1w { z8.s }, p0/z, [%[pow], x1, lsl #2]\n\t"	//z8, power[c]
		 "fadd z8.s, p0/m, z8.s, z6.s\n\t"					//z8, acumulador(z6)+power[c]
		 "fmul z8.s, p0/m, z8.s, z4.s\n\t"					//delta
		 "lastb s0, p0, z8.s\n\t"							//s0 delta, save last delta
		 "fadd z5.s, p0/m, z5.s, z8.s\n\t"					//z5 acumulador
		 "st1w z5.s, p0, [%[res], x1, lsl #2]\n\t"
		 "incw x1\n\t"
		 "whilelt p0.s, x1, %[sz]\n\t"
		 "b.first .loop_sve_r_0\n\t"
		 
		 
		//vê se é o CORNER
		
		 "sub x2, %[col], #1\n\t"							//x2=col-1
		 "cmp x1, x2\n\t"
		 "b.eq .sve_conerRU\n\t"
		 
		 "b .sve_end\n\t"
		 
		 // r=0 && c=col-1
		 ".sve_conerRU:\n\t"
		 "lsl x1, x1, #2\n\t"								//col-1
		 "ldr s1, %[amb]\n\t"								//amb_temp
		 "ldr s5, [%[temp], x1]\n\t"						//temp[col-1]
		 "ldr s2, %[Rx]\n\t"								//Rx_1
		 "ldr s3, %[Ry]\n\t"								//Ry_1
		 "ldr s4, %[Rz]\n\t"								//Rz_1
		 "fsub s1, s1, s5\n\t"								//(amb_temp - temp[col-1])
		 "add x2, x1, %[col], lsl #2\n\t"					//col-1+col
		 "fmul s1, s4, s1\n\t"
		 "ldr s6, [%[temp], x2]\n\t"						//temp[col-1+col]
		 "fsub s6, s6, s5\n\t"								//temp[c+col] - temp[c]
		 "ldr s4, %[ca]\n\t"								//cap_1
		 "fmadd s1, s6, s3, s1\n\t"							//acumulador
		 "sub x2, x1, #4\n\t"								//col-1-1
		 "ldr s6, [%[temp], x2]\n\t"						//temp[col-1-1]
		 "ldr s3, [%[pow], x1]\n\t"							//power[col-1]
		 "fsub s6, s6, s5\n\t"								//temp[col-1-1]- temp[col-1]
		 "fmadd s1, s6, s2, s1\n\t"							//acumulador
		 "fadd s1, s3, s1\n\t"								//acumulador+power[col-1]
		 "fmul s0, s1, s4\n\t"								//delta
		 "fadd s1, s0, s5\n\t"								//result[col-1]
		 "str s1, [%[res], x1]\n\t"
		 "b .sve_end\n\t"									//COMFIRMAR NOME
		 
		 
		 // r = row-1
		 ".sve_r_end:\n\t"	  
		 "mov x1, %[c] \n\t"								//iterador c=c_start
		 "madd x2, %[r], %[col], %[c]\n\t"					//r*col+c
		 "whilelt p0.s, x1, %[sz]\n\t"
		 "ld1rw {z10.s}, p0/z, %[Rx]\n\t"
		 "ld1rw {z1.s}, p0/z, %[Ry]\n\t"
		 "ld1rw {z2.s}, p0/z, %[Rz]\n\t"
		 "ld1rw {z3.s}, p0/z, %[amb]\n\t"
		 "ld1rw {z4.s}, p0/z, %[ca]\n\t"
		 "fmov z9.s ,p0/m, #2\n\t"
		 //loop
		 ".loop_sve_r_end:\n\t"
		 "ld1w { z5.s }, p0/z, [%[temp], x2, lsl #2]\n\t"	//z5, temp[r*col+c]
		 "mov z6.s, p0/m, z3.s\n\t"							//auxiliar z6
		 "fsub z6.s, p0/m, z6.s, z5.s\n\t"					//z6, (amb_temp - temp[r*col+c])
		 "sub x3, x2, %[col]\n\t"							//(r-1)*col+c
		 "ld1w { z7.s }, p0/z, [%[temp], x3, lsl #2]\n\t"	//z7, temp[(r-1)*col+c]
		 "fmul z6.s, p0/m, z6.s, z2.s\n\t"					//z6, (amb_temp - temp[r*col+c])*Rz_1
		 "fsub z7.s, p0/m, z7.s, z5.s\n\t"					//z7, temp[(r-1)*col+c]-temp[r*col+c]
		 "fmla z6.s, p0/m, z7.s, z1.s\n\t"					//z6, acumulador 
		 "add x3, x2, #1\n\t"								//r*col+c+1
		 "ld1w { z7.s }, p0/z, [%[temp], x3, lsl #2]\n\t"	//z7,  temp[r*col+c+1]
		 "sub x3, x2, #1\n\t"								//r*col+c-1
		 "ld1w { z8.s }, p0/z, [%[temp], x3, lsl #2]\n\t"	//z8, temp[r*col+c-1]
		 "fadd z7.s, p0/m, z7.s, z8.s\n\t"					//z7, temp[r*col+c+1]+temp[r*col+c-1]
		 "fmls z7.s, p0/m, z9.s, z5.s\n\t"					//z7, temp[r*col+c+1]+temp[r*col+c-1] - 2.0*temp[r*col+c]
		 "fmla z6.s, p0/m, z7.s, z10.s\n\t"					//z6 acumulador
		 "ld1w { z8.s }, p0/z, [%[pow], x2, lsl #2]\n\t"	//z8, power[r*col+c]
		 "fadd z8.s, p0/m, z8.s, z6.s\n\t"					//z8, acumulador(z6)+power[r*col+c]
		 "fmul z8.s, p0/m, z8.s, z4.s\n\t"					//delta
		 "lastb s0, p0, z8.s\n\t"							//s0 delta, save last delta
		 "fadd z5.s, p0/m, z5.s, z8.s\n\t"					//z6 acumulador
		 "st1w z5.s, p0, [%[res], x2, lsl #2]\n\t"
		 "incw x2\n\t"
		 "incw x1\n\t"
		 "whilelt p0.s, x1, %[sz]\n\t"
		 "b.first .loop_sve_r_end\n\t"
		
		
		 ".sve_end:\n\t"
		 "str s0, [%[delta]]\n\t"
		 
		 : [res] "+r" (result), [delta] "+r" (delta)
		 : [c] "r" (c_start), [Rx] "m" (Rx_1), [Ry] "m" (Ry_1), [Rz] "m" (Rz_1), [amb] "m" (amb_temp), [ca] "m" (Cap_1), [temp] "r" (temp),
		 [pow] "r" (power), [r] "r" (r), [col] "r" (col), [row] "r" (row), [sz] "r" (c_start+size)
		 : "x1", "x2", "x3", "memory", "p0", "z1", "z2", "z3", "z4", "z5", "z6", "z7", "z8", "z9", "z10", "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8"
	);
	

#else
	
	int c;
	for ( c = c_start; c < c_start + size; ++c ) 
	{
		if ((r == 0) && (c == col-1)) {
			delta[0] = (Cap_1) * (power[c] +
				(temp[c-1] - temp[c]) * Rx_1 +
				(temp[c+col] - temp[c]) * Ry_1 +
				(amb_temp - temp[c]) * Rz_1);
        }
		else if (r == 0) {
			delta[0] = (Cap_1) * (power[c] + 
				(temp[c+1] + temp[c-1] - 2.0*temp[c]) * Rx_1 + 
				(temp[col+c] - temp[c]) * Ry_1 + 
				(amb_temp - temp[c]) * Rz_1);
		}
		else if (c == col-1) {
			delta[0] = (Cap_1) * (power[r*col+c] + 
				(temp[(r+1)*col+c] + temp[(r-1)*col+c] - 2.0*temp[r*col+c]) * Ry_1 + 
				(temp[r*col+c-1] - temp[r*col+c]) * Rx_1 + 
				(amb_temp - temp[r*col+c]) * Rz_1);
		}	
		else if (r == row-1) {
			delta[0] = (Cap_1) * (power[r*col+c] + 
				(temp[r*col+c+1] + temp[r*col+c-1] - 2.0*temp[r*col+c]) * Rx_1 + 
				(temp[(r-1)*col+c] - temp[r*col+c]) * Ry_1 + 
				(amb_temp - temp[r*col+c]) * Rz_1);
		}	
		else if (c == 0) {
			delta[0] = (Cap_1) * (power[r*col] + 
				(temp[(r+1)*col] + temp[(r-1)*col] - 2.0*temp[r*col]) * Ry_1 + 
				(temp[r*col+1] - temp[r*col]) * Rx_1 + 
				(amb_temp - temp[r*col]) * Rz_1);
		}
		result[r*col+c] =temp[r*col+c]+ delta[0];
	}
	
	
#endif

	
}					  
						  
