/*
 *   routines to render a fractal landscape as an image
 */
#include <math.h>
#include <stdio.h>
#include "paint.h"
#include "crinkle.h"
#include "global.h"

char artist_Id[] = "$Id: artist.c,v 1.3 1993/02/18 15:36:13 spb Exp $";
#define SIDE 1.0
#define PI 3.14159265

/*{{{  void set_clut() */
void set_clut()
{
  int band,shade;
  float ambient = 0.1;  
  float top, bot;
  float intensity;
  int tmp;
  int i;
  float rb[N_BANDS] = { 0.167,0.200,0.333,0.450,0.600,1.000 };
  float gb[N_BANDS] = { 0.667,0.667,0.500,0.500,0.600,1.000 };
  float bb[N_BANDS] = { 0.500,0.450,0.333,0.200,0.000,1.000 };
#if MAX_COL > 255
Error Error Error max_col too large
#endif
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
    for(shade=0 ; shade < BAND_SIZE ; shade++)
    {
      /*{{{  set red */
      top = rb[band];
      bot = ambient * top;
      intensity = bot + ((shade * (top - bot))/BAND_SIZE);
      tmp = 255 * intensity;
      if (tmp < 0)
      {
        fprintf(stderr,"set_clut: internal error: invalid code %d\n",tmp);
        exit(2);
      }
      if( tmp > 255 )
      {
        tmp = 255;
      }
      red[BAND_BASE + (band*BAND_SIZE) + shade] = tmp;
      /*}}}*/
      /*{{{  set green */
      top = gb[band];
      bot = ambient * top;
      intensity = bot + ((shade * (top - bot))/BAND_SIZE);
      tmp = 255 * intensity;
      if (tmp < 0)
      {
        fprintf(stderr,"set_clut: internal error: invalid code %d\n",tmp);
        exit(2);
      }
      if( tmp > 255 )
      {
        tmp = 255;
      }
      green[BAND_BASE + (band*BAND_SIZE) + shade] = tmp;
      /*}}}*/
      /*{{{  set blue */
      top = bb[band];
      bot = ambient * top;
      intensity = bot + ((shade * (top - bot))/BAND_SIZE);
      tmp = 255 * intensity;
      if (tmp < 0)
      {
        fprintf(stderr,"set_clut: internal error: invalid code %d\n",tmp);
        exit(2);
      }
      if( tmp > 255 )
      {
        tmp = 255;
      }
      blue[BAND_BASE + (band*BAND_SIZE) + shade] = tmp;
      /*}}}*/
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
  varience = varience * 0.1;
}
/*}}}*/
/*{{{  Col get_col(Height p, Height p_minus_x, Height p_plus_y, Height shadow) */
Col get_col(Height p, Height p_minus_x, Height p_plus_y, Height shadow)
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
  delta_x = p - p_minus_x;
  delta_y = p_plus_y - p;
  /*{{{  calculate effective height */
  effective = (p + (varience * contour *
          ((SIDE *SIDE)/ ((SIDE*SIDE) +
          (delta_x*delta_x) +
          (delta_y*delta_y)))));
  /*}}}*/
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
  /*{{{  calculate cosine */
  cos_theta = ((delta_x * cos_phi) + (SIDE * sin_phi))/
              sqrt( SIDE + (delta_x*delta_x) + (delta_y*delta_y) );
  /*}}}*/
  /*{{{  calculate shading */
  shade = ((double) contrast * cos_theta) * (double) BAND_SIZE;
  if( shade > (BAND_SIZE-1))
  {
    shade = (BAND_SIZE-1);
  }
  /*{{{  if cos_theta is negative then point is really in shadow */
  if( shade < 0 )
  {
    fprintf(stderr,"warning bad angle\n");
    shade = 0;
  }
  /*}}}*/
  /*}}}*/
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
    res[i] = get_col(b[i],a[i],b[i-1],shadow[i]);
  }
  return(res);
}
  
/*}}}*/
