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

char calcalt_Id[] = "$Id: calcalt.c,v 2.1 1994/07/01 12:03:27 spb Exp $";

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
/*{{{  Fold *make_fold(int levels, int stop, Length len) */
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
Fold *make_fold (param,levels,stop,length)
struct parm *param;
int levels;
int stop;
Length length;
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
  p->save =NULL;
  p->p = param;
  p->scale = scale;
  p->midscale = midscale;
  for(i=0;i<NSTRIP;i++)
  {
    p->s[i] = NULL;
  }
  if( levels > stop )
  {
    p->next = make_fold(param,(levels-1),stop,(2.0*length));
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
  Strip *result=NULL;
  Strip **t;
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
  /*
   * There are two types of strip,
   *  A strips - generated by the lower recursion layers.
   *             these contain the corner points and half the side points
   *  B strips - added by this layer, this contains the mid points and
   *             half the side points.
   *
   * The various update routines test for NULL pointer arguments so
   * that this routine will not fail while filling the pipeline.
   */
  while( result == NULL )
  {
    /*{{{iterate*/
    switch(fold->state)
    {
      case START:
        /*{{{  perform an update. return first result*/
        t=fold->s;
        /* read in a new A strip at the start of the pipeline */
        t[0] = double_strip(next_strip(fold->next));
        if(t[2]){
          /* make the new B strip */
          t[1]=make_strip(fold->level);
        }
        /*
         * create the mid point
         * t := A B A
         */
        x_update(count,fold->midscale,0.0,t[0],t[1],t[2]);
        if(fold->p->rg1)
        {
          /*
           * first possible regeneration step
           * use the midpoints to regenerate the corner values
           * increment t by 2 so we still have and A B A pattern
           */
          v_update(count,fold->midscale,fold->p->midmix,t[1],t[2],t[3]);
          t+=2;
        }
        /*
         * fill in the edge points
         * increment t by 2 to preserve the A B A pattern
         */
        if( fold->p->cross )
        {
          t_update(count,fold->scale,0.0,t[0],t[1],t[2]);
          p_update(count,fold->scale,0.0,t[1],t[2],t[3]);
          t+=2;
        }else{
          vside_update(count,fold->scale,0.0,t[1]);
          hside_update(count,fold->scale,0.0,t[1],t[2],t[3]);
          t+=2;
        }
        if(fold->p->rg2)
        {
          /*
           * second regeneration step update midpoint
           * from the new edge values
           */
          if( fold->p->cross )
          {
            p_update(count,fold->scale,fold->p->mix,t[0],t[1],t[2]);
          }else{
            vside_update(count,fold->scale,fold->p->mix,t[1]);
          }
        }
        /* increment t by 1
         * this gives a B A B pattern to regen-3
         * if regen 3 is not being used it leaves t pointing to the
         * 2 new result strips
         */
        t++;
        if(fold->p->rg3)
        {
          /* final regenration step
           * regenerate the corner points from the new edge values
           * this needs a B A B pattern
           * leave t pointing to the 2 new result strips
           */
          if( fold->p->cross )
          {
            t_update(count,fold->scale,fold->p->mix,t[0],t[1],t[2]);
          }else{
            hside_update(count,fold->scale,fold->p->mix,t[0],t[1],t[2]);
          }
          t++;
        }
        result=t[1];
        fold->save=t[0];
        t[0]=t[1]=NULL;
        fold->state = STORE;
        /*}}}*/
      case STORE:
        /*{{{  return second value from previous update. */
        result = fold->save;
        fold->save=NULL;
        for(i=NSTRIP-1;i>1;i--)
        {
          fold->s[i] =fold->s[i-2];
        }
        fold->s[0] = fold->s[1]=NULL;
        fold->state = START;
        /*}}}*/
      default:
        fprintf(stderr,"next_strip: invalid state\n");
        exit(3);
    }
    /*}}}*/
  }
  return(result);
}
/*}}}*/

/*{{{void x_update(int count,float scale, float mix, Strip *a, Strip *b, Strip *c)*/
void x_update(count, scale, mix, a, b, c)
int count;
float scale;
float mix;
Strip *a;
Strip *b;
Strip *c;
{
  int i;
  float w;
  Height *mp, *lp, *rp;

  w = (1.0 - mix)/4.0;
  mp=b->d+1;
  lp=a->d;
  rp=c->d;

  for(i=0; i<count-2; i+=2)
  {
    mp[0] = (mix * mp[0]) + w * ( lp[0] + rp[0] + lp[2] + rp[2])
          + (scale * gaussian());
    mp+=2;
    lp+=2;
    rp+=2;
  }
}    
/*}}}*/
/*{{{void p_update(int count,float scale, float mix, Strip *a, Strip *b, Strip *c)*/
void p_update(count, scale, mix, a, b, c)
int count;
float scale;
float mix;
Strip *a;
Strip *b;
Strip *c;
{
  int i;
  float w;
  Height *mp, *lp, *rp;

  w = (1.0 - mix)/4.0;
  mp=b->d;
  lp=a->d+1;
  rp=c->d+1;

  for(i=0; i<count-2; i+=2)
  {
    mp[1] = (mix * mp[1]) + w * ( lp[0] + rp[0] + mp[0] + mp[2] )
          + (scale * gaussian());
    mp+=2;
    lp+=2;
    rp+=2;
  }
}    

/*}}}*/
/*{{{void t_update(int count,float scale, float mix, Strip *a, Strip *b, Strip *c)*/
void t_update(count, scale, mix, a, b, c)
int count;
float scale;
float mix;
Strip *a;
Strip *b;
Strip *c;
{
  int i;
  float w, we;
  Height *mp, *lp, *rp;

  w = (1.0 - mix)/4.0;
  we = (1.0 - mix)/3.0;
  mp=b->d;
  lp=a->d;
  rp=c->d;

    mp[0] = (mix * mp[0]) + we * ( lp[0] + rp[0] + mp[1] )
          + (scale * gaussian());
    mp++;
    lp++;
    rp++;
  for(i=1; i<count-1; i+=2)
  {
    mp[1] = (mix * mp[1]) + w * ( lp[1] + rp[1] + mp[0] + mp[2] )
          + (scale * gaussian());
    mp+=2;
    lp+=2;
    rp+=2;
  }
  mp[1] = (mix * mp[1]) + we * ( lp[1] + rp[1] + mp[0] )
        + (scale * gaussian());
}    


/*}}}*/
/*{{{void v_update(int count,float scale, float mix, Strip *a, Strip *b, Strip *c)*/
void v_update(count, scale, mix, a, b, c)
int count;
float scale;
float mix;
Strip *a;
Strip *b;
Strip *c;
{
  int i;
  float w, we;
  Height *mp, *lp, *rp;

  w = (1.0 - mix)/4.0;
  we = (1.0 - mix)/2.0;
  mp=b->d+1;
  lp=a->d;
  rp=c->d;

    mp[0] = (mix * mp[0]) + we * ( lp[0] + rp[0] )
          + (scale * gaussian());
    mp+=2;
    lp+=2;
    rp+=2;
  for(i=1; i<count-1; i+=2)
  {
    mp[0] = (mix * mp[0]) + w * ( lp[0] + rp[0] + lp[2] + rp[2] )
          + (scale * gaussian());
    mp+=2;
    lp+=2;
    rp+=2;
  }
  mp[0] = (mix * mp[0]) + we * ( lp[0] + rp[0] )
          + (scale * gaussian());
}    

/*}}}*/
/*{{{void vside_update(int count,float scale, float mix, Strip *a)*/
void vside_update(count, scale, mix, a)
int count;
float scale;
float mix;
Strip *a;
{
  int i;
  float w;
  Height *mp;

  w = (1.0 - mix)/2.0;
  mp=a->d;

  for(i=0; i<count-2; i+=2)
  {
    mp[1] = (mix * mp[1]) + w * ( mp[0] + mp[2] )
          + (scale * gaussian());
    mp+=2;
  }
}    


/*}}}*/
/*{{{void hside_update(int count,float scale, float mix, Strip *a, Strip *b, Strip *c)*/
void hside_update(count, scale, mix, a, b, c)
int count;
float scale;
float mix;
Strip *a;
Strip *b;
Strip *c;
{
  int i;
  float w, we;
  Height *mp, *lp, *rp;

  w = (1.0 - mix)/2.0;
  mp=b->d;
  lp=a->d;
  rp=c->d;

  for(i=0; i<count; i+=2)
  {
    mp[0] = (mix * mp[0]) + w * ( lp[0] + rp[0] )
          + (scale * gaussian());
    mp+=2;
    lp+=2;
    rp+=2;
  }
}    



/*}}}*/



