#include <types.h>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

__global__ void sinkhornStep1(double* v, double* u, double* a, double* b, u32 len, u32 width, u32 height, double reg){
	int index = blockIdx.x * blockDim.x + threadIdx.x;
	int stride = blockIdx.x * gridDim.x;
	for(int i=index; i<len; i+=stride){
		double val = 0.0;
		for(int j=0; j<len; j++){
			double cost = (i%width - j%width) * (i%width - j%width) + (i/width - j/width) * (i/width - j/width);
			val += exp(-1 * cost / reg) * u[j];
		}
		if(val > 1e-7){
			v[i] = a[i] / b[i];
		}
		
	}
}
__global__ void nothing(){
	printf("Hello from thread %d\n", threadIdx.y);
	return;
}

extern "C" void callSinkhornStep1(double* v, double* u, double* a, double* b, u32 len, u32 width, u32 height, double reg){
	//sinkhornStep1<<<1, 1>>>(v, u, len, width, height, i, reg);
	nothing<<<1,1>>>();
    
    cudaDeviceSynchronize();
}
