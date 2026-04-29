#include "image.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h> //jarvis why the fuck is memcpy in string.h
#include <unistd.h>
#include <omp.h>

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
	struct vectorN* out = malloc(sizeof(struct vectorN));
	out->n = img->width * img->height;
	out->data = malloc(sizeof(double)*out->n);
	double sum = 0; 
	u8* ptr = img->data;
	for(int i=0; i<out->n; i++){
		out->data[i] = 0.299*(*(ptr+i*3))+0.587*(*(ptr+i*3+1))+0.114*(*(ptr+i*3+2)); 
		sum += out->data[i];
	}
	for(int i=0; i<out->n; i++){
		out->data[i] = out->data[i] / sum;
	}

	return out;
}

struct vectorN* emptyVector(size_t n){
	struct vectorN* out = malloc(sizeof(struct vectorN));
	out->n = n;
	out->data = malloc(sizeof(double)*out->n);
	return out;
}

struct vectorN* vectorCopy(struct vectorN* src){
	struct vectorN* out = malloc(sizeof(struct vectorN));
	out->n = src->n;
	out->data = malloc(sizeof(double)*out->n);
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


#define WEIGHT_DIST 0.30
#define WEIGHT_COL (1.00-WEIGHT_DIST)

#define NORM_POS ((double)img0->width)

long double imgCalcCost(const struct image* img0, const struct image* img1, u32 i, u32 j){
    struct vector2 p0 = {(double)(i % img0->width) / NORM_POS, (double)(i / img0->width) / NORM_POS};
    struct vector2 p1 = {(double)(j % img1->width) / NORM_POS, (double)(j / img1->width) / NORM_POS};

    struct vector3 c0 = {
        (double)(*((u8*)img0->data + (i*3))) / 255.0,
        (double)(*((u8*)img0->data + (i*3) + 1)) / 255.0,
        (double)(*((u8*)img0->data + (i*3) + 2)) / 255.0
    };
    struct vector3 c1 = {
        (double)(*((u8*)img1->data + (j*3))) / 255.0,
        (double)(*((u8*)img1->data + (j*3) + 1)) / 255.0,
        (double)(*((u8*)img1->data + (j*3) + 2)) / 255.0
    };

    // Use Euclidean distance for color for better results
	double dPos = (p0.x-p1.x)*(p0.x-p1.x) + (p0.y-p1.y)*(p0.y-p1.y);
    double dCol = (c0.x-c1.x)*(c0.x-c1.x) + (c0.y-c1.y)*(c0.y-c1.y) + (c0.z-c1.z)*(c0.z-c1.z);
    
    return (WEIGHT_DIST * dPos) + (WEIGHT_COL * dCol);
}



double gibbsVal(const struct image* img0, const struct image* img1, u32 i, u32 j, double reg){
	return exp(-1*imgCalcCost(img0, img1, i, j)/reg);
}

#define EPSILON 1e-10

struct image* stinkhorn(struct image* supply, struct image* demand, double reg, double precision){
	struct vectorN* supplyVector = imgToStochVec(supply);
	struct vectorN* demandVector = imgToStochVec(demand);
	
	struct vectorN* u0 = emptyVector(supplyVector->n);
	struct vectorN* v0 = emptyVector(demandVector->n);
	for(int i=0; i<u0->n; i++){
		u0->data[i] = 1;
		v0->data[i] = 1;
	}
	double error = 1.0 + precision;
	double pError = error;
	double dError = 1;
	u16 iter = 0;
	while(error > precision && (fabs(dError)/error > 0) && iter < 2000){ //the vectors must be stochastic and whatnot, so this value is a 0-1 precision scale.
		#pragma omp parallel for
		for(int i=0; i<v0->n; i++){
			double val = 0.0;
			for(int j=0; j<v0->n; j++){
				//double cost = chimpCost(u0->data[i], v0->data[j]);
				//val += gibbs(cost, reg) * u0->data[j];
				val += gibbsVal(supply, demand, j, i, reg)*u0->data[j];
			}
			v0->data[i] = demandVector->data[i] / (val+EPSILON);
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
			u0->data[i] = supplyVector->data[i] / (val+EPSILON);

			double diff = temp - u0->data[i];
			error += (diff > 0) ? diff : -diff; 
			//printf("%f %f, %f\n", error, val, diff);
		}
		
		dError = error-pError;
		

		printf("err(%d)=%f\n", iter, error);
		iter++;
		//usleep(10000);
	}
	
	printf("In: \n");
	printVector(supplyVector);
	printVector(demandVector);
	printf("Out: \n");
	printVector(u0);
	printVector(v0);
	struct image* output = resizeImage(supply, supply);//malloc(sizeof(struct image));
	printf("output %p\n", output);
	output->width = supply->width;
	output->height = supply->height;
	output->bytesPerPixel = supply->bytesPerPixel;
	//output->data = calloc(output->bytesPerPixel , output->width * output->height);
	printf("output->data %p\n", output->data);
	
	double* T_x = malloc(u0->n*sizeof(double));
	printf("T_x %p\n", T_x);
	double* T_y = malloc(u0->n*sizeof(double));
	printf("T_y %p\n", T_y);
	
	for(int i=0; i<u0->n; i++){
		for(int j=0; j<v0->n; j++){
			//calculate the transport plan matrix at this entry ij

		}
	}

	return output;


}
