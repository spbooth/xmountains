/*
 *   routines to render a fractal landscape as an image
 */
#include <math.h>
#include <stdio.h>
#include "paint.h"
#include "crinkle.h"
#include "global.h"

char artist_Id[] = "$Id: artist.c,v 1.26 1994/04/04 19:27:55 spb Exp $";
#define SIDE 1.0
#ifndef PI
#define PI 3.14159265
#endif

float vstrength; /* strength of vertical light source */
float lstrength; /* strength of vertical light source */

/*{{{  void set_clut(Gun *red, Gun *green, Gun *blue)*/
/*
 * setup the colour lookup table
 */
void set_clut (max_col,red,green,blue)
int max_col;
Gun *red;
Gun *green;
Gun *blue;
{
  int band,shade;
  float top, bot;
  float intensity;
  int tmp;
  int i;
/*
*  float rb[N_BANDS] = { 0.167,0.200,0.333,0.450,0.600,1.000 };
*  float gb[N_BANDS] = { 0.667,0.667,0.500,0.500,0.600,1.000 };
*  float bb[N_BANDS] = { 0.500,0.450,0.333,0.200,0.000,1.000 };
*/

  float rb[N_BANDS];
  float gb[N_BANDS];
  float bb[N_BANDS];

  /* band base colours as RGB fractions */
  rb[0] = 0.450; rb[1] = 0.600; rb[2] = 1.000;
  gb[0] = 0.500; gb[1] = 0.600; gb[2] = 1.000;
  bb[0] = 0.333; bb[1] = 0.000; bb[2] = 1.000;

  /*{{{  black */
  red[BLACK]       = 0;
  green[BLACK]     = 0;
  blue[BLACK]      = 0;
  /*}}}*/
  /*{{{  white */
  red[WHITE]       = COL_RANGE;
  green[WHITE]     = COL_RANGE;
  blue[WHITE]      = COL_RANGE;
  /*}}}*/
  /*{{{  sky */
  red[SKY]         = 0.404*COL_RANGE;
  green[SKY]       = 0.588*COL_RANGE;
  blue[SKY]        = COL_RANGE;
  /*}}}*/
  /*{{{  sea (lit) */
  red[SEA_LIT]     = 0;
  green[SEA_LIT]   = 0.500*COL_RANGE;
  blue[SEA_LIT]    = 0.700*COL_RANGE;
  /*}}}*/
  /*{{{  sea (unlit)*/
  red[SEA_UNLIT]   = 0;
  green[SEA_UNLIT] = ((ambient+(vfract/(1.0+vfract)))*0.500)*COL_RANGE;
  blue[SEA_UNLIT]  = ((ambient+(vfract/(1.0+vfract)))*0.700)*COL_RANGE;
  /*}}}*/
  for( band=0 ; band<N_BANDS; band++)
  {
    for(shade=0 ; shade < band_size ; shade++)
    {
      if( (BAND_BASE + (band*band_size) + shade) >= max_col )
      {
        fprintf(stderr,"INTERNAL ERROR, overflowed clut\n");
        exit(1);
      }
      /*{{{  set red */
      top = rb[band];
      bot = ambient * top;
      intensity = bot + ((shade * (top - bot))/(band_size-1));
      tmp = COL_RANGE * intensity;
      if (tmp < 0)
      {
        fprintf(stderr,"set_clut: internal error: invalid code %d\n",tmp);
        exit(2);
      }
      if( tmp > COL_RANGE )
      {
        tmp = COL_RANGE;
      }
      red[BAND_BASE + (band*band_size) + shade] = tmp;
      /*}}}*/
      /*{{{  set green */
      top = gb[band];
      bot = ambient * top;
      intensity = bot + ((shade * (top - bot))/(band_size-1));
      tmp = COL_RANGE * intensity;
      if (tmp < 0)
      {
        fprintf(stderr,"set_clut: internal error: invalid code %d\n",tmp);
        exit(2);
      }
      if( tmp > COL_RANGE )
      {
        tmp = COL_RANGE;
      }
      green[BAND_BASE + (band*band_size) + shade] = tmp;
      /*}}}*/
      /*{{{  set blue */
      top = bb[band];
      bot = ambient * top;
      intensity = bot + ((shade * (top - bot))/(band_size-1));
      tmp = COL_RANGE * intensity;
      if (tmp < 0)
      {
        fprintf(stderr,"set_clut: internal error: invalid code %d\n",tmp);
        exit(2);
      }
      if( tmp > COL_RANGE )
      {
        tmp = COL_RANGE;
      }
      blue[BAND_BASE + (band*band_size) + shade] = tmp;
      /*}}}*/
    }
  }
}
/*}}}*/
/*{{{  Height *extract(Strip *s) */
/*
 * extract the table of heights from the Strip struct
 * and discard the rest of the struct.
 */
Height *extract (s)
Strip *s;
{
  int i;
  
  Height *p;
  p = s->d;
  free(s);
  for(i=0 ; i<width; i++ )
  {
    p[i] = shift + (vscale * p[i]);
  }
  return(p);
}
/*}}}*/
/*{{{  void init_artist_variables() */
/*
 * initialise the variables for the artist routines.
 */
void init_artist_variables()
{
  int i;
  float dh, dd;
  int pwidth;  /* longest lengthscale for update */
  
  width= (1 << levels)+1;
  pwidth= (1 << (levels - stop))+1;

  /* make the fractal SIDE wide, this makes it easy to predict the
   * average height returned by calcalt. If we have stop != 0 then
   * make the largest update length = SIDE
   */
  cos_phi = cos( phi );
  sin_phi = sin( phi );
  tan_phi = tan( phi );

  x_fact = cos_phi* cos(alpha);
  y_fact = cos_phi* sin(alpha);
  vscale = stretch * pwidth;  /* have approx same height as fractal width
                               * this makes each pixel SIDE=1.0 wide.
                               * c.f. get_col
                               */

  delta_shadow = tan_phi /cos(alpha);
  shadow_slip = tan(alpha);
  /* guess the average height of the fractal */
  varience = pow( SIDE ,(2.0 * fdim));
  varience = vscale * varience ;
  shift = shift * varience;
  varience = varience + shift;

  start = (sealevel - shift) / vscale ; /* always start at sealevel */ 

  /* set the position of the view point */
  viewheight = altitude * width;
  viewpos = - distance * width;

  /* set viewing angle and focal length (vertical-magnification)
   * try mapping the bottom of the fractal to the bottom of the
   * screen. Try to get points in the middle of the fractal
   * to be 1 pixel high
   */
  dh = viewheight;
  dd = (width / 2.0) - viewpos;
  focal = sqrt( (dd*dd) + (dh*dh) );
#ifndef SLOPPY
  tan_vangle = (double) ((double)(viewheight-sealevel)/(double) - viewpos);
  vangle = atan ( tan_vangle );
  vangle -= atan( (double) (height/2) / focal ); 
#else
  /* we are making some horrible approximations to avoid trig funtions */
  tan_vangle = (double) ((double)(viewheight-sealevel)/(double) - viewpos);
  tan_vangle = tan_vangle - ( (double) (height/2) / focal );
#endif

  top=make_fold(levels,stop,frac_start,slope,smooth,(SIDE / pwidth),start,mean,fdim);

  /* use first set of heights to set shadow value */
  shadow = extract(next_strip(top));
  a_strip = extract( next_strip(top) ); 
  b_strip = extract( next_strip(top) );

  /* initialise the light strengths */
  vstrength = vfract * contrast /( 1.0 + vfract );
  lstrength = contrast /( 1.0 + vfract );
}
/*}}}*/
/*{{{  Col get_col(Height p, Height p_minus_x, Height p_minus_y, Height shadow) */
/*
 * calculate the colour of a point.
 */
Col get_col (p,p_minus_x,p_minus_y,shadow)
Height p;
Height p_minus_x;
Height p_minus_y;
Height shadow;
{
  Height delta_x, delta_y;
  Height delta_x_sqr, delta_y_sqr;
  Height hypot_sqr;
  
  double norm, dshade;
  Height effective;
  Col result;
  int band, shade;
  /*{{{  if underwater*/
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
  /*
   * We have three light sources, one slanting in from the left
   * one directly from above and an ambient light.
   * For the directional sources illumination is proportional to the
   * cosine between the normal to the surface and the light.
   *
   * The surface contains two vectors
   * ( 1, 0, delta_x )
   * ( 0, 1, delta_y )
   *
   * The normal therefore is parallel to
   * (  -delta_x, -delta_y, 1)/sqrt( 1 + delta_x^2 + delta_y^2)
   *
   * For light parallel to ( cos_phi, 0, -sin_phi) the cosine is
   *        (cos_phi*delta_x + sin_phi)/sqrt( 1 + delta_x^2 + delta_y^2)
   *
   * For light parallel to ( cos_phi*cos_alpha, cos_phi*sin_alpha, -sin_phi)
   * the cosine is
   * (cos_phi*cos_alpha*delta_x + cos_phi*sin_alpha*delta_y+ sin_phi)/sqrt( 1 + delta_x^2 + delta_y^2)
   *
   * For vertical light the cosine is
   *        1 / sqrt( 1 + delta_x^2 + delta_y^2)
   */
   
  delta_x = p - p_minus_x;
  delta_y = p - p_minus_y;
  delta_x_sqr = delta_x * delta_x;
  delta_y_sqr = delta_y * delta_y;
  hypot_sqr = delta_x_sqr + delta_y_sqr;
  norm = sqrt( 1.0 + hypot_sqr );

  /*{{{  calculate effective height */
  effective = (p + (varience * contour *
          (1.0/ ( 1.0 + hypot_sqr))));
  /*}}}*/
  /*{{{  calculate colour band. */
  band = ( effective / varience) * N_BANDS;
  if ( band < 0 )
  {
    band = 0;
  }
  if( band > (N_BANDS - 1))
  {
    band = (N_BANDS -1);
  }
  result = (BAND_BASE + (band * band_size));
  /*}}}*/

  /*{{{calculate the illumination stength*/
  /*
   * add in a contribution for the vertical light. The normalisation factor
   * is applied later
   *
   */
  dshade = vstrength;
  
  if( p >= shadow )
  {
    /*
     * add in contribution from the main light source
     */
    /* dshade += ((double) lstrength * ((delta_x * cos_phi) + sin_phi));*/
    dshade += ((double) lstrength *
               ((delta_x * x_fact) + (delta_y * y_fact) + sin_phi));
  }
  /* divide by the normalisation factor (the same for both light sources) */
  dshade /= norm;
  /*}}}*/
  /*{{{  calculate shading */
  /* dshade should be in the range 0.0 -> 1.0
   * if the light intensities add to 1.0
   * now convert to an integer
   */
  shade = dshade * (double) band_size;
  if( shade > (band_size-1))
  {
    shade = (band_size-1);
  }
  /*{{{  if shade is negative then point is really in deep shadow */
  if( shade < 0 )
  {
      shade = 0;
  }
  /*}}}*/
  /*}}}*/
  result += shade;
  if( (result >= n_col) || (result < 0) )
  {
    fprintf(stderr,"INTERNAL ERROR colour out of range %d\n",result);
    exit(1);
  }
  return(result);
}
/*}}}*/
/*{{{  Col *makemap(Height *a, Height *b, Height *shadow) */
Col *makemap (a,b,shadow)
Height *a;
Height *b;
Height *shadow;
{
Col *res;
int i;

  /* This routine returns a plan view of the surface */
  res = (Col *) malloc(width * sizeof(Col) );
  if (res == NULL)
  {
    fprintf(stderr,"malloc failed for colour strip\n");
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
/*{{{  Col *camera(Height *a, Height *b, Height *shadow) */
Col *camera(a,b,shadow)
Height *a;
Height *b;
Height *shadow;
{
  int i, j, coord, last;
  Col *res, col;

  /* this routine returns a perspective view of the surface */
  res = (Col *) malloc( height * sizeof(Col) );
  if( res == NULL )
  {
    fprintf(stderr,"malloc failed for picture strip\n");
    exit(1);
  }
  /*
   * optimised painters algorithm
   *
   * scan from front to back, we can avoid calculating the
   * colour if the point is not visable.
   */
  for( i=0, last=0 ; (i < width)&&(last < height) ; i++ )
  {
    if( a[i] < sealevel )
    {
      a[i] = sealevel;
    }
    coord = 1 + project( i, a[i] );
    if( coord > last )
    {
      /* get the colour of this point, the front strip should be black */
      if( i==0 )
      {
        col = BLACK;
      }else{
        col = get_col(b[i],a[i],b[i-1],shadow[i]);
      }
      if( coord > height )
      {
        coord = height;
      }
      for(;last<coord;last++)
      {
        res[last]=col;
      }
    }
  }
  for(;last<height;last++)
  {
    res[last]=SKY;
  }
  return(res);
}
/*}}}*/
/*{{{  int project( int x , Height y ) */
/*
 *  project a point onto the screen position
 */
int project (x,y)
int x;
Height y;
{
  int pos;
#ifndef SLOPPY
  double theta;

  theta = atan( (double) ((viewheight - y)/( x - viewpos)) );
  theta = theta - vangle;
  pos = (height/2) - (focal * tan( theta));
#else
  float tan_theta;

  /* nast approx to avoid trig functions */
  tan_theta = (viewheight -y)/(x-viewpos) - tan_vangle;
  pos = (height/2) - (focal * tan_theta);
#endif
  if( pos > (height-1))
  {
    pos = height-1;
  }
  else if( pos < 0 )
  {
    pos = 0;
  }
  return( pos );
}
/*}}}*/
/*{{{  void finish_artist() */
/*
 * Tidy up and free everything.
 */
void finish_artist()
{
  free(a_strip);
  free(b_strip);
  free(shadow);
  free_fold(top);
}
/*}}}*/
