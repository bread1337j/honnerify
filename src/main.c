#include <stdlib.h>
#include <stdio.h>


#include <types.h>
#include "image.h"
#include "sinkhorn.h"

#include <string.h>
#include <unistd.h>


int main(int argc, char** argv){

	struct image* _target = NULL;//loadImage(argv[1]);
	struct image* source = NULL;//loadImage(argv[2]);
	int maxIter = 100;
	double reg = 10;
	char* outputString = "output/Output.png";
	u8 flags = 0;
	for(int i=1; i<argc; i++){
		if(!strcmp(argv[i], "-t")){
			i+=1;
			if(argc <= i){
				printf("Not enough arguments for parameter %s", argv[i-1]);
				exit(2);
			}
			_target = loadImage(argv[i]);
		} else if(!strcmp(argv[i], "-s") || !strcmp(argv[i], "-src")){
			i+=1;
			if(argc <= i){
				printf("Not enough arguments for parameter %s", argv[i-1]);
				exit(2);
			}
			source = loadImage(argv[i]);
		} else if(!strcmp(argv[i], "-i") || !strcmp(argv[i], "-iter")){
			i+=1;
			if(argc <= i){
				printf("Not enough arguments for parameter %s", argv[i-1]);
				exit(2);
			}
			sscanf(argv[i], "%d", &maxIter);
		} else if(!strcmp(argv[i], "-r") || !strcmp(argv[i], "-reg")){
			i+=1;
			if(argc <= i){
				printf("Not enough arguments for parameter %s", argv[i-1]);
				exit(2);
			}
			sscanf(argv[i], "%lf", &reg);
		} else if(!strcmp(argv[i], "-g") || !strcmp(argv[i], "-gif")){
			flags |= MAKE_GIF_FLAG;
		} else if(!strcmp(argv[i], "-c") || !strcmp(argv[i], "-cuda")){
			flags |= CUDA_BACKEND_FLAG;
		}else if(!strcmp(argv[i], "-R") || !strcmp(argv[i], "-Recursive")){
			flags |= RECURSIVE_IMAGE_FLAG;
		}else if(!strcmp(argv[i], "-v")){
			flags |= PRINT_TRANSPORT_PLAN;
		} else {
			printf("Unknown argument %s\n", argv[i]);
			exit(2);
		}
	}
	
	if(_target == NULL){
		printf("No target image specified!\n");
		exit(2);
	}
	if(source == NULL){
		printf("No source image specified!\n");
		exit(2);
	}


	if(_target->bytesPerPixel != source->bytesPerPixel){
		printf("\n!\n!");
		printf("Bytes per pixel don't match, the output will be straight NONSENSE\n");
		printf("\n!\n!");
		sleep(1); 
	}

	struct image* target = resizeImage(_target, source);
	


	//struct vectorN* supplyVector = imgToStochVec(source);
	//struct vectorN* demandVector = imgToStochVec(target);

	/*for(int i=0; i<supplyVector->n; i++){
		printf("%f, ", supplyVector->data[i]);
	}
	printf("\n");*/
	
	writeImage(source, "output/Source.png");
	struct image* output = sinkhorn(source, target, reg, 1e-5, maxIter, flags);

	writeImage(target, "output/RescaledTarget.png");
	//writeImage(_target, "output/_Sus.bmp");
	writeImage(output, outputString);

	return 0;
}
