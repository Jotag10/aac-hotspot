#ifndef KERNEL_AAC_HEADER__
#define KERNEL_AAC_HEADER__

#include <stddef.h>


void volatile kernel(float *result, float *temp, float *power, int c_start, int size, int col, int r,
					  float Cap_1, float Rx_1, float Ry_1, float Rz_1, float amb_temp);


#endif //KERNEL_AAC_HEADER__