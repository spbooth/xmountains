/*
 * Recursive update procedure for fractal landscapes
 *
 * The only procedures needed outside this file are
 *   make_fold, called once to initialise the data structs.
 *   next_strip, each call returns a new strip off the side of the surface
 *                you can keep calling this as often as you want.
 *   free_strip, get rid of the strip when finished with it.
 *   free_fold,  get rid of the data structs when finished with this surface.
 *
 * Apart from make_fold all these routines get their parameters from their
 * local Fold struct, make_fold initialises all these values and has to
 * get it right for the fractal to work. If you want to change the fractal
 * dim in mid run you will have to change values at every level.
 * each recursive level only calls the level below once for every two times it
 * is called itself so it will take a number of iterations for any changes to
 * be notices by the bottom (long length scale) level.
 * The surface always starts as perturbations from a flat surface at the
 * mean value passed as a parameter to make_fold. It will therefore take
 * a number of iterations for long length scale deformations to build up.
 */
#include <stdio.h>
#include <math.h>
#include "crinkle.h"

char calcalt_Id[] = "$Id: calcalt.c,v 1.8 1994/01/21 11:53:47 spb Rel $";

/*{{{  Strip *make_strip(int level) */
Strip *make_strip (level)
int level;
{
  Strip *p;
  int i , points;

  p = (Strip *) malloc( sizeof(Strip) );
  if( p == NULL )
  {
    fprintf(stderr,"make_strip: malloc failed\n");
    exit(1);
  }
  p->level = level;
  points = (1 << level) +1;
  p->d = (Height *)malloc( points * sizeof(Height) );
  if( p->d == NULL )
  {
    fprintf(stderr,"make_strip: malloc failed\n");
    exit(1);
  }
  return(p);
}
/*}}}*/
/*{{{  void free_strip(Strip *p) */
void free_strip (p)
Strip *p;
{
  if( p->d )
  {
    free(p->d);
    p->d = NULL;
  }
  free(p);
}
/*}}}*/
/*{{{  Strip *double_strip(Strip s) */
Strip *double_strip (s)
Strip *s;
{
  Strip *p;
  Height *a, *b;
  int i;

  p = make_strip((s->level)+1);
  a = s->d;
  b = p->d;
  for(i=0; i < (1<<s->level); i++)
  {
    *b = *a;
    a++;
    b++;
    *b = 0;
    b++;
  }
  *b = *a;
  return(p);
}
/*}}}*/
/*{{{  Strip *set_strip(int level, Height value) */
Strip *set_strip (level,value)
int level;
Height value;
{
  int i;
  Strip *s;
  Height *h;

  s = make_strip(level);
  h = s->d;
  for( i=0 ; i < ((1<<level)+1) ; i++)
  {
    *h = value;
    h++;
  }
  return(s);
}
/*}}}*/
/*{{{  void side_update(Strip *strip, Length scale) */
/* fill in the blanks in a strip that has just been doubled
 * this could be combined with the double routine but it would
 * make the code even messier than it already is
 */
void side_update (strip,scale)
Strip *strip;
Length scale;
{
  int count;
  int i;
  Height *p;

  count = ( 1 << (strip->level - 1));
  p = strip->d;
  for(i=0 ; i<count ; i++)
  {
    *(p+1) = ((scale * gaussian()) + (*p + *(p+2))/2.0) ;
    p += 2;
  }
}
/*}}}*/
/*{{{  void mid_update(Strip *left, Strip *new, Strip *right,Length scale, Length midscale) */
/* Calculate a new strip using the two strips to either side.
 * the "left" strip should be only half the size of the other
 * two.
 */
void mid_update (left,new,right,scale,midscale)
Strip *left;
Strip *new;
Strip *right;
Length scale;
Length midscale;
{
  int count;
  int i;
  Height *l , *n , *r;

  if( (left->level != (new->level - 1)) || (new->level != right->level))
  {
    fprintf(stderr,"mid_update: inconsistant sizes\n");
    exit(2);
  }
  count = ( 1 << left->level);
  l = left->d;
  n = new->d;
  r = right->d;
  for(i=0 ; i<count ; i++)
  {
    *n = ((scale * gaussian()) + (*l + *r)/2.0) ;
    n++;
    *n = ((midscale * gaussian()) + (*l + *(l+1) + *r + *(r+2))/4.0) ;
    n++;
    l++;
    r += 2;
  }
  /* the last one */
  *n = ((scale * gaussian()) + (*l + *r)/2.0) ;
}
/*}}}*/
/*{{{  void recalc(Strip *left, Strip *regen, Strip *right,Length scale) */
/* Recalculate all the old values using the points we have just 
 * generated. This is a little idea of mine to get rid of the
 * creases. However it may change the effective fractal dimension
 * a litle bit. But who cares ?
 */
void recalc (left,regen,right,scale)
Strip *left;
Strip *regen;
Strip *right;
Length scale;
{
  int count;
  int i;
  Height *l , *g , *r;

  if((left->level != regen->level) || (regen->level != right->level))
  {
    fprintf(stderr,"recalc: inconsistant sizes\n");
    exit(2);
  }
  count = ( (1 << (regen->level-1)) - 1 );
  l = left->d;
  g = regen->d;
  r = right->d;
  *g = ((scale * gaussian()) + (*l + *(g+1) + *r)/3.0) ;
  g += 2;
  l += 2;
  r += 2;
  for(i=0 ; i<count ; i++)
  {
    *g = ((scale * gaussian()) + (*l + *(g+1) + *(g-1) + *r)/4.0) ;
    g += 2;
    l += 2;
    r += 2;
  }
  /* the last one */
  *g = ((scale * gaussian()) + (*l + *(g-1) + *r)/3.0) ;
}
/*}}}*/
/*{{{  Strip *next_strip(Fold *fold) */
Strip *next_strip (fold)
Fold *fold;
{
  Strip *result;
  int i;

  if( fold->level == fold->stop)
  {
    /*{{{  generate values from scratch */
    result=make_strip(fold->stop);
    i=0;
    if( fold->slope )
    {
      result->d[i] = fold->mean;
      i++;
    }
    for( ; i < ((1 << fold->stop) +1) ; i++)
    {
      result->d[i] = fold->mean + (fold->scale * gaussian());
    }
    return(result);
    /*}}}*/
  }
  switch(fold->state)
  {
    case START:
      /*{{{  perform an update. return first result */
      /*
       * new is NULL
       * working is NULL
       * regen is a partial strip, only odd values are valid,
       * old is a fully calculated strip
       */
      fold->new = next_strip(fold->next);
      side_update(fold->regen,fold->scale);
      fold->working = make_strip(fold->level);
      mid_update(fold->new,fold->working,fold->regen,
                  fold->scale,fold->midscale);
      if( fold->smooth )
      {
        recalc(fold->working,fold->regen,fold->old,fold->scale);
      }
      result = fold->old;
      fold->old = NULL;
      fold->state = STORE;
      return(result);
      /*}}}*/
    case STORE:
      /*{{{  return second value from previous update. */
      result = fold->regen;
      fold->old = fold->working;
      fold->working = NULL;
      fold->regen = double_strip(fold->new);
      free_strip(fold->new);
      fold->new = NULL;
      fold->state = START;
      return(result);
      /*}}}*/
    default:
      fprintf(stderr,"next_strip: invalid state\n");
      exit(3);
  }
  return(NULL);
}
/*}}}*/
/*{{{  Fold *make_fold(int levels, int stop, int fractal_start, int slope,int smooth, Length len, Height start, Height mean, float fdim) */
/*
 * Initialise the fold structures.
 * As everything else reads the parameters from their fold
 * structs we need to set these here,
 * levels is the number of levels of recursion below this one.
 *    Number of points = 2^levels+1
 * stop is the number of levels that are generated as random offsets from a
 *    constant rather than from an average.
 * fractal_start, if true we start in the middle of a mountain range
 *    if false we build up mountains from the start.
 * slope, force the front of the fractal to remain close to the mean value
 * smooth turns the smoothing algorithm on or off
 * len is the length of the side of the square at this level.
 *   N.B this means the update square NOT the width of the fractal.
 *   len gets smaller as the level increases.
 * start, see uder fractal_start.
 * mean is the mean height.
 * fdim is the fractal dimension
 */
Fold *make_fold (levels,stop,fractal_start,slope,smooth,length,start,mean,fdim)
int levels;
int stop;
int fractal_start;
int slope;
int smooth;
Length length;
Height start;
Height mean;
float fdim;
{
  Fold *p;
  Length scale, midscale;
  double root2;

  if( (levels < stop) || (stop<0) )
  {
    fprintf(stderr,"make_fold: invalid parameters\n");
    fprintf(stderr,"make_fold: levels = %d , stop = %d \n",levels,stop);
    exit(1);
  }
  p = (Fold *)malloc(sizeof(Fold));
  if( p == NULL )
  {
    fprintf(stderr,"make_fold: malloc failed\n");
    exit(1);
  }
  root2=sqrt((double) 2.0 );
  scale = pow((double) length, (double) (2.0 * fdim));
  midscale = pow((((double) length)*root2), (double) (2.0 * fdim));
  p->level = levels;
  p->stop = stop;
  p->state = START;
  p->smooth = smooth;
  p->slope = slope;
  p->mean = mean;
  p->scale = scale;
  p->midscale = midscale;
  p->new = NULL;
  p->working = NULL;
  if( levels > stop )
  {
    p->next = make_fold((levels-1),stop,fractal_start,slope,smooth,(2.0*length),start,mean,fdim);
    if ( fractal_start )
    {
      p->regen = double_strip(next_strip( p->next ));
      p->old = NULL;
      /*
       * do an update step to flush out the undefined p->old
       * pointer, as p->old is used in the regeneration step we
       * have to force smoothing to be off. This should return a NULL
       * pointer
       */
      p->smooth = FALSE;
      if( NULL != next_strip( p ) )
      {
        fprintf(stderr,"make_fold: internal logic error\n");
        exit(2);
      }
      p->smooth = smooth;
    }else{
      p->regen = set_strip(levels,start);
      p->old = set_strip(levels,start);
    }
  }else{
    p->regen = NULL;
    p->old = NULL;
    p->next = NULL;
  }
  return( p );
}
/*}}}*/
/*{{{  void free_fold(Fold *f) */
void free_fold (f)
Fold *f;
{
  if( f->new != NULL )
  {
    free_strip(f->new);
    f->new = NULL;
  }
  if( f->working != NULL )
  {
    free_strip(f->working);
    f->working = NULL;
  }
  if( f->regen != NULL )
  {
    free_strip(f->regen);
    f->regen = NULL;
  }
  if( f->old != NULL )
  {
    free_strip(f->old);
    f->old = NULL;
  }
  if( f->next != NULL )
  {
    free_fold(f->next);
    f->next = NULL;
  }
  free(f);
  return;
}
/*}}}*/
