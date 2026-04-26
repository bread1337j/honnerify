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
	//return fabs((pos0.x * pos0.x + pos0.y * pos0.y + pos0.z * pos0.z) - (pos1.x * pos1.x + pos1.y * pos1.y + pos1.z * pos1.z));
	//double i0 = 0.299*pos0.x + 0.587 * pos0.y + 0.114 * pos0.z;
	//double i1 = 0.299*pos1.x + 0.587 * pos1.y + 0.114 * pos1.z;
	//return fabs(i1-i0);
}

#define WEIGHT_DIST 0.30
#define WEIGHT_COL (1.00-WEIGHT_DIST)

long double calcCost(const struct vector2 pos0, const struct vector2 pos1, const struct vector3 col0, const struct vector3 col1){
	//printf("(%.1f, %.1f)|(%.1f %.1f %.1f)->(%.1f, %.1f)|(%.1f %.1f %.1f)\n", pos0.x, pos0.y, col0.x, col0.y, col0.z, pos1.x, pos1.y, col1.x, col1.y, col1.z);

	return (WEIGHT_DIST * v2Dist(pos0, pos1) + WEIGHT_COL * v3Dist(col0, col1));
}
/*
#define IMG_REG img0->width

long double imgCalcCost(const struct image* img0, const struct image* img1, u32 i, u32 j){
	return calcCost(
					(struct vector2){(double)(i%(img0->width)) / IMG_REG, ((double)i)/(img0->width)/IMG_REG}, 
					(struct vector2){(double)(j%(img0->width)) / IMG_REG, ((double)j)/(img0->width)/IMG_REG},  

			(struct vector3){(double)(*((u8*)img0->data+(i*3)))/255, 
							 (double)(*((u8*)img0->data+(i*3)+1))/255, 
							 (double)(*((u8*)img0->data+(i*3)+2))/255}, 
			(struct vector3){(double)(*((u8*)img1->data+(j*3)))/255, 
					    	 (double)(*((u8*)img1->data+(j*3)+1))/255, 
							 (double)(*((u8*)img1->data+(j*3)+2))/255});
				//type casting from ohio
}
*/
#define NORM_VAL ((double)img0->width)

long double imgCalcCost(const struct image* img0, const struct image* img1, u32 i, u32 j){
    struct vector2 p0 = {(double)(i % img0->width) / NORM_VAL, (double)(i / img0->width) / NORM_VAL};
    struct vector2 p1 = {(double)(j % img1->width) / NORM_VAL, (double)(j / img1->width) / NORM_VAL};

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
    double dCol = (c0.x-c1.x)*(c0.x-c1.x) + (c0.y-c1.y)*(c0.y-c1.y) + (c0.z-c1.z)*(c0.z-c1.z);
    
    return (WEIGHT_DIST * v2Dist(p0, p1) + WEIGHT_COL * dCol);
}

double chimpCost(double val1, double val2){
	return (val2-val1) * (val2-val1);
}

double gibbs(const double C, const double reg){
	return exp(-C/reg);
}

double gibbsVal(const struct image* img0, const struct image* img1, u32 i, u32 j, double reg){
	//printf("imgCalcCost=%Lf, exp(-1*blah blah)=%f\n", imgCalcCost(img0, img1, i, j), exp(-1*imgCalcCost(img0, img1, i, j)/reg));
	return exp(-1*imgCalcCost(img0, img1, i, j)/reg);
}

#define EPSILON 1e-10

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
	/*
	printf("In: \n");
	printVector(supplyVector);
	printVector(demandVector);
	printf("Out: \n");
	printVector(u0);
	printVector(v0);

	double sum_x = 0.0;
	double sum_y = 0.0;
	double sum_total = 0.0;
	for(int i=0; i<u0->n; i++){
		for (int j = 0; j < v0->n; j++) {
			double mass_moved = u0->data[i] * gibbsVal(supply, demand, i, j, reg, v0);
			sum_x+=mass_moved*(j%(supply->width));
			sum_y+=mass_moved*((double)j/(supply->width));
			sum_total += mass_moved;
			printf("(%d -> %d): %.03f, ", i, j, mass_moved);
		}
		printf("\n");
	}
	printf("Directional sum: (%f, %f)\n", sum_x,sum_y);
	printf("Total sum: %f\n", sum_total);
	*/
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
	//should probably replace this later but it will make the code simpler for now
	
	//memset(output->data, 0, output->bytesPerPixel * output->width * output->height);
	/*
	for(int i=0; i<u0->n; i++){
		if(supplyVector->data[i] < 1e-8){
			T_x[i] = (double)(i%output->width);
			T_y[i] = (double)(i/output->width);
		}else{
			double sum_x = 0.0;
			double sum_y = 0.0;
			for(int j=0; j<u0->n; j++){
				double p = gibbsVal(supply, demand, i, j, reg) * u0->data[i] * v0->data[j];
				sum_x += p * (double)(j % output->width);
				sum_y += p * (double)(j / output->width);

			}
			
			T_x[i] = sum_x / supplyVector->data[i];// * output->width;
			T_y[i] = sum_y / supplyVector->data[i];// * output->height;
		}
		int tx = (int)(T_x[i] + 0.5);
		int ty = (int)(T_y[i] + 0.5);
		if(tx >= 0 && tx < output->width && ty >= 0 && ty < output->height){
			int target_idx = (ty * output->width + tx) * output->bytesPerPixel;
			memcpy((u8*)output->data+target_idx, (u8*)supply->data+i*supply->bytesPerPixel, output->bytesPerPixel);
		}
	}
	*/
	for(int i=0; i<u0->n; i++){
		if(supplyVector->data[i] > 1e-9){
			double sum_x = 0.0;
			double sum_y = 0.0;
			double sum_p = 0.0;
			for(int j=0; j<v0->n; j++){
				double p = gibbsVal(supply, demand, i, j, reg) * u0->data[i] * v0->data[j];
				sum_x += p * (double)(j % demand->width);
				sum_y += p * (double)(j / demand->width);
				sum_p += p;
			}
			T_x[i] = sum_x / (sum_p + EPSILON);
			T_y[i] = sum_y / (sum_p + EPSILON);
		} else {
			T_x[i] = (double)(i % supply->width);
			T_y[i] = (double)(i / supply->width);
		}

		int tx = (int)(T_x[i] + 0.5);
		int ty = (int)(T_y[i] + 0.5);

		if(tx >= 0 && tx < output->width && ty >= 0 && ty < output->height){
			int target_idx = (ty * output->width + tx) * output->bytesPerPixel;
			int src_idx = i * supply->bytesPerPixel;
			memcpy((u8*)output->data + target_idx, (u8*)supply->data + src_idx, output->bytesPerPixel);
		}
	}

	return output;


}
