#include <stdio.h>
#include <unistd.h>
#include <math.h>
#define s3 .5774
#define EPS 1e-3
char* legend = " .:-=+*#%@";
int w = 180;
int h = 90;
int chars = 10;
int rounds = 12;
typedef struct v {float x,  y, z;} v;

float counter = 0;

float vlen(v vec) {
  return sqrtf(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
}

v vnormalize(v x) {
  float n = vlen(x);
  v ret =  {x.x/n, x.y/n, x.z/n};
  return ret;
}

v sub (v x, v y) {
  v ret = {x.x - y.x, x.y - y.y, x.z - y.z};
  return ret;
}

v add (v x, v y) {
  v ret = {x.x + y.x, x.y + y.y, x.z + y.z};
  return ret;
}

float dist(v a, v b) {
  return vlen(sub(a, b));
}

v scale(float s, v x){
  v r = {s*x.x, s*x.y, s*x.z};
  return r;
}

float dot(v x, v y) {
  return x.x*y.x + x.y*y.y + x.z*y.z;
}

v light = {s3, s3, s3};

float max(float a, float b) {
  return a > b ? a : b;
}

v proj(v a, v b) {
  return scale(dot(a, b)/dot(b, b), b);
}

float torus(v c, float br, float sr, v axis, v vec) {
  v relPos = sub(vec, c);
  v axisProj = proj(relPos, axis);
  v planeProj = sub(relPos, axisProj);
  v circleProj = scale(br, vnormalize(planeProj));
  return dist(circleProj, relPos) - sr;
}

float sdf(v vec) {
  v c = {0, 0, 3};
  v ax = {sin(1.521*counter)*sin(2.134*counter), sin(1.521*counter)*cos(2.134*counter), cos(1.121*counter)};
  return torus(c, 1, 0.6, ax, vec);
}

v faceNorm(v x) {
  v dx = {x.x + EPS, x.y, x.z};
  v dy = {x.x, x.y + EPS, x.z};
  v dz = {x.x, x.y, x.z + EPS};
  v r = {(sdf(dx)-sdf(x))/EPS,(sdf(dy)-sdf(x))/EPS, (sdf(dz)-sdf(x))/EPS};
  return vnormalize(r);
}

int main() {
  while(1) {
    for(int i = 0; i < h; i ++) {
      for(int j = 0; j < w; j++) {
        float x = 2.0 * (float)j / (float)w - 1.0;
        float y = 2.0 * (float)i / (float)h - 1.0;
        v pos = {x, y, 1};
        v d = vnormalize(pos);
        float b = 0;
        for(int i = 0; i < rounds; i++) {
          if(sdf(pos) < EPS) {
            b = max(0.1, -dot(light, faceNorm(pos)));
            break;
          }
          pos = add(scale(sdf(pos), d), pos);
        }
        putchar(legend[(int)(b*10.0)]);
      }
      putchar('\n');
    }
    counter += 0.01;
    puts("\x1b[91A");
  }
}
