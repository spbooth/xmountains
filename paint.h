/* $Id: paint.h,v 1.13 1995/06/13 09:14:23 spb Exp $ */
#ifndef PAINT
#define PAINT

#include "crinkle.h"

/* colour code definitions */
typedef int Col;
typedef unsigned short Gun;


#define BLACK       0
#define WHITE       1
#define SEA_LIT     2
#define SEA_UNLIT   3
#define SKY         4
#define BAND_BASE   5
#ifndef BAND_SIZE
#define BAND_SIZE   80
#endif
#define N_BANDS     3
#define DEF_COL     (BAND_BASE + (N_BANDS * BAND_SIZE))
#define MIN_COL     (BAND_BASE + (N_BANDS * 2))
#define COL_RANGE   65535

#define PI 3.14159265

#ifdef ANSI
void set_clut(Gun *, Gun *, Gun *);
Height *extract(Strip *s);
void init_artist_variables();
Col get_col(Height p, Height p_plus_x, Height p_plus_y, Height shadow);
Col *makemap(Height *a, Height *b, Height *shadow);
Col *camera(Height *a, Height *b, Height *shadow);
Col *mirror(Height *a, Height *b, Height *shadow);
int project( int x , Height y );
#else
void set_clut();
Height *extract();
void init_artist_variables();
Col get_col();
Col *makemap();
Col *camera();
Col *mirror();
int project();
#endif


#endif
