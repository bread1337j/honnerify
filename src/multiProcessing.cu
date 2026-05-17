#include <types.h>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <stdlib.h>
#include <stdio.h>



__global__ void sinkhornStep1(double* u, double* v, double* a, double* b, u32 len, u32 width, u32 height, double reg){
	//printf("%f\n", reg);
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
			cost *= (width);
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
			cost *= (width);
			val += exp(-1 * cost / reg) * v[j];
		}
		if(val > 1e-7){
			u[i] = a[i] / val;
		}
	}

}

__global__ void naiveImageCreation(double* u, double* v, double* a, double* b, u8* supply, u8* demand, double reg, double mult, u32 len, u32 width, u8 bytesPerPixel){
	//pretty much step 3 tbh

	int index = blockIdx.x * blockDim.x + threadIdx.x;
	int stride = blockDim.x * gridDim.x;
	for(int i=0; i<len; i++){
		u8* ptr0 = supply+(i*bytesPerPixel);
		for(int j=index; j<len; j+=stride){
			double p0_x = (double)(i % width) / (double)width;
			double p0_y = (double)(i / width) / (double)width;
			
			double p1_x = (double)(j % width) / (double)width;
			double p1_y = (double)(j / width) / (double)width;

			double cost = (p0_x - p1_x)*(p0_x - p1_x) + (p0_y - p1_y)*(p0_y - p1_y);
			
			cost *= (width);
			double val = exp(-1 * cost / reg) * v[j] * u[i] * mult;
			double* ptr1 = b+(j*bytesPerPixel);
			double* ptr2 = a+(i*bytesPerPixel);
			for(int k=0; k<bytesPerPixel; k++){
				double transport = (*(ptr0+k))*val;
				if(*(ptr2+k) < transport){
					transport = *(ptr2+k);
				}
				if(*(ptr1+k)+transport > 255){
					transport = 255 - *(ptr1+k);
				}
				*(ptr1+k) += transport;
				*(ptr2+k) -= transport;
				//some bs race condition happens here. 
				//its a bug not a feature trust (the final version will surely not use this algorithm to distribute the pixels)
			}

		}
	}
}

__global__ void zeroOneCopyAnother(double* zero, double* target, u8* src, u8 bytesPerPixel, u32 len){
	int index = blockIdx.x * blockDim.x + threadIdx.x;
	int stride = blockDim.x * gridDim.x;
	for(int i=index; i<len; i+=stride){
		for(int j=0; j<bytesPerPixel; j++){
			zero[i*bytesPerPixel+j] = 0;
			target[i*bytesPerPixel+j] = src[i*bytesPerPixel+j];
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

extern "C" void cu_naiveImageCreation(double* u, double* v, double* a, double* b, u8* supply, u8* demand, double reg, double mult, u32 len, u32 width, u8 bytesPerPixel){
	int blockSize = 256;
	int numBlocks = (len + blockSize-1) / blockSize;
	
	zeroOneCopyAnother<<<numBlocks, blockSize>>>(b, a, supply, bytesPerPixel, len);
    cudaDeviceSynchronize();
	
	/*for(int i=0; i<len*bytesPerPixel; i++){
		printf(" %f=%hd ", a[i], supply[i]);
	}
	printf("\n");*/

	naiveImageCreation<<<numBlocks,blockSize>>>(u, v, a, b, supply, demand, reg, mult, len, width, bytesPerPixel);
    cudaDeviceSynchronize();
	/*
	for(int i=0; i<len*bytesPerPixel; i++){
		printf(" %d ", supply[i]);
	}
	printf("\n");
	for(int i=0; i<len*bytesPerPixel; i++){
		printf(" %.1f ", a[i]);
	}

	printf("\n->\n");
	for(int i=0; i<len*bytesPerPixel; i++){
		printf(" %.1f ", b[i]);
	}
	printf("\n");
*/
}
