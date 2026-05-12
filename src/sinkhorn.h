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
	u16 x, y, z;
};
struct vector2 {
	u16 x, y;
};
struct vectorN {
	u16 n;
	double* data;
};
struct vectorN* imgToStochVec(struct image* img);


#define MAKE_GIF_FLAG (1<<0)
#define CUDA_BACKEND_FLAG (1<<1)

struct image* sinkhorn(struct image* supply, struct image* demand, double reg, double precision, u32 maxIter, u8 flags);
#endif
