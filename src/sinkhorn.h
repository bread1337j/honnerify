#ifndef COMPUTATION_H
#define COMPUTATION_H


#include "image.h"
#include <math.h>

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
	u32 n;
	double* data;
};


#define MAKE_GIF_FLAG (1<<0)
#define CUDA_BACKEND_FLAG (1<<1)
#define RECURSIVE_IMAGE_FLAG (1<<2)
#define PRINT_TRANSPORT_PLAN (1<<3)


struct image* sinkhorn(struct image* supply, struct image* demand, double reg, double precision, u32 maxIter, u8 flags);
#endif
