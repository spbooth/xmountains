/* $Id: paint.h,v 1.3 1993/02/19 12:13:35 spb Exp $ */
#ifndef PAINT
#define PAINT

#include "crinkle.h"

/* colour code definitions */
typedef unsigned char Col;
typedef unsigned char Col_24[3];

#define SEA_LIT     0
#define SEA_UNLIT   1
#define SKY         2
#define BLACK       3
#define BAND_BASE   4
#define BAND_SIZE   40
#define N_BANDS     6
#define MAX_COL     (BAND_BASE + (N_BANDS * BAND_SIZE))

void set_clut();
Height *extract(Strip *s);
void init_artist_variables();
Col get_col(Height p, Height p_plus_x, Height p_plus_y, Height shadow);
Col *artist(Height *a, Height *b, Height *shadow);
Col *camera( Height *a, Col *c );
int project( int x , Height y );



#endif
