#include <stdio.h>
#include<X11/Xlib.h>
#include<X11/Xutil.h>
#include<X11/Xatom.h>

char X_graphics_Id[]="$Id: X_graphics.c,v 1.3 1994/01/10 17:37:18 spb Exp $";

Atom wm_protocols;
Atom wm_delete_window;

#ifndef FALSE
#define TRUE 1
#define FALSE 0
#endif
  Display *dpy;
  int screen;
  int graph_width;
  int graph_height;
  Window parent, win, root;
  int use_root=FALSE;

  GC gc;
  Pixmap pix;
  Colormap map, defaultmap;
  XColor table[256];

/*{{{void finish_graphics()*/
void finish_graphics()
{
  unsigned long attmask;
  XSetWindowAttributes attributes;
  int x,y,border,depth;
  
  XGetGeometry(dpy,win,&root,&x,&y,&graph_width,&graph_height,&border,&depth);
  XClearArea(dpy,win,0,0,graph_width,graph_height,FALSE);
  attmask = 0;
  attributes.colormap = defaultmap;
  XChangeWindowAttributes(dpy,win,attmask,&attributes);
  XCloseDisplay(dpy);
}
/*}}}*/

/*{{{void zap_events()*/
void zap_events()
{
  XEvent event;
  XExposeEvent *expose = (XExposeEvent *)&event;
  int exw, exh;
  while( XPending(dpy) ){
    XNextEvent(dpy, &event);
    switch(event.type) {
      case ClientMessage:
        if (event.xclient.message_type == wm_protocols &&
          event.xclient.data.l[0] == wm_delete_window)  {
            finish_prog();
          }
            break;
        case ButtonPress:
            break;
        case ButtonRelease:
              finish_prog();
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
}
/*}}}*/

/*{{{void init_graphics( ... )*/
void init_graphics( int want_use_root, int *s_graph_width, int *s_graph_height,int ncol, unsigned char *red, unsigned char *green, unsigned char *blue )
{
/*{{{defs*/
  char *display=NULL;       /* name of display to open, NULL for default */
  int depth=0;              /* positive value requires this many planes */
  Visual *vis;
  int mask;
  int count;
  int x=0;
  int y=0;
  unsigned long attmask;
  XSetWindowAttributes attributes;
  char * winname="Mountains";
  XTextProperty textprop;

  int border;
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
  for(i=0; i<ncol ; i++)
  {
    table[i].red   = red[i]*257;
    table[i].green = green[i]*257;
    table[i].blue  = blue[i]*257;
    while( ! XAllocColor(dpy,map,table+i) )
    {
      if( newmap ){
        fprintf(stderr,"failed to allocate colour %d\n",i);
        finish_graphics();
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
    attmask |= CWEventMask;
    attributes.event_mask = ExposureMask; /* catch expose events */
    attmask |= CWColormap;
    attributes.colormap = map;
    XChangeWindowAttributes(dpy,win,attmask,&attributes);
  }else{
    attmask |= CWEventMask;
    attributes.event_mask = ButtonPressMask|ButtonReleaseMask|ExposureMask;
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
  gcvmask = 0;
  gcvmask |= GCForeground;
  gcv.foreground = WhitePixel(dpy,screen);
  gcvmask |= GCBackground;
  gcv.background = BlackPixel(dpy,screen);
  gcvmask |= GCFillStyle;
  gcv.fill_style = FillSolid;
  gcvmask |= GCGraphicsExposures;
  gcv.graphics_exposures = FALSE;
  gc = XCreateGC(dpy,win,gcvmask,&gcv);
  pix = XCreatePixmap(dpy,win,graph_width,graph_height,depth);
/*}}}*/
  XSetForeground(dpy,gc,BlackPixel(dpy,screen));
  XFillRectangle(dpy,pix,gc,0,0,graph_width,graph_height); 
  XMapWindow(dpy, win );

  zap_events();
  
  *s_graph_width = graph_width;
  *s_graph_height = graph_height;
}
/*}}}*/

/*{{{void scroll_screen( int dist )*/
void scroll_screen( int dist )
{
  /* scroll the pixmap */
  if( dist > graph_width )
  {
    dist = graph_width;
  }
  /* copy the data */
  XCopyArea(dpy,pix,pix,gc,dist,0,graph_width-dist,graph_height,0,0);
  /* blank new region */
  XSetForeground(dpy,gc,BlackPixel(dpy,screen));
  XFillRectangle(dpy,pix,gc,graph_width-dist,0,dist,graph_height);
  /* update the window to match */
  XCopyArea(dpy,pix,win,gc,0,0,graph_width,graph_height,0,0);

}
/*}}}*/

/*{{{void plot_pixel( int x, int y, unsigned char value )*/
void plot_pixel( int x, int y, unsigned char value )
{
  XSetForeground(dpy,gc,table[value].pixel);
  XDrawPoint(dpy,pix,gc,x,y);
}
/*}}}*/


/*{{{void flush_region(int x, int y, int w, int h)*/
void flush_region(int x, int y, int w, int h)
{
  XCopyArea(dpy,pix,win,gc,x,y,w,h,x,y);
}
/*}}}*/

