#include <stdlib.h>
#include <stdio.h>


#include <types.h>
#include "image.h"
#include "computation.h"




int main(int argc, char** argv){
	if(argc < 3){
		printf("Not enough arguments\n");
		exit(1);
	}

	struct image* _target = loadImage(argv[1]);
	struct image* source = loadImage(argv[2]);

	struct image* target = resizeImage(_target, source);
	


	//struct vectorN* supplyVector = imgToStochVec(source);
	//struct vectorN* demandVector = imgToStochVec(target);

	/*for(int i=0; i<supplyVector->n; i++){
		printf("%f, ", supplyVector->data[i]);
	}
	printf("\n");*/
	
	struct image* output = stinkhorn(source, target, 0.5, 1e-5);


	writeImage(target, "output/RescaledTarget.bmp");
	//writeImage(_target, "output/_Sus.bmp");
	writeImage(output, "output/Output.bmp");

	return 0;
}
