#include "image.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h> 
#include <unistd.h>
#include "multiProcessing.h"

#define MAKE_GIF_FLAG (1<<0)
#define CUDA_BACKEND_FLAG (1<<1)
#define RECURSIVE_IMAGE_FLAG (1<<2)

struct matrix {
	u32 width;
	u32 height;
	void* data;
};

struct vector3 {
	double x, y, z;
};
struct vector2 {
	double x, y;
};

struct vectorN {
	u16 n;
	double* data;
};

void printVector(struct vectorN* vec){
	printf("<");
	for(int i=0; i<vec->n-1; i++){
		printf("%f, ", vec->data[i]);
	}
	printf("%f>\n", vec->data[vec->n-1]);
}

struct vectorN* imgToStochVec(struct image* img, double* buffer){
	struct vectorN* out = (struct vectorN*) malloc(sizeof(struct vectorN));
	out->n = img->width * img->height;
	if(buffer == NULL){
		out->data = (double*)malloc(sizeof(double)*(out->n));
	}else{
		out->data = buffer;
	}
	double sum = 0; 
	u8* ptr = (u8*)img->data;
	for(int i=0; i<out->n; i++){
		out->data[i] = (1+0.299*(*(ptr+i*3))+0.587*(*(ptr+i*3+1))+0.114*(*(ptr+i*3+2)));
		//out->data[i] = (*(ptr+i*3)) * (*(ptr+i*3)) * (*(ptr+i*3+1)) * (*(ptr+i*3+1)) * (*(ptr+i*3+2)) * (*(ptr+i*3+2)); 
		sum += out->data[i];
	}
	
	for(int i=0; i<out->n; i++){
		out->data[i] = out->data[i] / sum;
	}
	
	return out;
}

struct vectorN* emptyVector(size_t n){
	struct vectorN* out = (struct vectorN*)malloc(sizeof(struct vectorN));
	out->n = n;
	out->data = (double*)malloc(sizeof(double)*out->n);
	return out;
}

struct vectorN* vectorCopy(struct vectorN* src){
	struct vectorN* out = (struct vectorN*)malloc(sizeof(struct vectorN));
	out->n = src->n;
	out->data = (double*)malloc(sizeof(double)*out->n);
	memcpy(out->data, src->data, sizeof(double)*out->n);
	return out;
}

double vNDot(struct vectorN* v0, struct vectorN* v1){
	double out = 0;
	//if(v0->n!=v1->n) { dont get into this situation in the first place cuh }
	for(int i=0; i<v0->n; i++){
		out+=v0->data[i]*v1->data[i];
	}
	return out;
}


double imgCalcCost(const struct image* img0, const struct image* img1, u32 i, u32 j){
    struct vector2 p0 = {(double)(i % img0->width) / img0->width, (double)(i / img0->width) / img0->width};
    struct vector2 p1 = {(double)(j % img1->width) / img1->width, (double)(j / img1->width) / img1->width};

	double dPos = (p0.x-p1.x)*(p0.x-p1.x) + (p0.y-p1.y)*(p0.y-p1.y);

    return dPos;
}



double gibbsVal(const struct image* img0, const struct image* img1, u32 i, u32 j, double reg){
	//return imgCalcCost(img0, img1, i, j);
	return exp(-1*imgCalcCost(img0, img1, i, j)/reg);
}




#define EPSILON 1e-10

struct image* createImageCPU(struct image* supply, struct image* demand, struct vectorN* u0, struct vectorN* v0, double reg, struct vectorN* supplyVector){
	struct image* output = resizeImage(supply, supply);//malloc(sizeof(struct image));
	double* buffer_demand = (double*)calloc(output->bytesPerPixel * sizeof(double), output->width * output->height);
	double* buffer_supply = (double*)malloc(output->bytesPerPixel * sizeof(double) * output->width * output->height);
	
	
	u32 sum_supply = 0;
	for(int i=0; i<output->width * output->height; i++){
		for(int j=0; j<output->bytesPerPixel; j++){
			buffer_supply[i*output->bytesPerPixel+j] = *((u8*)output->data+i*output->bytesPerPixel+j);
			sum_supply += buffer_supply[i*output->bytesPerPixel+j];
		}
	}
	printf("Total supply mass pre-transform: %d\n", sum_supply);
/*
	for(int i=0; i<u0->n; i++){
		for(int j=0; j<v0->n; j++){
			double val = gibbsVal(supply, demand, i, j, reg)*u0->data[i]*v0->data[j];
			printf(" %f ", val);
		}
		printf("\n");
	}

	for(int i=0; i<u0->n; i++){
		printf(" %f ", supplyVector->data[i]);
	}
*/
	for(int i=0; i<u0->n; i++){
		u8* ptr0 = (u8*)supply->data+(i*supply->bytesPerPixel);
		double rowSum = 0;
		for(int j=0; j<v0->n; j++){
			//calculate the transport plan matrix at this entry ij
			double val = gibbsVal(supply, demand, i, j, reg)*u0->data[i]*v0->data[j];
			//printf(" %f ", val);
			rowSum+=val;
			double* ptr1 = buffer_demand+(j*output->bytesPerPixel);
			double* ptr2 = buffer_supply+(i*output->bytesPerPixel);
			for(int k=0; k<output->bytesPerPixel; k++){
				double transport = (*(ptr0+k))*(val/supplyVector->data[i]);
			/*	if(transport > 1e-4){
					printf("t%f\n", transport);

				}*/
				if(*(ptr2+k) < transport){
					transport = *(ptr2+k);
				}
				if(*(ptr1+k)+transport > 255){
					transport = 255 - *(ptr1+k);
				}
				*(ptr1+k) += transport;
				*(ptr2+k) -= transport;
				
			}

		}
		//printf("\n%f\n", rowSum);
	}
	sum_supply = 0;
	u32 sum_demand = 0;
	for(int i=0; i<output->width * output->height; i++){
		for(int j=0; j<output->bytesPerPixel; j++){
			sum_supply += buffer_supply[i*output->bytesPerPixel+j];
			sum_demand += buffer_demand[i*output->bytesPerPixel+j];
		}
	}
	/*
	printf("\nbuf supply:\n");
	for(int i=0; i<output->width * output->height * output->bytesPerPixel; i++){
		printf(" %f ", buffer_supply[i]);
	}
	printf("\nbuf demand:\n");
	for(int i=0; i<output->width * output->height * output->bytesPerPixel; i++){
		printf(" %f ", buffer_demand[i]);
	}
	printf("\n");
*/


	printf("total supply mass post-calc: %d\n", sum_supply);
	printf("total demand mass post-calc: %d\n", sum_demand);
	printf("Total mass post-calc: %d\n", sum_supply+sum_demand);
	sum_supply = 0;
	sum_demand = 0;
	u32 total_mass = 0;

	double descaler = 1;

	for(int i=0; i<output->bytesPerPixel * output->width * output->height; i++){
		double pixelVal = ceil(buffer_demand[i]+buffer_supply[i]);
		//printf(" d%.1fs%.1f ", buffer_demand[i], buffer_supply[i]);
		if(pixelVal > 255){
			pixelVal = 255;
		}else if(pixelVal < 0){
			pixelVal = 0;
		}
		sum_supply+=*(((u8*)supply->data+i));
		sum_demand+=(u8)pixelVal;
	}
	//printf("\n");
	descaler = (double)sum_supply / sum_demand;
	printf("descaling constant: %d/%d=%f\n", sum_supply, sum_demand, descaler);
	sum_supply = 0;
	sum_demand = 0;
	//printf("Out: \n");
	for(int i=0; i<output->bytesPerPixel * output->width * output->height; i++){
		double pixelVal=ceil(buffer_demand[i]+buffer_supply[i]);
		pixelVal *= descaler;
		if(pixelVal > 255){
			pixelVal = 255;
		}else if(pixelVal < 0){
			pixelVal = 0;
		}
		*((u8*)(output->data)+i) = (u8)(pixelVal);
		//printf(" %f ", pixelVal);
		sum_supply+=*(((u8*)supply->data+i));
		sum_demand+=*(((u8*)output->data+i));
		total_mass += (u8)pixelVal;
	}
	//printf("\n");

	printf("total output mass post-transform: %d\n", sum_demand);
	printf("total mass post-transform: %d\n", total_mass);
	return output;
}








// well this stinks 
struct image* stinkhornCPU(struct image* supply, struct image* demand, double reg, double precision, u32 maxIter, u8 flags){
	struct vectorN* supplyVector = imgToStochVec(supply, NULL);
	struct vectorN* demandVector = imgToStochVec(demand, NULL);
	struct vectorN* u0 = (struct vectorN*)malloc(sizeof(struct vectorN)); 
	struct vectorN* v0 = (struct vectorN*)malloc(sizeof(struct vectorN)); 
	u0->n = supplyVector->n;
	v0->n = demandVector->n;
	u0->data = (double*)(malloc(sizeof(double)*u0->n));
	v0->data = (double*)(malloc(sizeof(double)*v0->n));
	printf("%p\n", u0->data);


	for(int i=0; i<u0->n; i++){
		u0->data[i] = 1;
		v0->data[i] = 1;
	}
	double error = 1.0 + precision;
	double pError = error;
	double dError = 1;
	u8 c = 140;
	u16 iter = 0;
	char* str = (char*)malloc(50);

	

	while(error > precision && c > 0 && iter < maxIter){ 
		for(int i=0; i<v0->n; i++){
			double val = 0.0;
			for(int j=0; j<v0->n; j++){
				val += gibbsVal(supply, demand, j, i, reg)*u0->data[j];
			}
			if(val > 1e-7){
				v0->data[i] = demandVector->data[i] / (val);
			}
		}
		pError = error;
		error = 0.0;

		for(int i=0; i<u0->n; i++){
			double val = 0.0;
			for(int j=0; j<u0->n; j++){
				val += gibbsVal(supply, demand, i, j, reg) * v0->data[j];
			}
			double temp = u0->data[i];
			if(val > 1e-7){
				u0->data[i] = supplyVector->data[i] / (val);
			}

			double diff = temp - u0->data[i];
			error += (diff > 0) ? diff : -diff; 
		}
		
		dError = error-pError;
		if(fabs(dError) < 1e-5){
			c--;
		}

		if(flags & (MAKE_GIF_FLAG | RECURSIVE_IMAGE_FLAG)){
			struct image* prog = createImageCPU(supply, demand, u0, v0, reg, supplyVector);
			if(flags & MAKE_GIF_FLAG){
				sprintf(str, "output/gif/%04d.png", iter);
				writeImage(prog, str);
			}
			if(flags & RECURSIVE_IMAGE_FLAG){
				free(supply->data);
				free(supply);
				//free(supplyVector->data);
				free(supplyVector);
				supply = prog; 
				supplyVector = imgToStochVec(supply, NULL);
			}
		}
		iter++;
	}
	
	//printf("In: \n");
	//printVector(supplyVector);
	//printVector(demandVector);

	//printVector(u0);
	//printVector(v0);

	return createImageCPU(supply, demand, u0, v0, reg, supplyVector);
}

struct image* createImage(struct image* supply, struct image* demand, struct vectorN* u0, struct vectorN* v0, double reg, struct vectorN* supplyVector){
	struct image* output = malloc(sizeof(struct image));
	output->width = supply->width; 
	output->height = supply->height; 
	output->bytesPerPixel = supply->bytesPerPixel;
	
	double* buffer_demand; 
	double* buffer_supply;

	cu_createArr(&output->data, sizeof(u8)*output->width*output->height*output->bytesPerPixel);
	cu_createArr((void**)&buffer_demand, output->bytesPerPixel * sizeof(double) * output->width * output->height);
	cu_createArr((void**)&buffer_supply, output->bytesPerPixel * sizeof(double) * output->width * output->height);
	
	double mult = 1;//supplyVector->n*1.0;

	cu_naiveImageCreation(u0->data, v0->data, buffer_supply, buffer_demand, supply->data, demand->data, reg, mult, u0->n, supply->width, supply->bytesPerPixel, supplyVector->data);
	double sum_supply = 0;
	double sum_demand = 0;
	u32 total_mass = 0;
	/*
	printf("\nbuf supply:\n");
	for(int i=0; i<output->width * output->height * output->bytesPerPixel; i++){
		printf(" %f ", buffer_supply[i]);
	}
	printf("\nbuf_demand:\n");
	for(int i=0; i<output->width * output->height * output->bytesPerPixel; i++){
		printf(" %f ", buffer_demand[i]);
	}
	printf("\n");
	
	for(int i=0; i<output->bytesPerPixel * output->width * output->height; i++){
		double pixelVal = ceil(buffer_demand[i]+buffer_supply[i]);
		//printf(" d%.1fs%.1f ", buffer_demand[i], buffer_supply[i]);
		if(pixelVal > 255){
			pixelVal = 255;
		}else if(pixelVal < 0){
			pixelVal = 0;
		}
		sum_supply+=*(((u8*)supply->data+i));
		sum_demand+=(u8)pixelVal;
	}
	printf("\n");
	printf("total supply mass: %f\n", sum_supply); //everything breaks without this print statement. I don't know why.,, 
	printf("total output mass post-calc: %f\n", sum_demand);
	
	descaler = (double)sum_supply / sum_demand;
	printf("Descaler=%f\n", descaler);
	*/
	u32 sum_output = 0;
	for(int i=0; i<output->bytesPerPixel * output->width * output->height; i++){
		double pixelVal=ceil(buffer_demand[i]+buffer_supply[i]);
		//pixelVal *= descaler;
		if(pixelVal > 255){
			pixelVal = 255;
		}else if(pixelVal < 0){
			pixelVal = 0;
		}
		*((u8*)(output->data)+i) = (u8)(pixelVal);
		sum_output += (u8)pixelVal;
	}
	
	printf("Final output mass: %d\n", sum_output);

	return output;

}

struct image* sinkhornCuda(struct image* supply, struct image* demand, double reg, double precision, u32 maxIter, u8 flags){
	double* buf1; double* buf2;
	cu_createArr((void**)&(buf1),supply->width*supply->height*sizeof(double));
	cu_createArr((void**)&(buf2),demand->width*demand->height*sizeof(double));
	struct vectorN* supplyVector = imgToStochVec(supply, buf1);
	struct vectorN* demandVector = imgToStochVec(demand, buf2);
	struct vectorN* u0 = (struct vectorN*)malloc(sizeof(struct vectorN)); 
	struct vectorN* v0 = (struct vectorN*)malloc(sizeof(struct vectorN)); 
	u0->n = supplyVector->n;
	v0->n = demandVector->n;
	//printf("%p %p %p %p\n", u0->data, v0->data, supplyVector->data, demandVector->data);
	cu_createArr((void**)&(u0->data), sizeof(double) * u0->n);
	cu_createArr((void**)&(v0->data), sizeof(double) * v0->n);
	
	//printf("%p %p %d!=%lu\n", u0->data, v0->data, u0->n*sizeof(double), v0->data-u0->data); //padding moment trollface
	
	for(int i=0; i<u0->n; i++){
		u0->data[i] = 1;
		v0->data[i] = 1;
	}
	double error = 1.0 + precision;
	double pError = error;
	double dError = 1;
	u8 c = 14;
	u16 iter = 0;
	char* str = (char*)malloc(50);
	
	while(iter < maxIter){ 
		callSinkhornStep1(u0->data, v0->data, supplyVector->data, demandVector->data, u0->n, supply->width, supply->height, reg);
		callSinkhornStep2(u0->data, v0->data, supplyVector->data, demandVector->data, u0->n, supply->width, supply->height, reg);
		printf("Iteration %d\n", iter);
		iter++;
		if(flags & (MAKE_GIF_FLAG | RECURSIVE_IMAGE_FLAG)){
			struct image* prog = createImage(supply, demand, u0, v0, reg, supplyVector);
			if(flags & MAKE_GIF_FLAG){
				sprintf(str, "output/gif/%04d.png", iter);
				writeImage(prog, str);
			}
			if(flags & RECURSIVE_IMAGE_FLAG){
				//free(supply->data);
				//free(supply);
				//free(supplyVector->data);
				//free(supplyVector);
				supply = prog; 
				supplyVector = imgToStochVec(supply, buf1);
			}
		}
	}
	//printVector(u0);
	//printVector(v0);
	return createImage(supply, demand, u0, v0, reg, supplyVector);
	//return createImageCPU(supply, demand, u0, v0, reg, supplyVector);
}

struct image* sinkhorn(struct image* supply, struct image* demand, double reg, double precision, u32 maxIter, u8 flags){
	if(flags&CUDA_BACKEND_FLAG) {
		return sinkhornCuda(supply, demand, reg, precision, maxIter, flags);
	}else{
		return stinkhornCPU(supply, demand, reg, precision, maxIter, flags);
	}
	return NULL;
}
