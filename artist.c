/*
 *   routines to render a fractal landscape as an image
 */
#include <math.h>
#include "paint.h"
#include "crinkle.h"

char artist_Id[] = "$Id: artist.c,v 1.1 1991/10/22 23:18:57 spb Exp $";
#define SIDE 1.0
#define PI 3.14159265
/*{{{  global (uggh) variables */
Fold *top;
int levels = 9;
int smooth = TRUE;
float fdim = 0.75;
float start=0.0;  /* starting value for the surface */
float mean=0.0;
float varience;   /* rough estimate of the height of the range */
float contour = 0.1;
float contrast = 1.0;
double phi=(45.0 * PI)/180.0; /* angle of the light */
double cos_phi;
double sin_phi;
double tan_phi;
Height sealevel = -10000.0;
int width;
unsigned char red[256] ,green[256], blue[256];

Height *shadow;               /* height of the shadows */
Height *a_strip, *b_strip;    /* the two most recent strips */
/*}}}*/

/*{{{  void set_clut() */
void set_clut()
{
  int band,shade;
  int i;
  int rb[N_BANDS] = { 0,1,1,2,2,3 };
  int gb[N_BANDS] = { 6,4,4,3,3,3 };
  int bb[N_BANDS] = { 3,3,2,2,1,0 };

  /*{{{  black */
  red[BLACK]       = 0;
  green[BLACK]     = 0;
  blue[BLACK]      = 0;
  /*}}}*/
  /*{{{  sky */
  red[SKY]         = 80;
  green[SKY]       = 80;
  blue[SKY]        = 240;
  /*}}}*/
  /*{{{  sea (lit) */
  red[SEA_LIT]     = 0;
  green[SEA_LIT]   = 0;
  blue[SEA_LIT]    = 240;
  /*}}}*/
  /*{{{  sea (unlit) */
  red[SEA_UNLIT]   = 0;
  green[SEA_UNLIT] = 0;
  blue[SEA_UNLIT]  = 120;
  /*}}}*/
  for( band=0 ; band<N_BANDS; band++)
  {
    for(shade=1 ; shade < (BAND_SIZE+1) ; shade++)
    {
      red[BAND_BASE + (band*BAND_SIZE) + shade] = rb[band] * shade;
      green[BAND_BASE + (band*BAND_SIZE) + shade] = gb[band] * shade;
      blue[BAND_BASE + (band*BAND_SIZE) + shade] = bb[band] * shade;
    }
  }
}
/*}}}*/
/*{{{  Height *extract(Strip *s) */
Height *extract(Strip *s)
{
  Height *p;
  p = s->d;
  free(s);
  return(p);
}
/*}}}*/
/*{{{  void init_artist_variables() */
void init_artist_variables()
{
  int i;
  
  width= (1 << levels)+1;
  top=make_fold(levels,smooth,SIDE,start,mean,fdim);
  shadow = (Height *) malloc(width * sizeof(Height));
  if ( shadow == NULL )
  {
    printf("malloc failed for shadow array\n");
    exit(1);
  }
  for(i=0 ; i<width ; i++)
  {
    shadow[i] = start;
  }
  a_strip = extract( next_strip(top) ); 
  b_strip = extract( next_strip(top) );
  cos_phi = cos( phi );
  sin_phi = sin( phi );
  tan_phi = tan( phi );
  varience = pow( (width * SIDE),(2.0 * fdim));
  varience = varience * 2.0;
}
/*}}}*/
/*{{{  Col get_col(Height p, Height p_plus_x, Height p_plus_y, Height shadow) */
Col get_col(Height p, Height p_plus_x, Height p_plus_y, Height shadow)
{
  Height delta_x, delta_y;
  double cos_theta;         /* angle between the normal and the ray */
  Height effective;
  Col result;
  int band, shade;

  /*{{{  if underwater */
  if ( p < sealevel )
  {
    if( shadow > sealevel )
    {
      return( SEA_UNLIT );
    }else{
      return( SEA_LIT );
    }
  }
  /*}}}*/
  delta_x = p_plus_x - p;
  delta_y = p_plus_y - p;
  effective = (p + (varience * contour *
          ((SIDE *SIDE)/ ((SIDE*SIDE) +
          (delta_x*delta_x) +
          (delta_y*delta_y)))));
          
  /*{{{  calculate colour band. */
  band = ( effective * N_BANDS ) / varience ;
  if ( band < 0 )
  {
    band = 0;
  }
  if( band > (N_BANDS - 1))
  {
    band = (N_BANDS -1);
  }
  result = (BAND_BASE + (band * BAND_SIZE));
  /*}}}*/
  /*{{{  if in shadow */
  if( p < shadow )
  {
    return(result);
  }
  /*}}}*/
  cos_theta = ((delta_x * cos_phi) + (SIDE * sin_phi))/
              sqrt( SIDE + (delta_x*delta_x) + (delta_y*delta_y) );
  shade = contrast * cos_theta * BAND_SIZE;
  if( shade > (BAND_SIZE-1))
  {
    shade = (BAND_SIZE-1);
  }
  result += shade;
  return(result);
}
/*}}}*/
/*{{{  Col *artist(Height *a, Height *b, Height *shadow) */
/* Calculates a set of colours for the a strip */
Col *artist(Height *a, Height *b, Height *shadow)
{
Col *res;
int i;

  res = (Col *) malloc(width * sizeof(Col) );
  if (res == NULL)
  {
    printf("malloc failed for colour strip\n");
    exit(1);
  }
  res[0] = BLACK;
  for(i=1 ; i<width ; i++)
  {
    res[i] = get_col(a[i],b[i],a[i-1],shadow[i]);
  }
  return(res);
}
  
/*}}}*/
