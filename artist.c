/*
 *   routines to render a fractal landscape as an image
 */
#include <math.h>
#include <stdio.h>
#include "paint.h"
#include "crinkle.h"
#include "global.h"

char artist_Id[] = "$Id: artist.c,v 1.5 1993/03/15 11:31:46 spb Exp $";
#define SIDE 1.0
#define PI 3.14159265

/*{{{  void set_clut() */
void set_clut()
{
  int band,shade;
  float ambient = 0.5;  
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
  red[SKY]         = 103;
  green[SKY]       = 150;
  blue[SKY]        = 255;
  /*}}}*/
  /*{{{  sea (lit) */
  red[SEA_LIT]     = 0;
  green[SEA_LIT]   = 106;
  blue[SEA_LIT]    = 240;
  /*}}}*/
  /*{{{  sea (unlit) */
  red[SEA_UNLIT]   = 0;
  green[SEA_UNLIT] = 53;
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
void init_artist_variables()
{
  int i;
  float dh, dd;
  
  width= (1 << levels)+1;
  /* make the fractal SIDE wide, this makes it easy to predict the
   * average height returned by calcalt.
   */
  shadow = (Height *) malloc(width * sizeof(Height));
  if ( shadow == NULL )
  {
    fprintf(stderr,"malloc failed for shadow array\n");
    exit(1);
  }

  cos_phi = cos( phi );
  sin_phi = sin( phi );
  tan_phi = tan( phi );
  vscale = 0.8 * width;  /* have approx same height as width */
  varience = pow( SIDE,(2.0 * fdim));
  varience = vscale * varience ;
  shift = 0.5 * varience;
  varience = varience + shift;
  start = (sealevel - shift) / vscale ; /* always start at sealevel */ 
  for(i=0 ; i<width ; i++)
  {
    shadow[i] = start;
  }
  viewheight = 1.5 * varience;
  viewpos = -2.0 * width;
  dh = viewheight - (0.5 * varience);
  dd = (width / 2.0) - viewpos;
  focal = sqrt( (dd*dd) + (dh*dh) );
  tan_vangle = (double) (dh/dd);
  vangle = atan ( tan_vangle );

  top=make_fold(levels,smooth,(SIDE / width),start,mean,fdim);
  a_strip = extract( next_strip(top) ); 
  b_strip = extract( next_strip(top) );
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
/*{{{  Col *camera( Height *a, Col *c ) */
Col *camera( Height *a, Col *c )
{
  int i, j, coord, last;
  Col *res, last_col;

  res = (Col *) malloc( height * sizeof(Col) );
  if( res == NULL )
  {
    fprintf(stderr,"malloc failed for picture strip\n");
    exit(1);
  }
#if FALSE
  for(j=0 ; j<height ; j++)
  {
    res[j] = SKY;
  }
  for( i=width-1 ; i >= 0 ; i-- )
  {
    if( a[i] < sealevel )
    {
      a[i] = sealevel;
    }
    coord = project( i, a[i] );
    for(j=0; j<= coord ; j++)
    {
      res[j] = c[i];
    }
  }
#else
  last = height;
  last_col = SKY;
  for( i=width-1 ; i >= 0 ; i-- )
  {
    if( a[i] < sealevel )
    {
      a[i] = sealevel;
    }
    coord = 1 + project( i, a[i] );
    if( last > coord )
    {
      if( c[i] == last_col )
      {
        /* if the colours are the same just ensure that
         * last end up as the greater of the 2
         */
        coord = last;
      }else{
        for( j=coord; j < last ; j++ )
        {
          res[j] = last_col;
        }
      }
    }
    last = coord;
    last_col = c[i];
  }
  for( j=0 ; j < last ; j++)
  {
    res[j] = c[0];
  }
#endif
  return(res);
}
/*}}}*/
/*{{{  int project( int x , Height y ) */
int project( int x , Height y )
{
  int pos;
#if TRUE
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
void finish_artist()
{
  free(a_strip);
  free(b_strip);
  free(shadow);
  free_fold(top);
}
/*}}}*/
