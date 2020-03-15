#include "kernel.h"


/*
#if defined(SVE)
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

#if defined(NEON)

    #define NEON_STRIDE 4
    size_t iter = 0, rem = 0;
    if(size < NEON_STRIDE){
        for(int i = 0; i < size; i++){
            y[i] = A*x[i]+y[i];
        }
        return;
    }

    iter = size / NEON_STRIDE * NEON_STRIDE;
    
     asm volatile (
         // "ldr s0, [x0]\n\t"
         "mov x1, #0\n\t"
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
    );

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

    
    // asm volatile (
    //     // "ldr s0, [x0]\n\t"
    //     "mov x1, #0\n\t"                                      x0=i=0
    //     "mov x2, %[x]\n\t"                                    x2=x
    //     "mov x3, %[y]\n\t"                                    x3=y
    //     "add x4, x2, %[sz]\n\t"                               x4=x+iter*4 
    //     "ld1r { v0.4s } , [%[a]]\n\t"                         guarda A nas 4 posiçoes do registo
    //     ".loop_neon:\n\t"                                     
    //     "ld1 { v1.4s, v2.4s }, [x2], #32\n\t"            
    //     "ld1 { v3.4s, v4.4s }, [x3]\n\t"                      
    //     "fmla v3.4s, v1.4s, v0.4s\n\t"
    //     "fmla v4.4s, v2.4s, v0.4s\n\t"
    //     "st1 { v3.4s, v4.4s }, [x3], #32 \n\t"
    //     "cmp x2, x4\n\t"
    //     "b.lt .loop_neon\n\t"
    //     : [y] "+r" (y)
    //     : [sz] "r" (iter*4), [a] "r" (&A), [x] "r" (x)
    //     : "x1", "x2", "x3", "x4", "memory", "v0", "v1", "v2"
    // );

    //asm volatile (
    //    // "ldr s0, [x0]\n\t"
    //    "mov x1, #0\n\t"
    //    "mov x2, %[x]\n\t"
    //    "mov x3, %[y]\n\t"
    //    "add x4, x2, %[sz]\n\t"
    //    "ld1r { v0.4s } , [%[a]]\n\t"
    //    ".loop_neon:\n\t"
    //    "prfm PLDL1STRM, [x2, #64]\n\t"
    //    "prfm PLDL1STRM, [x3, #64]\n\t"
    //    "prfm PSTL1STRM, [x3, #64]\n\t"
    //    "ld1 { v1.4s, v2.4s, v3.4s, v4.4s }, [x2], #64\n\t"
    //    "ld1 { v5.4s, v6.4s, v7.4s, v8.4s }, [x3]\n\t"
    //    "fmla v5.4s, v1.4s, v0.4s\n\t"
    //    "fmla v6.4s, v2.4s, v0.4s\n\t"
    //    "fmla v7.4s, v3.4s, v0.4s\n\t"
    //    "fmla v8.4s, v4.4s, v0.4s\n\t"
    //    "st1 { v5.4s, v6.4s, v7.4s, v8.4s }, [x3], #64 \n\t"
    //    "cmp x2, x4\n\t"
    //    "b.lt .loop_neon\n\t"
    //    : [y] "+r" (y)
    //    : [sz] "r" (iter*4), [a] "r" (&A), [x] "r" (x)
    //    : "x1", "x2", "x3", "x4", "memory", "v0", "v1", "v2"
    //);

    

    rem = size % NEON_STRIDE;
    for(int i = iter; i<rem+iter; i++){
        y[i] = A*x[i]+y[i];
    }

#else
*/

void volatile kernel(FLOAT *result, FLOAT *temp, FLOAT *power, int c_start, int size, int col, int r,
					  FLOAT Cap_1, FLOAT Rx_1, FLOAT Ry_1, FLOAT Rz_1, FLOAT amb_temp)
{
    int c;
    
    for ( c = c_start; c < c_start + size; ++c ) 
    {
        /* Update Temperatures */
        result[r*col+c] =temp[r*col+c]+ ( Cap_1 * (power[r*col+c] + 
            (temp[(r+1)*col+c] + temp[(r-1)*col+c] - 2.f*temp[r*col+c]) * Ry_1 + 
            (temp[r*col+c+1] + temp[r*col+c-1] - 2.f*temp[r*col+c]) * Rx_1 + 
            (amb_temp - temp[r*col+c]) * Rz_1));
    }
}
