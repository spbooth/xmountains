#include "crinkle.h"
#include "paint.h"
#define PI 3.14159265

Fold *top;
int levels = 10;
int smooth = TRUE;
float fdim = 0.65;
float start;      /* starting value for the surface */
float mean=0.0;   /* mean value of surface */
float varience;   /* rough estimate of the height of the range */
float shift;      /* offset from calcalt to artist coordinates */
float contour = 0.3;
float contrast = 1.0;
double phi=(45.0 * PI)/180.0; /* angle of the light */
double cos_phi;
double sin_phi;
double tan_phi;
Height sealevel = 0.0;
int width;        /* width of the landscape, (function of levels) */
unsigned char red[256] ,green[256], blue[256];

/*
 * viewport parameters
 */
int height=768;   /* height of the screen */
double vangle=(30 * PI)/180.0;    /* view angle 0 == horizontal
                                     *increase to look down
                                     */
double tan_vangle;                                     
float vscale;           /* rescale physical height by this amount. */
float viewpos;        /* position of viewpoint */
float viewheight;      /* height of viewpoint */
float focal;                  /* focal length, calc to preserve aspect ratio */

Height *shadow;               /* height of the shadows */
Height *a_strip, *b_strip;    /* the two most recent strips */

