#include <stdlib.h>
#include <stdio.h>

#include <types.h>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION 
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_resize2.h>
#include <stb_image_write.h>
struct image {
	i32 width;
	i32 height;
	i32 bytesPerPixel;
	void* data;
};

struct image* loadImage(char* path){
	struct image* out = (struct image*) malloc(sizeof(struct image));
	out->data=stbi_load(path, &out->width, &out->height, &out->bytesPerPixel, 3);
	// printf("Bytes per pixel: %d\n", out->bytesPerPixel);
	return out;
}

struct image* resizeImage(struct image* orig, struct image* target){
	struct image* out = (struct image*) malloc(sizeof(struct image));
	out->width = target->width, 
	out->height = target->height,
	out->bytesPerPixel = target->bytesPerPixel;
	

	out->data = (void*)stbir_resize_uint8_srgb(orig->data, orig->width, orig->height, 0,
										0, out->width, out->height, 0, STBIR_RGB);

	return out;
}

void writeImage(struct image* img, char* path){
	stbi_write_png(path, img->width, img->height, 3, img->data, img->width*img->bytesPerPixel);
}
