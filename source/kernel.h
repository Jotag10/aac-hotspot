#ifndef KERNEL_AAC_HEADER__
#define KERNEL_AAC_HEADER__

#include <stddef.h>


void volatile kernel(float *result, float *temp, float *power, size_t c_start, size_t size, size_t col, size_t r,
					  float Cap_1, float Rx_1, float Ry_1, float Rz_1, float amb_temp);
					  
void volatile kernel_ifs(float *result, float *temp, float *power, size_t c_start, size_t size, size_t col, size_t r, size_t row,
					  float Cap_1, float Rx_1, float Ry_1, float Rz_1, float amb_temp, float *d);

#endif //KERNEL_AAC_HEADER__