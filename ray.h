#ifndef RAY_H_
#define RAY_H_

typedef struct v {float x,  y, z;} v;
typedef v bin (v, v);
typedef float cov (v);
typedef float cov2 (v, v);
typedef struct t {v e1, e2, e3, a;} t;

bin sub;
bin add;

cov2 dot;
cov vlen;
cov2 dist;

typedef struct renderable {
  void *data;
  float (* sdf) (struct renderable, v);
  void (* destroy) (struct renderable);
} renderable;

void superDestroy(renderable r);


renderable newRenderable();

float blankSDF(renderable r, v x) {
  return 100000;
}


typedef struct spheredat {
  v center;
  float r;
} spheredat;

typedef struct torusdat {
  v center;
  v axis;
  float br;
  float sr;
} torusdat;

typedef struct halfdat {
  v center;
  v normal;
} halfdat;

typedef struct td {
  renderable r;
  t trans;
}td;

typedef struct rod {
  v center;
  v normal;
  float r;
} rod;

float sdfRod(renderable r, v x);


void destroyTransform(renderable r);

float sdfTransform(renderable r, v x);

renderable transformRenderable(renderable r, t a);

float sdfSphere(renderable sphere, v x);

float sdfHalf(renderable sphere, v x);

renderable newHalf(v half, v normal);

renderable newSphere(v center, float r);

void destroySphere (struct renderable);

float sdfTorus(renderable torus, v x);

renderable newTorus(v center, v axis, float bigr, float smallr);

typedef struct cpr {renderable a, b;} cpr;

renderable cup(renderable, renderable);

float sdfCup(renderable, v);

float sdfCap(renderable, v);


renderable cap(renderable, renderable);

void render(renderable a, v *lightc, int lightv);

#endif //  RAY_H_
