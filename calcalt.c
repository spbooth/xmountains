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

char calcalt_Id[] = "$Id: calcalt.c,v 2.0 1994/07/01 09:34:52 spb Exp $";

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
/*{{{  Fold *make_fold(int levels, int stop, int fractal_start, int slope,int smooth, Length len, Height start, Height mean, float fdim) */
/*
 * Initialise the fold structures.
 * As everything else reads the parameters from their fold
 * structs we need to set these here,
 * p  is the parameter struct common to all update levels.
 * levels is the number of levels of recursion below this one.
 *    Number of points = 2^levels+1
 * stop is the number of levels that are generated as random offsets from a
 *    constant rather than from an average.
 * fractal_start, if true we start in the middle of a mountain range
 *    if false we build up mountains from the start height.
 * length is the length of the side of the square at this level.
 *   N.B this means the update square NOT the width of the fractal.
 *   len gets smaller as the level increases.
 * start, the starting height for a non-fractal start.
 */
Fold *make_fold (param,levels,stop,fractal_start,length,start)
struct parm *param;
int levels;
int stop;
int fractal_start;
Length length;
Height start;
{
  Fold *p;
  Length scale, midscale;
  double root2;
  Strip *tmp;
  int i;

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
  scale = pow((double) length, (double) (2.0 * param->fdim));
  midscale = pow((((double) length)*root2), (double) (2.0 * param->fdim));
  p->level = levels;
  p->stop = stop;
  p->state = START;
  p->p = param;
  p->scale = scale;
  p->midscale = midscale;
  for(i=0;i<NSTRIP;i++)
  p->s[i] = NULL;
  if( levels > stop )
  {
    p->next = make_fold(param,(levels-1),stop,fractal_start,(2.0*length),start);
    if ( fractal_start )
    {
      /*
       * flush out the NULL pointers so the
       * pipeline is ready to run
       */
      while( p->s[0] == NULL )
      {
        next_strip(p);
      }
    }else{
      p->s[NSTRIP-2] = set_strip(p->level,start);
    }
  }else{
    p->next = NULL;
  }
  return( p );
}
/*}}}*/
/*{{{  void free_fold(Fold *f) */
void free_fold (f)
Fold *f;
{
  int i;
  for(i=0;i<NSTRIP;i++)
  {
    if( f->s[i] != NULL )
    {
      free_strip(f->s[i]);
      f->s[i] = NULL;
    }
  }
  free(f);
  return;
}
/*}}}*/

/*{{{  Strip *next_strip(Fold *fold) */
Strip *next_strip (fold)
Fold *fold;
{
  Strip *result, *tmp;
  int i, count;

  count = ((1 << fold->stop) +1);
  if( fold->level == fold->stop)
  {
    /*{{{  generate values from scratch */
    result=make_strip(fold->stop);
    for( i=0 ; i < count ; i++)
    {
      result->d[i] = fold->p->mean + (fold->scale * gaussian());
    }
    if( fold->p->force_front ){
      result->d[0] = fold->p->mean;
    }
    if( fold->p->force_back ){
      result->d[count-1] = fold->p->mean;
    }
    return(result);
    /*}}}*/
  }
  switch(fold->state)
  {
    case START:
      /*{{{  perform an update. return first result*/
      tmp=NULL;
      while( tmp == NULL )
      {
        tmp = next_strip(fold->next);
      }
      fold->s[7] = double_strip(tmp);
      x_update(count,fold->midscale,0.0,fold->s[5],fold->s[6],fold->s[7]);
      if(fold->p->rg1)
      {
        v_update(count,fold->midscale,fold->p->mix,fold->s[4],fold->s[5],fold->s[6]);
      }
      if( fold->p->cross )
      {
        t_update(count,fold->scale,0.0,fold->s[3],fold->s[4],fold->s[5]);
        p_update(count,fold->scale,0.0,fold->s[2],fold->s[3],fold->s[4]);
      }else{
        vside_update(count,fold->scale,0.0,fold->s[4]);
        hside_update(count,fold->scale,0.0,fold->s[2],fold->s[3],fold->s[4]);
      }
      if(fold->p->rg2)
      {
        if( fold->p->cross )
        {
          p_update(count,fold->scale,fold->p->mix,fold->s[1],fold->s[2],fold->s[3]);
        }else{
          vside_update(count,fold->scale,fold->p->mix,fold->s[2]);
        }
      }
      if(fold->p->rg3)
      {
        if( fold->p->cross )
        {
          t_update(count,fold->scale,fold->p->mix,fold->s[0],fold->s[1],fold->s[2]);
        }else{
          hside_update(count,fold->scale,fold->p->mix,fold->s[0],fold->s[1],fold->s[2]);
        }
      }
      
      result=fold->s[0];
      fold->state = STORE;
      return(result);
      /*}}}*/
    case STORE:
      /*{{{  return second value from previous update. */
      result = fold->s[1];
      for(i=0;i<(NSTRIP-2);i++)
      {
        fold->s[i] =fold->s[i+2];
      }
      fold->s[NSTRIP-2] = fold->s[NSTRIP-1]=NULL;
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

