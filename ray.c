#include <stdio.h>
#include <unistd.h>
#include <curses.h>
#include <stdlib.h>
#include <math.h>
#include "ray.h"
#define s3 .5774
#define EPS 1e-4

char* legend = " .:-=+*#%@";
int w = 90;
int h = 45;
int chars = 10;
int rounds = 40;

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

v mult(t a, v x) {
  return add(add(scale(x.x, a.e1), scale(x.y, a.e2)), add(scale(x.z, a.e3), a.a));
}

v vmult(t a, v x) {
  return add(add(scale(x.x, a.e1), scale(x.y, a.e2)), scale(x.z, a.e3));
}

t tmult(t a, t b) {
  t r = { vmult(a, b.e1), vmult(a, b.e2), vmult(a, b.e3), mult(a, b.a) };
  return r;
}

float dot(v x, v y) {
  return x.x*y.x + x.y*y.y + x.z*y.z;
}

v light = {s3, s3, s3};

float max(float a, float b) {
  return a > b ? a : b;
}

float min(float a, float b) {
  return -max(-a, -b);
}

v proj(v a, v b) {
  return scale(dot(a, b), b);
}

float torus(v c, float br, float sr, v axis, v vec) {
  v relPos = sub(vec, c);
  v axisProj = proj(relPos, axis);
  v planeProj = sub(relPos, axisProj);
  v circleProj = scale(br, vnormalize(planeProj));
  return dist(circleProj, relPos) - sr;
}

float sdfTorus(renderable torus, v x) {
  torusdat *t = (torusdat *)torus.data;
  v relPos = sub(x, t->center);
  v axisProj = proj(relPos, t->axis);
  v planeProj = sub(relPos, axisProj);
  v circleProj = scale(t->br, vnormalize(planeProj));
  return dist(circleProj, relPos) - t->sr;
}

renderable newTorus(v center, v axis, float br, float sr) {
  torusdat *sd = (torusdat *) malloc(sizeof(torusdat));
  sd->center = center;
  sd->axis = axis;
  sd->br = br;
  sd->sr = sr;
  renderable ret = { sd, &sdfTorus };
  return ret;
}

v sdfNorm(renderable rend, v x) {
  v dx = {x.x + EPS, x.y, x.z};
  v dy = {x.x, x.y + EPS, x.z};
  v dz = {x.x, x.y, x.z + EPS};
  float sdfx = rend.sdf(rend, x);
  v r = {(rend.sdf(rend, dx)-sdfx)/EPS,(rend.sdf(rend, dy)-sdfx)/EPS, (rend.sdf(rend, dz)-sdfx)/EPS};
  return vnormalize(r);
}

float sdfSphere(renderable sphere, v x) {
  spheredat *sd = (spheredat *) sphere.data;
  return dist(sd->center, x) - sd->r;
}

renderable newSphere(v center, float r) {
  spheredat *sd = (spheredat *) malloc(sizeof(spheredat));
  sd->center = center;
  sd->r = r;
  renderable ret = { sd, &sdfSphere };
  return ret;
}

void destroySphere(renderable s) {
  free(s.data);
}

float sdfCup(renderable r, v x) {
  cpr *c = (cpr *) r.data;
  return min(c->a.sdf(c->a, x), c->b.sdf(c->b, x));
}

renderable cup(renderable a, renderable b) {
  cpr *c = (cpr *) malloc(sizeof(cpr));
  c->a = a;
  c->b = b;
  renderable ret = { c, &sdfCup };
  return ret;
}

float sdfCap(renderable r, v x) {
  cpr *c = (cpr *) r.data;
  return max(c->a.sdf(c->a, x), c->b.sdf(c->b, x));
}

renderable cap(renderable a, renderable b) {
  cpr *c = (cpr *) malloc(sizeof(cpr));
  c->a = a;
  c->b = b;
  renderable ret = { c, &sdfCap };
  return ret;
}

void render(renderable a, v* lightc, int lightv) {
  int row, col;
  getmaxyx(stdscr, row, col);
  float *buffer = calloc(row*col, sizeof(float));
  for (int li = 0; li < lightv; li++) {
    float ratio = 0.5* (float) col / (float) row;
    for(int i = 0; i < row; i ++) {
      for(int j = 0; j < col; j++) {
        float x = ratio * (2.0 * (float)j / (float)col - 1.0);
        float y = 2.0 * (float)i / (float)row - 1.0;
        v pos = {x, y, 1};
        v d = vnormalize(pos);
        for(int ii = 0; ii < rounds; ii++) {
          float curSDF = a.sdf(a, pos);
          if(curSDF < EPS) {
            *(buffer + i*col + j) += min(.9, max(0.1, -dot(lightc[li], sdfNorm(a, pos))));
            break;
          }
          pos = add(scale(curSDF, d), pos);
        }
      }
    }
  }
  for(int i = 0; i < row; i ++) {
    for(int j = 0; j < col; j++) {
      mvaddch(i, j, legend[(int)((*(buffer + i*col + j))*10.0)]);
    }
  }
}

v vec(float x, float y, float z) {
  v r = {x, y, z};
  return r;
}

t mat(float t11, float t12, float t13, float t14,
      float t21, float t22, float t23, float t24,
      float t31, float t32, float t33, float t34) {
  t r = {vec(t11, t21, t31), vec(t12, t22, t32), vec(t13, t23, t33), vec(t14, t24, t34)};
  return r;
}

t ident() {
  return mat(1, 0, 0, 0,
             0, 1, 0, 0,
             0, 0, 1, 0);
}

t trans(v tr) {
  return mat(1, 0, 0, tr.x,
             0, 1, 0, tr.y,
             0, 0, 1, tr.z); 
}

t rotx(float theta) {
  return mat(1, 0, 0, 0,
             0, cos(theta), -sin(theta), 0,
             0, sin(theta),  cos(theta), 0); 
}
t roty(float theta) {
  return mat(cos(theta), 0, -sin(theta), 0,
             0, 1, 0, 0,
             sin(theta), 0,  cos(theta), 0); 
}
float sdfTransform(renderable r, v x) {
  td * tt = (td *)r.data;
  return tt->r.sdf(tt->r, mult(tt->trans, x));
}
t rotz(float theta) {
  return mat(cos(theta), -sin(theta), 0, 0,
             sin(theta), cos(theta), 0, 0,
             0, 0, 1, 0); 
}

void  dv(char* ret, v x) {
   sprintf(ret, "<%.2f, %.2f, %.2f>", x.x, x.y, x.z);
 }

char *dt(char *ret, t x) {
   char yee1[80];
   char yee2[80];
   char yee3[80];
   char yee4[80];

   dv(yee1, x.e1);
   dv(yee2, x.e2);
   dv(yee3, x.e3);
   dv(yee4, x.a);
   sprintf(ret, "[%s %s %s %s]", yee1, yee2, yee3, yee4);

   return ret;
}

renderable transformRenderable(renderable r, t a) {
  td *d = (td *) malloc(sizeof(td));
  d->r = r;
  d->trans = a;
  renderable ret = { d, &sdfTransform };
  return ret;
}

int main() {
  initscr();
  cbreak();
  keypad(stdscr, TRUE);
  noecho();
  v center1 = {.5, 0, 0};
  v center = {-.5, 0, 0};
  float r = 1;
  v l1 = {-s3/2, s3/2, s3/2};
  v l2 = {s3/2, s3/2, s3/2};
  v lights[] = {l1, l2};
  renderable s = newSphere(center, r);
  renderable s1 = newSphere(center1, r);
  renderable t1 = newTorus(vec(0, 0,0), vec(0,0,1), 1.5, .5);
  renderable c = cup(t1, cap(s, s1));
  renderable final = transformRenderable(c, trans(vec(0, 0, -3)));
  td *tt = (td *) final.data;
  while (1) {
    render(final, lights, 2);
    char yee[80];
    dt(yee, tt->trans);
    mvprintw(0,0, yee);
    refresh();
    int ch = getch();
    mvprintw(0,0,"%d %c", ch, ch);
    refresh();
    switch(ch) {
      case KEY_UP:
        tt->trans = tmult( tt->trans,rotx(0.05));
        break;
      case KEY_DOWN:
        tt->trans = tmult( tt->trans,rotx(-0.05));
        break;
      case KEY_LEFT:
        tt->trans = tmult( tt->trans,roty(0.05));
        break;
      case KEY_RIGHT:
        tt->trans = tmult( tt->trans,roty(-0.05));
        break;
      case 'w':
        tt->trans = tmult( tt->trans,trans(vec(0,0,0.1)));
        break;
      case 'a':
        tt->trans = tmult( tt->trans,trans(vec(-.1,0,0)));
        break;
      case 's':
        tt->trans = tmult( tt->trans,trans(vec(0,0,-0.1)));
        break;
      case 'd':
        tt->trans = tmult( tt->trans,trans(vec(.1,0,0)));
        break;
      default:
        break;

    }
  }
  destroySphere(s1);
  destroySphere(s);
  free(t1.data);
  free(c.data);
}
