#include <stdio.h>
#include<X11/Xlib.h>
#include<X11/Xutil.h>
#include<X11/Xatom.h>
#include "paint.h"
char X_graphics_Id[]="$Id: X_graphics.c,v 1.16 1994/02/18 14:19:01 spb Exp $";

char *display=NULL;       /* name of display to open, NULL for default */
char *geom=NULL;          /* geometry of window, NULL for default */

Atom wm_protocols;
Atom wm_delete_window;

#ifndef FALSE
#define TRUE 1
#define FALSE 0
#endif

  int quit_xmount=FALSE;
  Display *dpy;
  int screen;
  unsigned int graph_width;
  unsigned int graph_height;
  Window parent, win, root;
  int use_root=FALSE;
  
#include <X11/bitmaps/gray>

  Pixmap stip;
  unsigned int depth=0;
  GC gc;
  Pixmap pix;
  Colormap map, defaultmap;
  XColor *table=NULL;
void zap_events();
void finish_graphics();
  
/*{{{void zap_events(int snooze)*/
void zap_events(snooze)
int snooze;
{
  XEvent event;
  XExposeEvent *expose = (XExposeEvent *)&event;
  int exw, exh, i;

#ifndef NO_SLEEP
  i=0;
  do{
#endif
  while( XPending(dpy) ){
    XNextEvent(dpy, &event);
    switch(event.type) {
      case ClientMessage:
        if (event.xclient.message_type == wm_protocols &&
          event.xclient.data.l[0] == wm_delete_window)  {
            quit_xmount=TRUE;
          }
            break;
        case ButtonPress:
            break;
        case ButtonRelease:
              quit_xmount=TRUE;
            break;
        case Expose:
          if( (expose->x < graph_width) && (expose->y < graph_height))
          {
            if( (expose->x + expose->width) > graph_width)
            {
              exw=graph_width - expose->x;
            }else{
              exw = expose->width;
            }
            if( (expose->y + expose->height) > graph_height)
            {
              exh=graph_height - expose->y;
            }else{
              exh = expose->height;
            }
            XCopyArea(dpy,pix,win,gc,expose->x,expose->y,
                          exw,exh,
                          expose->x,expose->y);
	  }
	  break;
        default:
            fprintf(stderr,"unrecognized event %d\n",event.type);
            XCloseDisplay(dpy);
            exit(1);
            break;
    }
  }
  if( quit_xmount )
  {
    finish_graphics();
    finish_artist();
    exit(0);
  }
#ifndef NO_SLEEP
    /* sleeping is very bad because it will prevent
     * events being processed but I suppose it is better
     * than being a CPU hog, as a compremise always check for
     * events at least once a second, looping for longer sleep times.
     * process the events before a sleep to make sure the screen is up to date.
     * the events must always be processed at least once.
     */
    if( snooze )
    {
      sleep(1);
    }
    i++;
  }while( i<snooze );
#endif
}
/*}}}*/
  
/*{{{void finish_graphics()*/
void finish_graphics()
{
  unsigned long attmask;
  XSetWindowAttributes attributes;
  int x,y;
  unsigned int border;
  int count;
  
  XFreePixmap(dpy,pix);
  XFreeGC(dpy,gc);
  XFreePixmap(dpy,stip);
  /* reset things if this was the root window. */
  if( use_root )
  {
    XGetGeometry(dpy,win,&root,&x,&y,&graph_width,&graph_height,&border,&depth);
    XClearArea(dpy,win,0,0,graph_width,graph_height,FALSE);
    attmask = 0;
    attmask |= CWColormap;
    attributes.colormap = defaultmap;
    XChangeWindowAttributes(dpy,win,attmask,&attributes);
  }
  if( map != defaultmap )
  {
    XFreeColormap(dpy, map );
  }
  if( count = XPending(dpy) )
  {
    fprintf(stderr,"WARNING: %d events still pending\n",count);
  }
  XCloseDisplay(dpy);
}
/*}}}*/

/*{{{void blank_region(lx,ly,ux,uy)*/
void blank_region(lx,ly,ux,uy)
int lx,ly,ux,uy;
{
  if( depth < 4 )
  {
    /* use a textured gray sky on monochrome displays
     * we may need this on any low-depth display
     */
    XSetForeground(dpy,gc,WhitePixel(dpy,screen));
    XSetFillStyle(dpy,gc,FillOpaqueStippled);
    XFillRectangle(dpy,pix,gc,lx,ly,ux,uy);
    XSetFillStyle(dpy,gc,FillSolid);
  }else{
    XSetForeground(dpy,gc,table[SKY].pixel);
    XFillRectangle(dpy,pix,gc,lx,ly,ux,uy);
  }
}
/*}}}*/

/*{{{void init_graphics( ... )*/
void init_graphics( want_use_root, use_background, s_graph_width,s_graph_height,ncol,red,green,blue )
int want_use_root;    /* display on the root window */
int  use_background;  /* install the pixmap as the background-pixmap */
int *s_graph_width;
int *s_graph_height;
int ncol;
Gun *red;
Gun *green;
Gun *blue;
{
/*{{{defs*/
  Visual *vis;
  int mask;
  int count;
  int x=0;
  int y=0;
  int gbits=0;
  unsigned long attmask;
  XSetWindowAttributes attributes;
  char * winname="Xmountains";
  XTextProperty textprop;

  unsigned int border;
  unsigned long gcvmask;
  XGCValues gcv;
  
  int i;
  int newmap=FALSE;

/*}}}*/

  use_root = want_use_root;
  graph_width = *s_graph_width;
  graph_height = *s_graph_height;
/*{{{open display*/

  dpy = XOpenDisplay(display);
  
  if( ! dpy )
  {
    fprintf(stderr,"failed to open display\n");
    exit(1);
  }
  screen = DefaultScreen(dpy);
  parent = RootWindow(dpy, screen);
/*}}}*/
/*{{{find appropriate vis*/
  map=defaultmap=DefaultColormap(dpy,screen);
  vis = DefaultVisual(dpy,screen);
  depth = DefaultDepth(dpy,screen);
/*}}}*/
/*{{{set colormap*/
  table=(XColor *)malloc(ncol * sizeof(XColor));
  if( NULL == table )
  {
    fprintf(stderr,"malloc failed for colour table\n");
    exit(1);
  }
  for(i=0; i<ncol ; i++)
  {
    table[i].red   = red[i];
    table[i].green = green[i];
    table[i].blue  = blue[i];
    while( ! XAllocColor(dpy,map,table+i) )
    {
      if( newmap ){
        fprintf(stderr,"failed to allocate colour %d\n",i);
        XCloseDisplay(dpy);
        exit(1);
      }else{
        map = XCopyColormapAndFree(dpy,map);
        newmap=TRUE;
      }
    }
  }
/*}}}*/
/*{{{create window*/
  attmask = 0;
  if( use_root )
  {
    win = parent;
    if( ! use_background )
    {
      attmask |= CWEventMask;
      attributes.event_mask = ExposureMask; /* catch expose events */
    }
    attmask |= CWColormap;
    attributes.colormap = map;
    XChangeWindowAttributes(dpy,win,attmask,&attributes);
  }else{
    if( geom )
    {
      gbits =XParseGeometry(geom,&x,&y,&graph_width,&graph_height);
      if((gbits & XValue) && (gbits & XNegative))
      {
        x += DisplayWidth(dpy,screen) - graph_width;
      }
      if((gbits & YValue) && (gbits & YNegative))
      {
        y += DisplayHeight(dpy,screen) - graph_height;
      }
    }
    attmask |= CWEventMask;
    if( ! use_background )
    {
      attributes.event_mask = ButtonPressMask|ButtonReleaseMask|ExposureMask;
    }else{
      attributes.event_mask = ButtonPressMask|ButtonReleaseMask;
    }
    attmask |= CWBackPixel;
    attributes.background_pixel = BlackPixel(dpy,screen);
    attmask |= CWBackingStore;
    attributes.backing_store = NotUseful;
    attmask |= CWColormap;
    attributes.colormap = map;
    win = XCreateWindow(dpy,parent,x,y,graph_width,graph_height,0,
      depth,InputOutput,vis,attmask,&attributes);
  
    /* Setup for ICCCM delete window. */
    wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
    wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    (void) XSetWMProtocols (dpy, win, &wm_delete_window, 1);
    textprop.value = (unsigned char *) winname;
    textprop.encoding = XA_STRING;
    textprop.format = 8;
    textprop.nitems = strlen(winname);
    XSetWMName(dpy,win,&textprop);
  }
/*}}}*/
/*{{{create pixmap*/
  XGetGeometry(dpy,win,&root,&x,&y,&graph_width,&graph_height,&border,&depth);
  stip = XCreateBitmapFromData(dpy,win,gray_bits,gray_width,gray_height);

  gcvmask = 0;
  gcvmask |= GCForeground;
  gcv.foreground = WhitePixel(dpy,screen);
  gcvmask |= GCBackground;
  gcv.background = BlackPixel(dpy,screen);
  gcvmask |= GCGraphicsExposures;
  gcv.graphics_exposures = FALSE;
  gcvmask |= GCStipple;
  gcv.stipple = stip;
  gcvmask |= GCFillStyle;
  gcv.fill_style = FillSolid;
  gcv.graphics_exposures = FALSE;
  
  gc = XCreateGC(dpy,win,gcvmask,&gcv);
  pix = XCreatePixmap(dpy,win,graph_width,graph_height,depth);

/*}}}*/
  blank_region(0,0,graph_width,graph_height); 

  if( use_background )
  {
    XSetWindowBackgroundPixmap(dpy,win,pix);
  }
  if( ! use_root )
  {
    XMapWindow(dpy, win );
  }

  zap_events(0);
  
  *s_graph_width = graph_width;
  *s_graph_height = graph_height;
}
/*}}}*/

/*{{{void scroll_screen( int dist )*/
void scroll_screen( dist )
int dist;
{
  /* scroll the pixmap */
  if( dist > graph_width )
  {
    dist = graph_width;
  }
  /* copy the data */
  XCopyArea(dpy,pix,pix,gc,dist,0,graph_width-dist,graph_height,0,0);
  /* blank new region */
  blank_region(graph_width-dist,0,dist,graph_height);
  /* update the window to match */
  XCopyArea(dpy,pix,win,gc,0,0,graph_width,graph_height,0,0);

}
/*}}}*/

/*{{{void plot_pixel( int x, int y, Gun value )*/
void plot_pixel( x, y, value )
int x;
int y;
Gun value;
{
  XSetForeground(dpy,gc,table[value].pixel);
  XDrawPoint(dpy,pix,gc,x,y);
}
/*}}}*/

/*{{{void flush_region(int x, int y, int w, int h)*/
void flush_region( x, y, w, h)
int x;
int y;
int w;
int h;
{
  XCopyArea(dpy,pix,win,gc,x,y,w,h,x,y);
}
/*}}}*/

