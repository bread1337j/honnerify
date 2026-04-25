#ifndef IMAGE_H 
#define IMAGE_H
#include <types.h>
#include <stb_image.h>
#include <stb_image_resize2.h>
#include <stb_image_write.h>
struct image {
	i32 width;
	i32 height;
	i32 bytesPerPixel;
	void* data;
};
struct image* loadImage(char* path);
struct image* resizeImage(struct image* orig, struct image* target);
void writeImage(struct image* img, char* path);
#endif
