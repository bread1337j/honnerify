#include <stdlib.h>
#include <stdio.h>


#include <types.h>
#include "image.h"





int main(int argc, char** argv){
	if(argc < 3){
		printf("Not enough arguments\n");
		exit(1);
	}

	struct image* _target = loadImage(argv[1]);
	struct image* source = loadImage(argv[2]);

	struct image* target = resizeImage(_target, source);

	writeImage(target, "Sus.bmp");
	writeImage(_target, "_Sus.bmp");

	return 0;
}
