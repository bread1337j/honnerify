#include <types.h>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

__global__ void sinkhornStep1(double* v, double* u, u32 len, u32 width, u32 height, u32 i, double reg){
	double out = 0;
	for(int j=0; j<len; j++){
		double cost = (i%width - j%width) * (i%width - j%width) + (i/width - j/width) * (i/width - j/width);
	}
}
__global__ void nothing(){
	return;
}

extern "C" void callSinkhornStep1(double* v, double* u, u32 len, u32 width, u32 height, u32 i, double reg){
	//sinkhornStep1<<<1, 1>>>(v, u, len, width, height, i, reg);
	nothing<<<1,1>>>();
    
    cudaDeviceSynchronize();
}
