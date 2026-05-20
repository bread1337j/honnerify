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
		double p0_x = (double)(i % width) / (double)width;
		double p0_y = (double)(i / width) / (double)width;
		for(int j=0; j<len; j++){
			
			double p1_x = (double)(j % width) / (double)width;
			double p1_y = (double)(j / width) / (double)width;
			//probably less efficient this way but like. I want to be able to read my code man.
			double cost = (p0_x - p1_x)*(p0_x - p1_x) + (p0_y - p1_y)*(p0_y - p1_y);
			
			//double cost = ((i%width)/width - (j%width)/width) * ((i%width)/width - (j%width)/width) + ((i/width)/width - (j/width)/width) * ((i/width)/width - (j/width)/width);
			cost *= (width);
			//val += __expf(-1 * cost / reg) * u[j];
			val += exp(-1 * cost / reg) * u[j];
		}
		if(val > 1e-7){
			//v[i] = b[i] / val;
			v[i] = 1.0 / val;
		}
	}
}

__global__ void sinkhornStep2(double* u, double* v, double* a, double* b, u32 len, u32 width, u32 height, double reg){
	int index = blockIdx.x * blockDim.x + threadIdx.x;
	int stride = blockDim.x * gridDim.x;
	for(int i=index; i<len; i+=stride){
		double val = 0.0;
		double p0_x = (double)(i % width) / (double)width;
		double p0_y = (double)(i / width) / (double)width;
		for(int j=0; j<len; j++){ //since the cost is just distance, it should be symmetric, thus C^T=C
			
			double p1_x = (double)(j % width) / (double)width;
			double p1_y = (double)(j / width) / (double)width;

			double cost = (p0_x - p1_x)*(p0_x - p1_x) + (p0_y - p1_y)*(p0_y - p1_y);
			
			//double cost = ((i%width)/width - (j%width)/width) * ((i%width)/width - (j%width)/width) + ((i/width)/width - (j/width)/width) * ((i/width)/width - (j/width)/width);
		
			cost *= (width);
			//val += __expf(-1 * cost / reg) * v[j];
			val += exp(-1 * cost / reg) * v[j];
		}
		if(val > 1e-7){
			//u[i] = a[i] / val;
			u[i] = 1.0 / val;
		}
	}

}



__global__ void naiveImageCreation(double* u, double* v, double* a, double* b, u8* supply, double* supplySubtractor9000, double reg, double mult, u32 len, u32 width, u8 bytesPerPixel, double* sum, double* supplyVector){
	//pretty much step 3 tbh
	int j = blockIdx.x * blockDim.x + threadIdx.x;

	double p1_x = (double) (j%width) / (double) width;
	double p1_y = (double) (j/width) / (double) width;
	double* ptr1 = b + (j * bytesPerPixel);

	for(int i=0; i<len; i++){
		double p0_x = (double) (i%width) / (double) width;
		double p0_y = (double) (i/width) / (double) width;

		double cost = (p0_x - p1_x)*(p0_x - p1_x) + (p0_y - p1_y)*(p0_y - p1_y);
		cost *= width;
		//double val = __expf(-1 * cost / reg) * v[j] * u[i] * mult;
		double val = exp(-1 * cost / reg) * v[j] * u[i];
		//atomicAdd(sum, val);
		if(val > 1e-4){
			printf(" %d %d %f \n", i, j, val);
		}
		for(int k=0; k<bytesPerPixel; k++){
			double transport = ((double)supply[i*bytesPerPixel + k]) * (val / supplyVector[i]);
			transport = fmin(transport, (double)supply[i * bytesPerPixel + k]);
			transport = fmin(transport, 255.0 - ptr1[k]);

			
			//atomicAdd(ptr1+k, transport); 
			ptr1[k] += transport;
			atomicAdd(supplySubtractor9000+(i*3+k), transport);
		}
	}
	//printf("\n");
}

__global__ void subtractSupply(double* supply, double* supplySubtractor9000, u32 len, u8 bytesPerPixel){
	int index = blockIdx.x * blockDim.x + threadIdx.x;
	for (int k=0; k<bytesPerPixel; k++){
		supply[index*bytesPerPixel+k] -= supplySubtractor9000[index*bytesPerPixel+k];
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
	//cudaMemPrefetchAsync(u, len*sizeof(double),0,0);
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
	cudaError_t err = cudaMallocManaged(ptr, len);
	if(err != cudaSuccess){
		fprintf(stderr, "Epic cuda malloc fail: %s\n", cudaGetErrorString(err));
	}
}

extern "C" void cu_naiveImageCreation(double* u, double* v, double* a, double* b, u8* supply, u8* demand, double reg, double mult, u32 len, u32 width, u8 bytesPerPixel, double* supplyVector){ //this shit....is so ass...
    cudaDeviceSynchronize();
	int blockSize = 256;
	int numBlocks = (len + blockSize-1) / blockSize;
	
	zeroOneCopyAnother<<<numBlocks, blockSize>>>(b, a, supply, bytesPerPixel, len);
    cudaDeviceSynchronize();
	
	/*for(int i=0; i<len*bytesPerPixel; i++){
		printf(" %f=%hd ", a[i], supply[i]);
	}
	printf("\n");*/
	
	double* supplySubtractor9000; 
	cudaMalloc(&supplySubtractor9000, len*bytesPerPixel*sizeof(double));
	cudaMemset(supplySubtractor9000, 0, len*bytesPerPixel*sizeof(double));
	
	double* sum; //the non-double stochasticness mechanism
	cu_createArr((void**)&sum, sizeof(double));
	
	double* sVec;

	cudaMallocManaged(&sVec, len*sizeof(double));
	for(int i=0; i<len; i++){
		sVec[i] = supplyVector[i]; //temporary code (spoiler alert it is staying this way,,
	}
	//cudaMemcpy(sVec, supplyVector, width*bytesPerPixel*sizeof(double), cudaMemcpyHostToDevice);

	naiveImageCreation<<<numBlocks,blockSize>>>(u, v, a, b, supply, supplySubtractor9000, reg, mult, len, width, bytesPerPixel, sum, sVec);
    cudaDeviceSynchronize();

	subtractSupply<<<numBlocks, blockSize>>>(a, supplySubtractor9000, len, bytesPerPixel);
    cudaDeviceSynchronize();


	//printf("%f %d", *sum, width);
	
	cudaFree(supplySubtractor9000);
	cudaFree(sum);
	cudaFree(sVec);


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
