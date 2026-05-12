#include <types.h>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <stdlib.h>
#include <stdio.h>



__global__ void sinkhornStep1(double* u, double* v, double* a, double* b, u32 len, u32 width, u32 height, double reg){
	int index = blockIdx.x * blockDim.x + threadIdx.x;
	int stride = blockDim.x * gridDim.x;
	for(int i=index; i<len; i+=stride){
		double val = 0.0;
		for(int j=0; j<len; j++){
			double p0_x = (double)(i % width) / (double)width;
			double p0_y = (double)(i / width) / (double)width;
			
			double p1_x = (double)(j % width) / (double)width;
			double p1_y = (double)(j / width) / (double)width;
			//probably less efficient this way but like. I want to be able to read my code man.
			double cost = (p0_x - p1_x)*(p0_x - p1_x) + (p0_y - p1_y)*(p0_y - p1_y);
			
			//double cost = ((i%width)/width - (j%width)/width) * ((i%width)/width - (j%width)/width) + ((i/width)/width - (j/width)/width) * ((i/width)/width - (j/width)/width);
			cost = cost * (double)width;
			val += exp(-1 * cost / reg) * u[j];
		}
		if(val > 1e-7){
			v[i] = b[i] / val;
		}
	}
}

__global__ void sinkhornStep2(double* u, double* v, double* a, double* b, u32 len, u32 width, u32 height, double reg){
	int index = blockIdx.x * blockDim.x + threadIdx.x;
	int stride = blockDim.x * gridDim.x;
	for(int i=index; i<len; i+=stride){
		double val = 0.0;
		for(int j=0; j<len; j++){ //since the cost is just distance, it should be symmetric, thus C^T=C
			double p0_x = (double)(i % width) / (double)width;
			double p0_y = (double)(i / width) / (double)width;
			
			double p1_x = (double)(j % width) / (double)width;
			double p1_y = (double)(j / width) / (double)width;

			double cost = (p0_x - p1_x)*(p0_x - p1_x) + (p0_y - p1_y)*(p0_y - p1_y);
			
			//double cost = ((i%width)/width - (j%width)/width) * ((i%width)/width - (j%width)/width) + ((i/width)/width - (j/width)/width) * ((i/width)/width - (j/width)/width);
			cost = cost * (double)width;
			val += exp(-1 * cost / reg) * v[j];
		}
		if(val > 1e-7){
			u[i] = a[i] / val;
		}
	}

}


__global__ void nothing(){
	printf("Hello from thread %d\n", threadIdx.x);
	return;
}

extern "C" void callSinkhornStep1(double* u, double* v, double* a, double* b, u32 len, u32 width, u32 height, double reg){
	int blockSize = 256;
	int numBlocks = (len + blockSize-1) / blockSize;
	sinkhornStep1<<<numBlocks,blockSize>>>(u, v, a, b, len, width, height, reg);
    cudaDeviceSynchronize();
}

extern "C" void callSinkhornStep2(double* u, double* v, double* a, double* b, u32 len, u32 width, u32 height, double reg){
	int blockSize = 256;
	int numBlocks = (len + blockSize-1) / blockSize;
	sinkhornStep2<<<numBlocks,blockSize>>>(u, v, a, b, len, width, height, reg);
    cudaDeviceSynchronize();
}




extern "C" void cu_createArr(void** ptr, u32 len){
	cudaMallocManaged(ptr, len);
}
