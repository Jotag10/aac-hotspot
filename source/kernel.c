#include "kernel.h"
#include <cstdio>
#include <stdlib.h>
void volatile kernel(float *result, float *temp, float *power, size_t c_start, size_t size, size_t col, size_t r,
					  float Cap_1, float Rx_1, float Ry_1, float Rz_1, float amb_temp)
{

#if defined(NEON)

    #define NEON_STRIDE 4
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
	float *teste = (float *) calloc (4, sizeof(float));
	int val=0;
    iter = (size+c_start) / NEON_STRIDE * NEON_STRIDE;
     asm volatile (
         
		 "mov x1, %[c]\n\t"				//iterador c=c_start
		 //"lsr x2, %[r], #2 \n\t"
		 "ld1r { v0.4s } , [%[Rx]]\n\t"
		 "ld1r { v1.4s } , [%[Ry]]\n\t"
		 "ld1r { v2.4s } , [%[Rz]]\n\t"
		 "ld1r { v3.4s } , [%[amb]]\n\t"
		 "ld1r { v4.4s } , [%[ca]]\n\t"
		 "fmov v9.4s , #2\n\t"
		 "mul x2, %[r], %[col]\n\t"				//r*col
		 "mov x5, #4\n\t"
		 "mul x2, x2, x5\n\t"					//r*col*4
		 "mul x1, x1, x5\n\t"					//c*4
		 "add x2, x2, x1\n\t"					//(r*col+c)
		 "mov x4, #0\n\t"
		 
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
		 : "x1", "x2", "x3","x5", "memory", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9"
    );
	
	for ( int c = c_start; c < iter; ++c ) 
	{
		float cona =temp[r*col+c]+ ( Cap_1 * (power[r*col+c] + 
			(temp[(r+1)*col+c] + temp[(r-1)*col+c] - 2.f*temp[r*col+c]) * Ry_1 + 
			(temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c]) * Rx_1 + 
			(amb_temp - temp[r*col+c]) * Rz_1));
		
		printf("normal: %f\n", cona);
	}

	//printf ("c: %d, iter: %d\n",val, iter*4);
	/*
	for (size_t c = c_start; c < c_start+4; ++c ) 
	{
		float teste1 =temp[r*col+c]+ ( Cap_1 * (power[r*col+c] + 
				(temp[(r+1)*col+c] + temp[(r-1)*col+c] - 2.f*temp[r*col+c]) * Ry_1 + 
                (temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c]) * Rx_1 + 
                (amb_temp - temp[r*col+c]) * Rz_1));
		printf("normal: %f, new: %f\n", teste1, teste[c-c_start]); 
		
	}
	printf ("\n\n");
	
	
	
	//DUVIDAS
	// "memory"- diz que é usado quando se faz leituras ou escritas para itens nao listados como inputs ou outputs, no exemplo nao ha esses casos.
	// %[]
	// ifs, como resolver loads nao paralelos
	//"sub x3, x2, %[col]\n\t", nao reconhece o registo, diz que esta a usar como valor imediato
	
	/*
	
    //exemplo
	void SAXPY(float * x, float * y, float A, size_t size)
	{
		for(size_t i = 0; i < size; i++){
			y[i] = A*x[i] + y[i];
		}
	}
		 "mov x1, #0 \n\t"
         "ld1r { v0.4s } , [%[a]]\n\t"
         ".loop_neon:\n\t"
         "ldr q1, [%[x], x1]\n\t"
         "ldr q2, [%[y], x1]\n\t"
         "fmla v2.4s, v1.4s, v0.4s\n\t"
         "str q2, [%[y], x1]\n\t"
         "add x1, x1, #16\n\t"
         "cmp x1, %[sz]\n\t"
         "b.lt .loop_neon\n\t"
         : [y] "+r" (y)
         : [sz] "r" (iter*4), [a] "r" (&A), [x] "r" (x)
         : "x1", "x2", "x3", "memory", "v0", "v1", "v2"
		*/
    //NEON V2
    // asm volatile (
    //     // "ldr s0, [x0]\n\t"
    //     "mov x1, #0\n\t"
    //     "mov x2, %[x]\n\t"
    //     "mov x3, %[y]\n\t"
    //     "add x4, x2, %[sz]\n\t"
    //     "ld1r { v0.4s } , [%[a]]\n\t"
    //     ".loop_neon:\n\t"
    //     "ld1 { v1.4s }, [x2], #16\n\t"
    //     "ld1 { v2.4s }, [x3]\n\t"
    //     "fmla v2.4s, v1.4s, v0.4s\n\t"
    //     "st1 { v2.4s }, [x3], #16 \n\t"
    //     "cmp x2, x4\n\t"
    //     "b.lt .loop_neon\n\t"
    //     : [y] "+r" (y)
    //     : [sz] "r" (iter*4), [a] "r" (&A), [x] "r" (x)
    //     : "x1", "x2", "x3", "x4", "memory", "v0", "v1", "v2"
    // );
    
	
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
