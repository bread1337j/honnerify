#include "image.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h> //jarvis why the fuck is memcpy in string.h
#include <unistd.h>

struct matrix {
	u32 width;
	u32 height;
	void* data;
};

struct vector3 {
	u16 x, y, z;
};
struct vector2 {
	u16 x, y;
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
	//printf("Expected len: %d\n", out->n);
	//printf("Intensity vector: \n[");
	for(int i=0; i<out->n; i++){
		out->data[i] = 0.299*(*(ptr+i*3))+0.587*(*(ptr+i*3+1))+0.114*(*(ptr+i*3+2)); 
		//printf("%f, ", out->data[i]);
		sum += out->data[i];
	}
	//printf("]\nStochastic intensity vector: \n[");
	for(int i=0; i<out->n; i++){
		out->data[i] = out->data[i] / sum;
		//printf("%f, ", out->data[i]);
	}
	//printf("]\n");

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


double v2Dist(const struct vector2 pos0, const struct vector2 pos1){
	return (pos0.x - pos1.x) * (pos0.x - pos1.x) + (pos0.y - pos1.y) * (pos0.y - pos1.y);
}

double v3Dist(const struct vector3 pos0, const struct vector3 pos1){
	return (pos0.x - pos1.x) * (pos0.x - pos1.x) + (pos0.y - pos1.y) * (pos0.y - pos1.y) + (pos0.z - pos1.z) * (pos0.z - pos1.z);
}

#define WEIGHT_DIST 0.5
#define WEIGHT_COL 0.5

double calcCost(const struct vector2 pos0, const struct vector2 pos1, const struct vector3 col0, const struct vector3 col1){
	return WEIGHT_DIST * v2Dist(pos0, pos1) + WEIGHT_COL * v3Dist(col0, col1);
}

double imgCalcCost(const struct image* img0, const struct image* img1, u32 i0, u32 j0, u32 i1, u32 j1){
	return calcCost((struct vector2){i0, j0}, (struct vector2){i1, j1},  
			(struct vector3){*((u8*)img0->data+(j0*img0->width+i0)), 
							*((u8*)img0->data+(j0*img0->width+i0)+1), 
							*((u8*)img0->data+(j0*img0->width+i0)+2)}, 
			(struct vector3){*((u8*)img1->data+(j1*img1->width+i1)), 
							*((u8*)img1->data+(j1*img1->width+i1)+1), 
							*((u8*)img1->data+(j1*img1->width+i1)+2)});
				//type casting from ohio
}

double chimpCost(double val1, double val2){
	return (val2-val1) * (val2-val1);
}

double gibbs(const double C, const double reg){
	return exp(-C/reg);
}

double gibbsVal(const struct image* img0, const struct image* img1, u32 i0, u32 j0, u32 i1, u32 j1, struct vectorN* v){

}

#define EPSILON 0.001

struct image* stinkhorn(struct image* supply, struct image* demand, double reg, double precision){
	struct vectorN* supplyVector = imgToStochVec(supply);
	struct vectorN* demandVector = imgToStochVec(demand);
	
	struct vectorN* u0 = vectorCopy(supplyVector);
	struct vectorN* v0 = vectorCopy(demandVector);
	
	for(int i=0; i<u0->n; i++){
		u0->data[i] = 1;
		v0->data[i] = 1;
	}
	double error = 1.0 + precision;
	double pError = error;
	double dError = 1;
	while(error > precision && fabs(dError) > 0){ //the vectors must be stochastic and whatnot, so this value is a 0-1 precision scale.
		for(int i=0; i<v0->n; i++){
			double val = 0.0;
			for(int j=0; j<v0->n; j++){
				double cost = chimpCost(u0->data[i], v0->data[j]);
				val += gibbs(cost, reg) * u0->data[j];
			}
			v0->data[i] = demandVector->data[i] / (val);
		}
		pError = error;
		error = 0.0;

		for(int i=0; i<u0->n; i++){
			double val = 0.0;
			for(int j=0; j<u0->n; j++){
				double cost = chimpCost(u0->data[i], v0->data[j]);
				val += gibbs(cost, reg) * v0->data[i];
				
			}
			u0->data[i] = supplyVector->data[i] / (val);

			double current_mass = u0->data[i] * val; 
			double diff = u0->data[i]*val - demandVector->data[i];
			error += (diff>0)?diff:-diff;

		}
		
		dError = error-pError;
		

		printf("err(u,v)=%f\n", error);
		usleep(10000);
	}
	printf("In: \n");
	printVector(supplyVector);
	printVector(demandVector);
	printf("Out: \n");
	printVector(u0);
	printVector(v0);
		
}
