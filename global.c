#include "crinkle.h"
#include "paint.h"
#define PI 3.14159265

Fold *top;
int levels = 10;
int smooth = FALSE;
float fdim = 0.65;
float start=0.0;  /* starting value for the surface */
float mean=0.0;
float varience;   /* rough estimate of the height of the range */
float contour = 0.3;
float contrast = 1.0;
double phi=(45.0 * PI)/180.0; /* angle of the light */
double cos_phi;
double sin_phi;
double tan_phi;
Height sealevel = 0.0;
int width;
unsigned char red[256] ,green[256], blue[256];

Height *shadow;               /* height of the shadows */
Height *a_strip, *b_strip;    /* the two most recent strips */

