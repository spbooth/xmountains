#ifndef GRAPHICS
#define GRAPHICS
/* These are the interfaces to the graphics sub-system
 * it should be possible to swap in an alternate sub-system if this interface is preserved
 *
 * we include OS specific here as well.
 *
 */

// initialise the graphics sub-system
void init_graphics(int  want_use_root,unsigned long  window_id,int  use_background,int  want_clear,Graph *gptr,Gun *red,Gun *green,Gun *blue );
// blank an entire column
void blank_col( int );
// plot a pixel using one of our pre-calcualted colours
void plot_pixel (int, int, Col);
// scroll the screen image left or right (-ve values reverse direction)
void scroll_screen ( int );
// run the animation loop
void animate(void next_frame(Graph *g),Graph *g,int snooze,int millis);
// get an integer from the clock. Not graphics related but often OS related
int get_clock();



#endif
