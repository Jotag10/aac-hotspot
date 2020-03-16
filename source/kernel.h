#ifndef KERNEL_AAC_HEADER__
#define KERNEL_AAC_HEADER__

#include <stddef.h>

typedef float FLOAT;

void volatile kernel(float *result, FLOAT *temp, FLOAT *power, int c_start, int size, int col, int r,
					  FLOAT Cap_1, FLOAT Rx_1, FLOAT Ry_1, FLOAT Rz_1, FLOAT amb_temp);


#endif //KERNEL_AAC_HEADER__