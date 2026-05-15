#ifndef MULTIPROCESSING_H 
#define MULTIPROCESSING_H

#include <types.h>

void callSinkhornStep1(double* v, double* u, double* a, double* b, u32 len, u32 width, u32 height, double reg);
void callSinkhornStep2(double* u, double* v, double* a, double* b, u32 len, u32 width, u32 height, double reg);
void cu_createArr(void** ptr, u32 len);

void cu_naiveImageCreation(double* u, double* v, double* a, double* b, u8* supply, u8* demand, double reg, double mult, u32 len, u32 width, u8 bytesPerPixel);

#endif
