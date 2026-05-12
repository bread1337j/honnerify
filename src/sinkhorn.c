#include "image.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h> 
#include <unistd.h>
#include "multiProcessing.h"


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

struct vectorN* imgToStochVec(struct image* img){
	struct vectorN* out = (struct vectorN*) malloc(sizeof(struct vectorN));
	out->n = img->width * img->height;
	out->data = (double*)malloc(sizeof(double)*(out->n+1));
	out->data++;
	double sum = 0; 
	u8* ptr = (u8*)img->data;
	for(int i=0; i<out->n; i++){
		out->data[i] = 0.299*(*(ptr+i*3))+0.587*(*(ptr+i*3+1))+0.114*(*(ptr+i*3+2)) / 255; 
		//out->data[i] = (*(ptr+i*3)) * (*(ptr+i*3)) * (*(ptr+i*3+1)) * (*(ptr+i*3+1)) * (*(ptr+i*3+2)) * (*(ptr+i*3+2)); 
		sum += out->data[i];
	}
	
	for(int i=0; i<out->n; i++){
		out->data[i] = out->data[i] / sum;
	}
	out->data[-1] = sum; //I am in the TRENCHES dawg 
	
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

    return 1*dPos;
}



double gibbsVal(const struct image* img0, const struct image* img1, u32 i, u32 j, double reg){
	return exp(-1*imgCalcCost(img0, img1, i, j)/reg);
}




#define EPSILON 1e-10

struct image* createImage(struct image* supply, struct image* demand, struct vectorN* u0, struct vectorN* v0, double reg, struct vectorN* supplyVector){
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


	for(int i=0; i<u0->n; i++){
		for(int j=0; j<v0->n; j++){
			double val = gibbsVal(supply, demand, i, j, reg)*u0->data[i]*v0->data[j] * output->width;
		}
	}
	double mult = supplyVector->n * 0.9;
	for(int i=0; i<u0->n; i++){
		u8* ptr0 = (u8*)supply->data+(i*supply->bytesPerPixel);
		for(int j=0; j<v0->n; j++){
			//calculate the transport plan matrix at this entry ij
			double val = gibbsVal(supply, demand, i, j, reg)*u0->data[i]*v0->data[j] * mult;
			double* ptr1 = buffer_demand+(j*output->bytesPerPixel);
			double* ptr2 = buffer_supply+(i*output->bytesPerPixel);
			
			for(int k=0; k<output->bytesPerPixel; k++){
				double transport = (*(ptr0+k))*val;
				*(ptr1+k) += transport;
				*(ptr2+k) -= transport;
				
			}

		}
	}
	sum_supply = 0;
	u32 sum_demand = 0;
	for(int i=0; i<output->width * output->height; i++){
		for(int j=0; j<output->bytesPerPixel; j++){
			sum_supply += buffer_supply[i*output->bytesPerPixel+j];
			sum_demand += buffer_demand[i*output->bytesPerPixel+j];
		}
	}
	sum_supply = 0;
	sum_demand = 0;
	for(int i=0; i<output->bytesPerPixel * output->width * output->height; i++){
		double pixelVal=ceil(buffer_demand[i]+buffer_supply[i]);
		if(pixelVal > 255){
			pixelVal = 255;
		}else if(pixelVal < 0){
			pixelVal = 0;
		}
		*((u8*)(output->data)+i) = (u8)(pixelVal);

		sum_supply+=*(((u8*)supply->data+i));
		sum_demand+=*(((u8*)output->data+i));
	}
	//printf("%hhd\n", *((u8*)output->data));
	//printf("Old mass: %d\nNew mass: %d\n", sum_supply, sum_demand);
	//printf("These numbers should be roughly the same!\n");
	return output;
}


double cudaGibbsVal(u32 width, u32 height, u32 i, u32 j, double reg){
	double cost = (i%width - j%width) * (i%width - j%width) + (i/width - j/width) * (i/width - j/width);
	return exp(-1*cost/reg);
}





struct image* stinkhorn(struct image* supply, struct image* demand, double reg, double precision, u32 maxIter, u8 makeGif){
	struct vectorN* supplyVector = imgToStochVec(supply);
	struct vectorN* demandVector = imgToStochVec(demand);
	
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
	u8 c = 14;
	u16 iter = 0;
	char* str = (char*)malloc(50);

	

	while(error > precision && c > 0 && iter < maxIter){ //the vectors must be stochastic and whatnot, so this value is a 0-1 precision scale.
		for(int i=0; i<v0->n; i++){
			double val = 0.0;
			for(int j=0; j<v0->n; j++){
				val += gibbsVal(supply, demand, j, i, reg)*u0->data[j];
			}
			//callSinkhornStep1(v0->data, u0->data, v0->n, supply->width, supply->height, i, reg);
			if(val > 1e-7){
				v0->data[i] = demandVector->data[i] / (val);
			}
		}
		pError = error;
		error = 0.0;

		for(int i=0; i<u0->n; i++){
			double val = 0.0;
			for(int j=0; j<u0->n; j++){
				//double cost = chimpCost(u0->data[i], v0->data[j]);
				//val += gibbs(cost, reg) * v0->data[i];
				val += gibbsVal(supply, demand, i, j, reg) * v0->data[j];
				//printf("%f\n", val);
			}
			double temp = u0->data[i];
			if(val > 1e-7){
				u0->data[i] = supplyVector->data[i] / (val);
			}

			double diff = temp - u0->data[i];
			error += (diff > 0) ? diff : -diff; 
			//printf("%f %f, %f\n", error, val, diff);
		}
		
		dError = error-pError;
		if(fabs(dError) < 1e-5){
			c--;
		}

		printf("err(%d)=%f\n", iter, error);
		if(makeGif){
			struct image* prog = createImage(supply, demand, u0, v0, reg, supplyVector);
			sprintf(str, "output/gif/%d.bmp", iter);
			writeImage(prog, str);
		}
		iter++;
		//usleep(10000);
	}
	
	/*printf("In: \n");
	printVector(supplyVector);
	printVector(demandVector);
	printf("Out: \n");
	printVector(u0);
	printVector(v0);
	*/

	return createImage(supply, demand, u0, v0, reg, supplyVector);




}
