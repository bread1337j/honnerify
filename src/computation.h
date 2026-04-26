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
void stinkhorn(struct image* supply, struct image* demand, double reg, double precision);
#endif
