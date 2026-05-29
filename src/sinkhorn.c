#include "sinkhorn.h"
#include "image.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h> 
#include <unistd.h>
#include "multiProcessing.h"

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
		out->data[i] = (1+0.299*(*(ptr+i*3))+0.587*(*(ptr+i*3+1))+0.114*(*(ptr+i*3+2))); // THE ACTUAL LUMINANCE
		// out->data[i] = (*(ptr+i*3)) * (*(ptr+i*3)) + (*(ptr+i*3+1)) * (*(ptr+i*3+1)) + (*(ptr+i*3+2)) * (*(ptr+i*3+2)); // nOT THIIS ONE
		//out->data[i] = 1.0 / out->n;
		/*if(out->data[i] == 0){
			out->data[i] = 1;
		}*/
		sum += out->data[i];
	}
	printf("Created vector of sum %f, it will be normalized to 1.\n", sum);
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
    struct vector2 p0 = {(double)(i % img0->width), (double)(i / img0->width)}; // Why is this kid normalizing the x and y values
    struct vector2 p1 = {(double)(j % img1->width), (double)(j / img1->width)};

	double dPos = (p0.x-p1.x)*(p0.x-p1.x) + (p0.y-p1.y)*(p0.y-p1.y);

    return dPos;
}



double gibbsVal(const struct image* img0, const struct image* img1, u32 i, u32 j, double reg){
	//return imgCalcCost(img0, img1, i, j);
	double cost = imgCalcCost(img0, img1, i, j);
	double output = exp(-1*cost/reg);
	// printf("Cost is %lf -> GibbsVal is %lf\n", cost, output);
	return output;
}


void printMatrixInfo(struct image* supply, struct image* demand, struct vectorN* u0, struct vectorN* v0, double reg){
	printf("u:\n");
	printVector(u0);
	printf("v:\n");
	printVector(v0);
	printf("K:\n");
	for(int i=0; i<u0->n; i++){
		for(int j=0; j<v0->n; j++){
			double val = gibbsVal(supply, demand, i, j, reg);
			printf(" %f ", val);
		}
		printf("\n");
	}
	printf("uKv:\n");
	for(int i=0; i<u0->n; i++){
		for(int j=0; j<v0->n; j++){
			double val = gibbsVal(supply, demand, i, j, reg)*u0->data[i]*v0->data[j];
			printf(" %f ", val);
		}
		printf("\n");
	}
	double* rowSums = calloc(sizeof(double) , u0->n);
	double* colSums = calloc(sizeof(double) , u0->n);

	for(int j=0; j<u0->n; j++){
		for(int i=0; i<u0->n; i++){
			rowSums[j] += gibbsVal(supply, demand, i, j, reg)*u0->data[i]*v0->data[j];
			colSums[j] += gibbsVal(supply, demand, j, i, reg)*u0->data[j]*v0->data[i];
		}
		
	}
	printf("Sums:\nRows:\n");
	for(int i=0; i<u0->n; i++){
		printf(" %f ", rowSums[i]);
	}
	printf("\nColumns:\n");
	for(int i=0; i<u0->n; i++){
		printf(" %f ", colSums[i]);
	}

	printf("\n");
}

#define EPSILON 1e-10

struct image* createImageCPU(struct image* supply, struct image* demand, struct vectorN* u0, struct vectorN* v0, double reg, struct vectorN* supplyVector, u8 flags){
	struct image* output = resizeImage(supply, supply);//malloc(sizeof(struct image));
	double* buffer_demand = (double*)calloc(output->bytesPerPixel * sizeof(double), output->width * output->height);
	double* buffer_supply = (double*)malloc(output->bytesPerPixel * sizeof(double) * output->width * output->height);
	double sum_supply = 0;
	for(int i=0; i<output->width * output->height; i++){
		for(int j=0; j<output->bytesPerPixel; j++){
			buffer_supply[i*output->bytesPerPixel+j] = *((u8*)output->data+i*output->bytesPerPixel+j);
			sum_supply += buffer_supply[i*output->bytesPerPixel+j];
		}
	}
	printf("Total supply mass pre-transform: %f\n", sum_supply);
	
	//printMatrixInfo(supply, demand, u0, v0, reg);
	


/*
	for(int i=0; i<u0->n; i++){
		printf(" %f ", supplyVector->data[i]);
	}
*/
	for(int i=0; i<u0->n; i++){
		u8* ptr0 = (u8*)supply->data+(i*supply->bytesPerPixel);
		double rowSum = 0;
		for(int j=0; j<v0->n; j++){
			if(supplyVector->data[i] < 1e-4){
				continue;
			}
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
	double sum_demand = 0;
	for(int i=0; i<output->width * output->height; i++){
		for(int j=0; j<output->bytesPerPixel; j++){
			sum_supply += buffer_supply[i*output->bytesPerPixel+j];
			sum_demand += buffer_demand[i*output->bytesPerPixel+j];
		}
	}
	
	/*printf("\nbuf supply:\n");
	for(int i=0; i<output->width * output->height * output->bytesPerPixel; i++){
		printf(" %f ", buffer_supply[i]);
	}
	printf("\nbuf demand:\n");
	for(int i=0; i<output->width * output->height * output->bytesPerPixel; i++){
		printf(" %f ", buffer_demand[i]);
	}
	printf("\n");*/



	printf("total supply mass post-calc: %f\n", sum_supply);
	printf("total demand mass post-calc: %f\n", sum_demand);
	printf("Total mass post-calc: %f\n", sum_supply+sum_demand);
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
	descaler = sum_demand / sum_supply;
	//descaler = 1;
	printf("descaling constant: %f/%f=%f\n", sum_supply, sum_demand, descaler);
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


#define MIN(i, j) (((i) < (j)) ? (i) : (j))
#define MAX(i, j) (((i) > (j)) ? (i) : (j))
struct image* discreteCreateImage(struct image* supply, struct image* demand, struct vectorN* u0, struct vectorN* v0, double reg, struct vectorN* supplyVector, struct vectorN* demandVector, u8 flags) {
	// possibly make these user defined inputs....
	int searchRadius = 25;
	double thresholdDelta = 0.025;


	int totalPixels = supply->width * supply->height;
	int numUnassigned = totalPixels;
	int* assignedDemandPixels = calloc(totalPixels, sizeof(int)); //demand obv
	int* pixelsToAssign = malloc(sizeof(int) * totalPixels); //ie supply
	srand(0);
	for (int i = 0; i < totalPixels; i++) {
		pixelsToAssign[i] = i;//(int)(random()*totalPixels);
		// RANDOMIZE THIS SELECTION
	}
	for (int i = 0; i < totalPixels; i++) {
		int j = (int)(random() % (totalPixels));
		int temp = pixelsToAssign[i];
		pixelsToAssign[i] = pixelsToAssign[j];
		pixelsToAssign[j] = temp;
		// RANDOMIZE THIS SELECTION
		// ok :thumbsup:
	}


	struct vector2* assignments = calloc(totalPixels, sizeof(struct vector2));
	// x=original position/supply   &&     y=new position/demand

	//Total pixels should actually be total demand pixels but we're only ever using this for images
	struct vectorN* reglna = malloc(sizeof(struct vectorN)); 
	reglna->data = malloc(sizeof(double) * demandVector->n);
	for (int demandPix = 0; demandPix < totalPixels; demandPix++) {
		double demandScaler = v0->data[demandPix];
		reglna->data[demandPix] = reg * log(demandScaler);
	}

	struct vectorN* reglnb = malloc(sizeof(struct vectorN));
	reglnb->data = malloc(sizeof(double) * supplyVector->n);
	for (int supplyPix = 0; supplyPix < totalPixels; supplyPix++) {
		double supplyScaler = u0->data[supplyPix];
		reglnb->data[supplyPix] = reg * log(supplyScaler);
	}

	int oneExtraPass = 0;
	for (double threshold = 1; threshold > 0.0 || !oneExtraPass; threshold -= thresholdDelta) {
		if (threshold <= 0.0) {
			oneExtraPass = 1;
			searchRadius = MAX(supply->width, supply->height);
		}
		int anyCandidates = 0;
		for (int pixelIndex = 0; pixelIndex < numUnassigned; pixelIndex++) {
			int supplyPixel = pixelsToAssign[pixelIndex];
			double supplyVal = supplyVector->data[supplyPixel];
			
			int likeliestDemandPixel;
			double likeliestDemandPixelScore;
			int pixelFound = 0;

			int xPos = supplyPixel % supply->width;
			int yPos = supplyPixel / supply->width;
			int xMin = MAX(xPos - searchRadius, 0);
			int xMax = MIN(xPos + searchRadius, supply->width - 1);
			int yMin = MAX(yPos - searchRadius, 0);
			int yMax = MIN(yPos + searchRadius, supply->height - 1);

			// Find target pixels in radius
			for (int x = xMin; x <= xMax; x++) {
				for (int y = yMin; y <= yMax; y++) {
					int targetedDemandPixel = x + y * supply->width;
					if (assignedDemandPixels[targetedDemandPixel]) continue;

					double cost = (x - xPos) * (x - xPos) + (y - yPos) * (y - yPos);
					double newestScore = reglna->data[targetedDemandPixel] + reglnb->data[supplyPixel] - cost;
					// -C+reg*ln(a)+reg*ln(b) = reg * ln(P) ---- this has the same ordering of reg * ln(P) and therefore P as well (P being the gibbs val)

					if (!pixelFound || newestScore > likeliestDemandPixelScore) {
						likeliestDemandPixel = targetedDemandPixel;
						likeliestDemandPixelScore = newestScore;
						pixelFound = 1;
						anyCandidates = 1;
					}
					// add case for none found
				}
			}

			if (pixelFound == 0) {
				continue;
			} 
			// determine actual gibbsVal of likeliest demandPixel
			// if no pixels available, skip until confirmed no more possible assignments
			double likelihood = u0->data[likeliestDemandPixel] * 
								v0->data[supplyPixel] *
								gibbsVal(supply, demand, supplyPixel, likeliestDemandPixel, reg) / supplyVal;
			if (supplyVal == 0) printf("WARNING: division by supplyval=0!!!!!!!!!!!!!!!!!!!!\n");
			if (likelihood > threshold) {
				// Remove the two pixels from availability
				pixelsToAssign[pixelIndex] = pixelsToAssign[numUnassigned - 1];
				// pixelsToAssign[numUnassigned - 1] = -1;
				assignedDemandPixels[likeliestDemandPixel] = 1;
				pixelIndex--;

				//assign
				assignments[totalPixels - numUnassigned].x = supplyPixel;
				assignments[totalPixels - numUnassigned].y = likeliestDemandPixel;
				//printf("%lf/%lf Assigned %d->%d\n", likelihood, threshold, supplyPixel, likeliestDemandPixel);

				numUnassigned--;
				// determines later if all pixels are mutually unassigned
			} else {
				// printf("%lf < %lf\n", likelihood, threshold);
			}
		}	

		if (!anyCandidates) {
			printf("WARNING: No assignments found!!!!!!!!!!!!!!! (should be impossible)\n");
			break;
		}
	}


	// int* lastDemandPixels = malloc(sizeof(int) * numUnassigned);

	// int unassignedDemandPixelsFound = 0;
	// for (int i = 0; unassignedDemandPixelsFound < numUnassigned; i++) {
	// 	if (0 == assignedDemandPixels[i]) { // if demand pixel is not yet assigned then
	// 		lastDemandPixels[unassignedDemandPixelsFound] = i;
	// 		unassignedDemandPixelsFound++;
	// 	}
	// }

	// for (int i = 0; i < numUnassigned; i++) {
	// 	assignments[totalPixels - numUnassigned].x = pixelsToAssign[i];
	// 	assignments[totalPixels - numUnassigned].y = lastDemandPixels[i];
	// 	printf("Last resort %d->%d\n", assignments[totalPixels - numUnassigned].x, assignments[totalPixels - numUnassigned].y);
	// }

	// // int pixelIndex = numUnassigned - 1;
	// // while (pixelIndex >= 0) { // just assigns the unassigned pixels to closest neighbor
	// // 	int supplyPixel = pixelsToAssign[pixelIndex];
	// // 	int sxPos = supplyPixel % supply->width;
	// // 	int syPos = supplyPixel / supply->width;

	// // 	int closestPixelIndex;
	// // 	int closestDist = supply->width+supply->height; // guaranteed to be farther than the farthest remaining pixel

	// // 	for (int demandIndex = 0; demandIndex < numUnassigned; demandIndex++) {
	// // 		int demandPixel = lastDemandPixels[demandIndex];
	// // 		int dxPos = demandPixel % supply->width;;
	// // 		int dyPos = demandPixel % supply->width;

	// // 		double sqdist = (sxPos - dxPos) * (sxPos - dxPos) + (syPos - dyPos) * (syPos - dyPos);

	// // 		if (sqdist < closestDist) {
	// // 			closestPixelIndex = demandIndex;
	// // 			closestDist = sqdist;
	// // 		}
	// // 	}

	// // 	//remove then assign
	// // 	lastDemandPixels[closestPixelIndex] = lastDemandPixels[numUnassigned - 1]; 

	// // 	assignments[totalPixels - numUnassigned].x = supplyPixel;
	// // 	assignments[totalPixels - numUnassigned].y = lastDemandPixels[closestPixelIndex];
	// // 	printf("Last resort %d->%d\n", supplyPixel, lastDemandPixels[closestPixelIndex]);

		
	// // 	numUnassigned--;
	// // 	pixelIndex--;
	// // }
	
	// // Final image creation
	struct image* output = resizeImage(supply, supply);
	double* newData = (double*)calloc(totalPixels, sizeof(double) * output->bytesPerPixel);
	int bytesPerPixel = output->bytesPerPixel;
	double* data = output->data;

	for (int i = 0; i < totalPixels; i++) {
		struct vector2 assignment = assignments[i];
		int originalIndex = assignment.x;
		int newIndex = assignment.y;
		// printf("%d->%d\n", originalIndex, newIndex);

		u8* originalIndexPtr = (u8*) data + originalIndex * bytesPerPixel; 
		u8* newIndexPtr = (u8*) newData + newIndex * bytesPerPixel;

		for (int byte = 0; byte < bytesPerPixel; byte++) {
			// printf("%d/", originalIndexPtr[byte]);
			newIndexPtr[byte] = originalIndexPtr[byte];
		}
		// printf("\n");
	}

	output->data = newData;
	
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
	u0->data = (double*)(malloc(sizeof(double) * (u0->n)));
	v0->data = (double*)(malloc(sizeof(double) * (v0->n)));
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
		// printf("\n\n");
		for(int i=0; i<v0->n; i++){
			double val = 0.0;
			for(int j=0; j<v0->n; j++){
				val += gibbsVal(supply, demand, j, i, reg)*u0->data[j];
			}
			if(val > 1e-7){
				v0->data[i] = demandVector->data[i] / (val);
				//v0->data[i] = 1.0 / (val);
			}
		}
		//printf("\nRows scaled\n");
		//printMatrixInfo(supply, demand, u0, v0, reg);
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
				//u0->data[i] = 1.0 / (val);
			}

			double diff = temp - u0->data[i];
			error += (diff > 0) ? diff : -diff; 
		}
		//printf("\n\nColumns scaled\n");
		//printMatrixInfo(supply, demand, u0, v0, reg);
		
		dError = error-pError;
		if(fabs(dError) < 1e-5){
			c--;
		}

		if(flags & (MAKE_GIF_FLAG | RECURSIVE_IMAGE_FLAG)){
			struct image* prog;
			if(flags & USE_GREEDY_ALGORITHM) {
				prog = discreteCreateImage(supply, demand, u0, v0, reg, supplyVector, demandVector, flags);
			} else {
				prog = createImageCPU(supply, demand, u0, v0, reg, supplyVector, flags);
			}

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
				//demandVector = imgToStochVec(demand, NULL);
			}
		}
		iter++;
	}
	
	//printf("In: \n");
	//printVector(supplyVector);
	//printVector(demandVector);

	//printVector(u0);
	//printVector(v0);
	if(flags & USE_GREEDY_ALGORITHM) {
		return discreteCreateImage(supply, demand, u0, v0, reg, supplyVector, demandVector, flags);
	} else {
		return createImageCPU(supply, demand, u0, v0, reg, supplyVector, flags);
	}
}


struct image* createImage(struct image* supply, struct image* demand, struct vectorN* u0, struct vectorN* v0, double reg, struct vectorN* supplyVector, u8 flags){
	struct image* output = malloc(sizeof(struct image));
	output->width = supply->width; 
	output->height = supply->height; 
	output->bytesPerPixel = supply->bytesPerPixel;
	
	double* buffer_demand; 
	double* buffer_supply;

	cu_createArr(&output->data, sizeof(u8)*output->width*output->height*output->bytesPerPixel);
	cu_createArr((void**)&buffer_demand, output->bytesPerPixel * sizeof(double) * output->width * output->height);
	cu_createArr((void**)&buffer_supply, output->bytesPerPixel * sizeof(double) * output->width * output->height);
	

	cu_naiveImageCreation(u0->data, v0->data, buffer_supply, buffer_demand, supply->data, demand->data, reg, u0->n, supply->width, supply->bytesPerPixel, supplyVector->data);
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
	*/
	printf("\n");
	printf("total supply mass: %f\n", sum_supply); //everything breaks without this print statement. I don't know why.,, 
	printf("total output mass post-calc: %f\n", sum_demand);
	
	
	if(flags & PRINT_TRANSPORT_PLAN){
		printVector(u0);
		printVector(v0);
		for(int i=0; i<u0->n; i++){
			if(supplyVector->data[i] > 0){
				for(int j=0; j<v0->n; j++){
					double val = gibbsVal(supply, demand, i, j, reg)*u0->data[i]*v0->data[j];// supplyVector->data[i];
					if(val > 0){
						printf(" %f ", val);
					}else{
						printf(" ");
					}
				}
			} 
			printf("\n");
		}
	}

	u32 sum_output = 0;
	for(int i=0; i<output->bytesPerPixel * output->width * output->height; i++){
		double pixelVal=round(buffer_demand[i]+buffer_supply[i]);
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
	
	if(flags & MAKE_GIF_FLAG){
		writeImage(supply, "output/gif/0000.png");
	}
	while(iter < maxIter){ 
		callSinkhorn(u0->data, v0->data, supplyVector->data, demandVector->data, u0->n, supply->width, supply->height, reg);
		printf("Iteration %d\n", iter);
		iter++;
		if(flags & (MAKE_GIF_FLAG | RECURSIVE_IMAGE_FLAG) && iter%1 == 0){
			struct image* prog;
			if(flags & USE_GREEDY_ALGORITHM){
				prog = discreteCreateImage(supply, demand, u0, v0, reg, supplyVector, demandVector, flags);
			}else{
				prog = createImage(supply, demand, u0, v0, reg, supplyVector, flags);
			}
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
				supplyVector = imgToStochVec(supply, buf1); //infinite mass duping is so back...
				//demandVector = imgToStochVec(demand, buf2);
			}
		}
	}
	//printVector(u0);
	//printVector(v0);
	// return createImage(supply, demand, u0, v0, reg, supplyVector, flags);
	printf("%d", flags & USE_GREEDY_ALGORITHM);
	if(flags & USE_GREEDY_ALGORITHM) {
		return discreteCreateImage(supply, demand, u0, v0, reg, supplyVector, demandVector, flags);
	} else {
		return createImage(supply, demand, u0, v0, reg, supplyVector, flags);
	}
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
