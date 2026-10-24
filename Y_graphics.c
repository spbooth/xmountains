/**  This is an implementation of the graphics functions in wayland.
* 
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include <time.h>
#ifndef NO_SIGNAL
#include <signal.h>
#endif
#include "paint.h"
#include "graphics.h"


#ifndef FALSE
#define TRUE 1
#define FALSE 0
#endif

  unsigned int graph_width;
  unsigned int graph_height;

  char *display=NULL;       /* name of display to open, NULL for default */
  char *geom=NULL;          /* geometry of window, NULL for default */
  int swosh=FALSE;

  /* plot history */
  int plot_x, plot_y1, plot_y2;
  uint32_t plot_col;
  int plot_saved=FALSE;
  uint32_t *table; // CLUT translating Col values into native wayland pixels
  
  // backing store for the image. This is what we actually write to.
  // initally we store this as wayland pixels because they are the same size as Col values
  // and this will make publishing the image faster. We could optimise for COL_8_BIT by
  // storing these as COL and mapping in the blit operation.
  // also worth exploring if its worth writing directly to the shared space
  typedef uint32_t Pix;
  Pix *pix;
  
extern Graph g;

void finish_artist();

/* Shared memory support code */
static void
randname(char *buf)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long r = ts.tv_nsec;
    for (int i = 0; i < 6; ++i) {
        buf[i] = 'A'+(r&15)+(r&16)*2;
        r >>= 5;
    }
}

static int
create_shm_file(void)
{
    int retries = 100;
    do {
        char name[] = "/wl_shm-XXXXXX";
        randname(name + sizeof(name) - 7);
        --retries;
        int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd >= 0) {
            shm_unlink(name);
            return fd;
        }
    } while (retries > 0 && errno == EEXIST);
    return -1;
}

static int
allocate_shm_file(size_t size)
{
    int fd = create_shm_file();
    if (fd < 0)
        return -1;
    int ret;
    do {
        ret = ftruncate(fd, size);
    } while (ret < 0 && errno == EINTR);
    if (ret < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

/* Wayland code */
struct client_state {
	/* app state */
	void (*next_column)(Graph *g);
	Graph *g;
    /* Globals */
    struct wl_display *wl_display;
    struct wl_registry *wl_registry;
    struct wl_shm *wl_shm;
    struct wl_compositor *wl_compositor;
    struct xdg_wm_base *xdg_wm_base;
    /* Objects */
    struct wl_surface *wl_surface;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
    struct wl_seat *wl_seat;
    struct wl_keyboard *wl_keyboard;
    uint32_t last_frame;
    int snooze;
    int millis;
    int quit;
};

static void
wl_buffer_release(void *data, struct wl_buffer *wl_buffer)
{
    /* Sent by the compositor when it's no longer using this buffer */
    wl_buffer_destroy(wl_buffer);
}

static const struct wl_buffer_listener wl_buffer_listener = {
    .release = wl_buffer_release,
};

static struct wl_buffer *
draw_frame(struct client_state *state)
{
    const int width = state->g->graph_width, height = state->g->graph_height;
    int stride = width * 4;
    int size = stride * height;


    int fd = allocate_shm_file(size);
    if (fd == -1) {
        return NULL;
    }

    uint32_t *data = mmap(NULL, size,
            PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        close(fd);
        return NULL;
    }

    struct wl_shm_pool *pool = wl_shm_create_pool(state->wl_shm, fd, size);
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0,
            width, height, stride, WL_SHM_FORMAT_XRGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);

    if( state->next_column ){
    	// plot a column
    	state->next_column(state->g);
    }
    int offset = 0;
    if( swosh && state->g->repeat < 0){
    	offset = -state->g->repeat;
    }
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if( x < state->g->pixmap_width && y < state->g->graph_height){
            	data[y * width + x] = pix[y * state->g->pixmap_width +x + offset ];
            }else{
            	data[y * width + x] = table[BLACK];
            }
        }
    }

    munmap(data, size);
    wl_buffer_add_listener(buffer, &wl_buffer_listener, NULL);
    return buffer;
}

static void
xdg_surface_configure(void *data,
        struct xdg_surface *xdg_surface, uint32_t serial)
{
    struct client_state *state = data;
    xdg_surface_ack_configure(xdg_surface, serial);

    struct wl_buffer *buffer = draw_frame(state);
    wl_surface_attach(state->wl_surface, buffer, 0, 0);
    wl_surface_commit(state->wl_surface);
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

static void
xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

static void
finish_graphics(struct client_state *state)
{
    if (state->wl_keyboard) {
        wl_keyboard_destroy(state->wl_keyboard);
        state->wl_keyboard = NULL;
    }
    if (state->wl_seat) {
        wl_seat_destroy(state->wl_seat);
        state->wl_seat = NULL;
    }
    if (state->xdg_toplevel) {
        xdg_toplevel_destroy(state->xdg_toplevel);
        state->xdg_toplevel = NULL;
    }
    if (state->xdg_surface) {
        xdg_surface_destroy(state->xdg_surface);
        state->xdg_surface = NULL;
    }
    if (state->wl_surface) {
        wl_surface_destroy(state->wl_surface);
        state->wl_surface = NULL;
    }
    if (state->wl_display) {
        wl_display_disconnect(state->wl_display);
        state->wl_display = NULL;
    }
}

static void
wl_keyboard_key(void *data, struct wl_keyboard *wl_keyboard,
        uint32_t serial, uint32_t time, uint32_t key, uint32_t key_state)
{
    struct client_state *state = data;
    /* Escape, the xkb keycode is the same on all layouts.
     * Only set the flag here; the wayland display must not be
     * disconnected from inside an event callback, the event loop in
     * animate() checks the flag and shuts down once it is set.
     */
    /** The scancode values used by wayland are evdev scancodes
     * which differ from xkb by 8 to ESC is scancode 1
     *
     */
    if (key_state == WL_KEYBOARD_KEY_STATE_RELEASED && key == 1)
        state->quit = TRUE;
}

/* We do not interpret the keymap, so just drop the file descriptor */
static void
wl_keyboard_keymap(void *data, struct wl_keyboard *wl_keyboard,
        uint32_t format, int32_t fd, uint32_t size)
{
    close(fd);
}

static void
wl_keyboard_enter(void *data, struct wl_keyboard *wl_keyboard,
        uint32_t serial, struct wl_surface *surface,
        struct wl_array *keysyms)
{
}

static void
wl_keyboard_leave(void *data, struct wl_keyboard *wl_keyboard,
        uint32_t serial, struct wl_surface *surface)
{
}

static void
wl_keyboard_modifiers(void *data, struct wl_keyboard *wl_keyboard,
        uint32_t serial, uint32_t mods_depressed,
        uint32_t mods_latched, uint32_t mods_locked,
        uint32_t group)
{
}

static void
wl_keyboard_repeat_info(void *data, struct wl_keyboard *wl_keyboard,
        int32_t rate, int32_t delay)
{
}

static const struct wl_keyboard_listener wl_keyboard_listener = {
    .keymap = wl_keyboard_keymap,
    .enter = wl_keyboard_enter,
    .leave = wl_keyboard_leave,
    .key = wl_keyboard_key,
    .modifiers = wl_keyboard_modifiers,
    .repeat_info = wl_keyboard_repeat_info,
};

static void
wl_seat_capabilities(void *data, struct wl_seat *wl_seat,
        uint32_t capabilities)
{
    struct client_state *state = data;
    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
        if (!state->wl_keyboard) {
            state->wl_keyboard = wl_seat_get_keyboard(wl_seat);
            wl_keyboard_add_listener(state->wl_keyboard,
                    &wl_keyboard_listener, state);
        }
    } else if (state->wl_keyboard) {
        wl_keyboard_destroy(state->wl_keyboard);
        state->wl_keyboard = NULL;
    }
}

static const struct wl_seat_listener wl_seat_listener = {
    .capabilities = wl_seat_capabilities,
};

static const struct wl_callback_listener wl_surface_frame_listener;
static void
wl_surface_frame_done(void *data, struct wl_callback *cb, uint32_t time)
{
	/* Destroy this callback */
	wl_callback_destroy(cb);


	/* Request another frame */
	struct client_state *state = data;
	if(state->quit){
		// we are stopping no more frame requests
		return;
	}

	cb = wl_surface_frame(state->wl_surface);
	wl_callback_add_listener(cb, &wl_surface_frame_listener, state);

	int do_update=TRUE;
	if( state->last_frame != 0){
		int elapsed = time - state->last_frame;
		if( state->g->pause){
			do_update = elapsed > (1000*state->snooze);
		}else{
			do_update = elapsed > state->millis;
		}
		//printf("time %d  do_update=%d elapsed=%d\n",time,do_update,elapsed);
	}


	/* Submit a frame for this event */
	if( do_update){
		struct wl_buffer *buffer = draw_frame(state);
		wl_surface_attach(state->wl_surface, buffer, 0, 0);
		//wl_surface_damage_buffer(state->wl_surface, 0, 0, INT32_MAX, INT32_MAX);


		state->last_frame = time;
	}
	wl_surface_commit(state->wl_surface);
}


static const struct wl_callback_listener wl_surface_frame_listener = {
	.done = wl_surface_frame_done,
};

static void
registry_global(void *data, struct wl_registry *wl_registry,
        uint32_t name, const char *interface, uint32_t version)
{
    struct client_state *state = data;
    if (strcmp(interface, wl_shm_interface.name) == 0) {
        state->wl_shm = wl_registry_bind(
                wl_registry, name, &wl_shm_interface, 1);
    } else if (strcmp(interface, wl_compositor_interface.name) == 0) {
        state->wl_compositor = wl_registry_bind(
                wl_registry, name, &wl_compositor_interface, 4);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        state->xdg_wm_base = wl_registry_bind(
                wl_registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(state->xdg_wm_base,
                &xdg_wm_base_listener, state);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        state->wl_seat = wl_registry_bind(
                wl_registry, name, &wl_seat_interface, 1);
        wl_seat_add_listener(state->wl_seat, &wl_seat_listener, state);
    }
}

static void
registry_global_remove(void *data,
        struct wl_registry *wl_registry, uint32_t name)
{
    /* This space deliberately left blank */
}

static const struct wl_registry_listener wl_registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

  


/*{{{void blank_region(lx,ly,dx,dy)*/
void blank_region(int lx,int ly,int dx,int dy)
{
	  int x, y;

	  /* the display is always 32 bit on wayland, so the textured gray
	   * sky for low depth displays in the X11 back end is not needed
	   */
	  for( y = 0; y < dy; y++ )
	  {
	    for( x = 0; x < dx; x++ )
	    {
	      pix[(y+ly) * g.pixmap_width + (x+lx)] = table[SKY];
	    }
	  }
}
/*}}}*/

/*{{{void blank_col( pos )*/
void blank_col(int  pos )
{
  blank_region(pos,0,pos,graph_height);
}
/*}}}*/

struct client_state state = { 0 };

/*{{{unsigned char gun8(float intensity)*/
/*
 * convert a Gun intensity in [0,1] to an 8 bit value,
 * the wayland equivalent of convertGun
 */
unsigned char gun8(float intensity)
{
  int value;
  value = (int) (intensity * 255.0 + 0.5);
  if( value < 0 )
  {
    value = 0;
  }
  if( value > 255 )
  {
    value = 255;
  }
  return (unsigned char) value;
}
/*}}}*/
/*{{{void init_graphics( ... )*/
void init_graphics(int  want_use_root,unsigned long  window_id,int  use_background,int  want_clear,Graph *gptr,Gun *red,Gun *green,Gun *blue )
{
	 int i=0;
	/*{{{set palette*/
	  table = (uint32_t *)malloc((size_t) gptr->n_col * sizeof(uint32_t));
	  if( NULL == table )
	  {
	    fprintf(stderr,"malloc failed for colour table\n");
	    exit(1);
	  }
	  for(i = 0; i < gptr->n_col; i++)
	  {
	    table[i] = 0xff000000u
	             | ((uint32_t) gun8(red[i]) << 16)
	             | ((uint32_t) gun8(green[i]) << 8)
	             |  (uint32_t) gun8(blue[i]);
	  }
	/*}}}*/




/*{{{defs*/

  int x=0;
  int y=0;
  int gbits=0;

/*}}}*/


/*{{{create pixmap*/
  graph_width = gptr->graph_width;
  graph_height = gptr->graph_height;
  if(swosh)
  {
    if(gptr->repeat < 0)
    {
      gptr->pixmap_width = graph_width - gptr->repeat;
    }
    else
    {
      if(gptr->repeat == 0)
      {
        gptr->repeat = graph_width;
      }
      gptr->pixmap_width = graph_width + gptr->repeat;
    }
  }
  else
  {
    gptr->pixmap_width = graph_width;
  }
  pix = malloc(  gptr->pixmap_width * graph_height * sizeof(Pix));
  if( ! pix ){
	  fprintf(stderr,"malloc failed for internal pixmap\n");
	  exit(1);
  }
  //pix = XCreatePixmap(dpy, win, gptr->pixmap_width, graph_height, depth);

/*}}}*/
  blank_region(0, 0, gptr->pixmap_width, graph_height); 


  
  gptr->graph_width = graph_width;
  gptr->graph_height = graph_height;
  state.g=gptr;


  state.wl_display = wl_display_connect(NULL);
  state.wl_registry = wl_display_get_registry(state.wl_display);
  wl_registry_add_listener(state.wl_registry, &wl_registry_listener, &state);
  wl_display_roundtrip(state.wl_display);

  state.wl_surface = wl_compositor_create_surface(state.wl_compositor);
  state.xdg_surface = xdg_wm_base_get_xdg_surface(
          state.xdg_wm_base, state.wl_surface);
  xdg_surface_add_listener(state.xdg_surface, &xdg_surface_listener, &state);
  state.xdg_toplevel = xdg_surface_get_toplevel(state.xdg_surface);
  xdg_toplevel_set_title(state.xdg_toplevel, "Ymountains");
  xdg_toplevel_set_app_id(state.xdg_toplevel, "ymountains");
  wl_surface_commit(state.wl_surface);


}
/*}}}*/

/*{{{void scroll_screen( int dist )*/
void scroll_screen(int dist )
{
	  int reverse=FALSE;
	  int y;

	  if( dist < 0 )
	  {
	    dist = -dist;
	    reverse=TRUE;
	  }
	  /* scroll the pixmap */
	  if( dist > g.pixmap_width )
	  {
	    dist = g.pixmap_width;
	  }
	  if( reverse )
	  {
	    /* copy the data */
	    for( y = 0; y < graph_height; y++ )
	    {
	      memmove(&pix[y * g.pixmap_width + dist],
	              &pix[y * g.pixmap_width],
	              (size_t) (g.pixmap_width - dist) * sizeof(Pix));
	    }
	    /* blank new region */
	    blank_region(0,0,dist,graph_height);
	  }else{
	    /* copy the data */
	    for( y = 0; y < graph_height; y++ )
	    {
	      memmove(&pix[y * g.pixmap_width],
	              &pix[y * g.pixmap_width + dist],
	              (size_t) (g.pixmap_width - dist) * sizeof(Pix));
	    }
	    /* blank new region */
	    blank_region(g.pixmap_width - dist, 0, dist, graph_height);
	  }

	  flush_region(0,0,graph_width,graph_height);
}
/*}}}*/

/*{{{void plot_pixel( int x, int y, Col value )*/
void plot_pixel(int x,int  y,Col value )
{
  int do_draw, draw_x, draw_y1, draw_y2;
  uint32_t draw_colour;
  int r;
  
  /* if x is negative this means flush the stored request */
  if(! plot_saved) /* no stored values */
  {
    plot_x = x;
    plot_y1=plot_y2 = y;
    plot_col=table[value];
    do_draw=FALSE;
    plot_saved = (x >=0);
  }else{
    if( x < 0 )  /* requesting a flush */
    {
      draw_x=plot_x;
      draw_y1=plot_y1;
      draw_y2=plot_y2;
      draw_colour = plot_col;
      plot_saved=FALSE;
      do_draw=TRUE;
    }else{      /* plot request with saved value */
      if( (x==plot_x) && (plot_col == table[value])) /* add to line */
      {
        if(y<plot_y1) plot_y1=y;
        if(y>plot_y2) plot_y2=y;
        do_draw=FALSE;
      }else{
        draw_x=plot_x;
        draw_y1=plot_y1;
        draw_y2=plot_y2;
        draw_colour=plot_col;
        do_draw=TRUE;
        plot_x=x;
        plot_y1=plot_y2=y;
        plot_col=table[value];
      }
    }
  }
  if( do_draw )
  {
	    for( r = draw_y1; r <= draw_y2; r++ )
	    {
	      pix[r * g.pixmap_width + draw_x] = draw_colour;
	    }
  }
}
/*}}}*/

/*{{{void flush_region(int x, int y, int w, int h)*/
void flush_region(int x,int  y,int  w,int  h)
{
  /* flush outstanding plots */
  plot_pixel(-1,0,0);
  wl_surface_damage_buffer(state.wl_surface, x, y, w, h);
}
/*}}}*/
void animate(void next_frame(Graph *g),Graph *g,int snooze,int millis){
	state.next_column=next_frame;
	state.g=g;
	state.snooze=snooze;
	state.millis=millis;

	if( swosh ){
		// make initial surface as quick as possible
		// so run till pause
		while(! g->pause){
			next_frame(g);
		}
	}

	struct wl_callback *cb = wl_surface_frame(state.wl_surface);
	wl_callback_add_listener(cb, &wl_surface_frame_listener, &state);

	// wl_dsiplay_dispatch processes the queued events and will block
	// if there are no events. However this includes any event that
	// causes state.quit to be set so there should be no delay in exit
	while (! state.quit && wl_display_dispatch(state.wl_display)) {
		/* This space deliberately left blank */
	}



	finish_graphics(&state);
	finish_artist();
}

int get_clock(){
	return time((time_t *) 0);
}
