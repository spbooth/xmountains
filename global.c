#include "crinkle.h"
#include "paint.h"

Fold *top;
int levels = 10;
int stop = 2;
int smooth = TRUE;
int frac_start = TRUE;
int slope = TRUE;
float fdim = 0.65;
Height start;      /* starting value for the surface */
Height mean=0.0;   /* mean value of surface */
Height varience;   /* rough estimate of the height of the range */
Height shift=0.5;    /* offset from calcalt to artist coordinates */
float stretch=0.6;   /* vertical stretch */
float contour = 0.3;
float ambient = 0.3;  /* level of ambient light */
float contrast = 1.0; /* contrast,
                       * increases or decreases effect of cosine rule */
float vfract   = 0.6; /* relative strength of vertical light relative
                        * to the main light source
                        */
float altitude = 2.5;
float distance = 4.0;
double phi=(40.0 * PI)/180.0; /* angle of the light */
double cos_phi;
double sin_phi;
double tan_phi;
Height sealevel = 0.0;
int width;        /* width of the landscape, (function of levels) */
int seed=0;       /* zero means read the clock */

/*
 * viewport parameters
 */
int height;                       /* height of the screen */
double vangle;                    /* view angle 0 == horizontal
                                   *increase to look down
                                   */
double tan_vangle;                                     
float vscale;           /* rescale physical height by this amount. */
float viewpos;        /* position of viewpoint */
float viewheight;      /* height of viewpoint */
float focal;                  /* focal length, calc to preserve aspect ratio */

Height *shadow;               /* height of the shadows */
Height *a_strip, *b_strip;    /* the two most recent strips */

