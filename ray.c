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
int rounds = 80;

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
  return scale(dot(a, b)/dot(b, b), b);
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
  renderable ret = { sd, &sdfTorus, &superDestroy};
  return ret;
}

renderable newRod(v center, v normal, float r) {
  rod *sd = (rod *) malloc(sizeof(rod));
  sd->center = center;
  sd->normal= normal;
  sd->r = r;
  renderable ret = { sd, &sdfRod, &superDestroy};
  return ret;
}

float sdfRod(renderable torus, v x) {
  rod *t = (rod *)torus.data;
  v relpos = sub(x, t->center);
  v projpos = proj(relpos, t->normal);
  return dist(relpos, projpos) - t->r;
}

renderable newCyl(v center, v normal, float r, float l) {
  renderable rod = newRod(center, normal, r);
  renderable half1 = newHalf(center, scale(-1, normal));
  renderable half2 = newHalf(add(scale(l, normal), center), normal);
  return cap(cap(rod, half1), half2);
}

renderable newRenderable() {
 renderable r =  {NULL, &blankSDF, &superDestroy};
 return r;
}
void superDestroy(renderable r) {
  free(r.data);
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
  renderable ret = { sd, &sdfSphere, &superDestroy };
  return ret;
}

float sdfHalf(renderable half, v x) {
  halfdat *hd = (halfdat *) half.data;
  return dot(hd->normal, sub(x, hd->center)); 
}

renderable newHalf(v center, v normal) {
  halfdat *hd = (halfdat *) malloc(sizeof(halfdat));
  hd-> center = center;
  hd->normal = normal;
  renderable ret = {hd, &sdfHalf, &superDestroy };
  return ret;
}

void destroySphere(renderable s) {
  free(s.data);
}

float sdfCup(renderable r, v x) {
  cpr *c = (cpr *) r.data;
  return min(c->a.sdf(c->a, x), c->b.sdf(c->b, x));
}

void cprDestroy(renderable r) {
  cpr *c = (cpr *) r.data;
  c->a.destroy(c->a);
  c->b.destroy(c->b);
  superDestroy(r);
}

renderable cup(renderable a, renderable b) {
  cpr *c = (cpr *) malloc(sizeof(cpr));
  c->a = a;
  c->b = b;
  renderable ret = { c, &sdfCup, &cprDestroy };
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
  renderable ret = { c, &sdfCap, &cprDestroy };
  return ret;
}

void render(renderable a, v* lightc, int lightv, int shadows) {
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
            if(shadows) {
              v lightDir = vnormalize(scale(-1, lightc[li]));
              v newPos = add(pos, scale(2 * EPS, lightDir));
              int found = 0;
              for(int jj = 0; jj < rounds; jj++) {
                 float ss = a.sdf(a, newPos);
                 if(ss < EPS) {
                   found = 1;
                   break;
                 }
                 newPos = add(scale(ss, lightDir), newPos);
              }
              if(found) {
                break;
              }
            }
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
  renderable ret = { d, &sdfTransform, &destroyTransform };
  return ret;
}

void destroyTransform(renderable r) {
  td *d = (td *) r.data;
  d->r.destroy(d->r);
  superDestroy(r);
}

renderable dick(float headR, float shaftr, float length, float ballR, float ballOffset) {
  renderable headBall = newSphere(vec(0,0,0), headR);
  renderable halfHead = cap(headBall, newHalf(vec(0,0,0), vec(1, 0, 0)));
  renderable shaft = newCyl(vec(0,0,0), vec(1, 0, 0), shaftr, length - headR);
  renderable ball1 = newSphere(vec(length - headR, ballOffset, 0), ballR);
  renderable ball2 = newSphere(vec(length - headR, -ballOffset, 0), ballR);
  
  return cup(ball2, cup(ball1, cup(halfHead, shaft)));
}

renderable coolTestScene() {
  v center1 = {.5, 0, 0};
  v center = {-.5, 0, 0};
  float r = 1;
  renderable s = newSphere(center, r);
  renderable s1 = newSphere(center1, r);
  renderable t1 = newTorus(vec(0, 0,0), vec(0,0,1), 1.5, .5);
  renderable cyl = newCyl(vec(0, 0, 2), vec(0, -1, 0), 1, 2);
  renderable comp = cup(cap(s, s1), cup(t1, cyl));
  return comp;
}

int main() {
  initscr();
  cbreak();
  keypad(stdscr, TRUE);
  noecho();
  renderable comp = dick(1.2, 1, 6, 1.2, .6);
  //renderable comp = cup(newHalf(vec(0,2,0), vec(0,-1,0)), newCyl(vec(0,2,0), vec(0,-1,0), 1, 2));
  renderable final = transformRenderable(comp, trans(vec(2, 0, -3)));
  td *tt = (td *) final.data;
  v l1 = {-s3/2, s3/2, s3/2};
  v l2 = {s3/2, s3/2, s3/2};
  v lights[] = {l1, l2};
  while (1) {
    render(final, lights, 2, 1);
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
      case 'q':
        tt->trans = tmult( tt->trans,rotz(0.05));
        break;
      case 'e':
        tt->trans = tmult( tt->trans,rotz(-0.05));
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
      case ' ':
        tt->trans = tmult( tt->trans,trans(vec(0,.10,0)));
        break;
      case 'c':
        tt->trans = tmult( tt->trans,trans(vec(0,-.10,0)));
        break;
      default:
        break;

    }
  }
  final.destroy(final);
}
