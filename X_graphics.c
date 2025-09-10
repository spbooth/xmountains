#include <stdio.h>
#include <string.h>
#include<X11/Xlib.h>
#include<X11/Xutil.h>
#include<X11/Xatom.h>
# define VROOT /* always do this */
#ifdef VROOT
#include"vroot.h"
#endif
#include "paint.h"
char X_graphics_Id[]="$Id: X_graphics.c,v 1.26 2009/08/28 09:09:17 spb Exp $";

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
  int do_clear=FALSE;
  int pixmap_installed=FALSE;

  int swosh=FALSE;

  /* plot history */
  int plot_x, plot_y1, plot_y2;
  unsigned long plot_col;
  int plot_saved=FALSE;
  
#include <X11/bitmaps/gray>

  Pixmap stip;
  unsigned int depth=0;
  GC gc;
  Pixmap pix;
  Pixmap bg_pix;
  Colormap map, defaultmap;
  XColor *table=NULL;
  
void finish_graphics();
  
Graph g={
1024,
768,
0,
0,
0.3,
1.0,
0.3,
0.6,
2.5,
4.0,
(40.0 * PI)/180.0,
0.0,
0.5,
0.0,
0.6,
DEF_COL,
60,
10,
2,
FALSE,
TRUE,
20,
0,
0
};

void finish_artist();

/*{{{void zap_events(int snooze)*/
void zap_events(int snooze)
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
            if(g.repeat < 0)
            {
              XCopyArea(dpy, pix, win, gc, expose->x - g.repeat, expose->y,
                            exw, exh,
                            expose->x,expose->y);
            }
            else
            {
              XCopyArea(dpy,pix,win,gc,expose->x,expose->y,
                            exw,exh,
                            expose->x,expose->y);
            }
          }
          break;
        default:
            fprintf(stderr,"xmountains: unrecognized event %d\n",event.type);
            /* XCloseDisplay(dpy);
             * exit(1);
             */
            break;
    }
  }
  if( quit_xmount )
  {
    finish_graphics();
    finish_artist();
    exit(0);
  }
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
    if( pixmap_installed &&  do_clear )
    {
      /* restore default pixmap for the root window */
      XSetWindowBackgroundPixmap(dpy,win,ParentRelative);
      XClearWindow(dpy,win);
    }else{
      XGetGeometry(dpy,win,&root,&x,&y,&graph_width,&graph_height,&border,&depth);
      XClearArea(dpy,win,0,0,graph_width,graph_height,FALSE);
    }
    if( map != defaultmap )
    {
      attmask = 0;
      attmask |= CWColormap;
      attributes.colormap = defaultmap;
      XChangeWindowAttributes(dpy,win,attmask,&attributes);
    }
  }
  if( map != defaultmap )
  {
    XFreeColormap(dpy, map );
  }
  if((count = XPending(dpy)))
  {
    fprintf(stderr,"WARNING: %d events still pending\n",count);
  }
  XCloseDisplay(dpy);
}
/*}}}*/

/*{{{void blank_region(lx,ly,ux,uy)*/
void blank_region(int lx,int ly,int ux,int uy)
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

/*{{{void blank_col( pos )*/
void blank_col(int  pos )
{
  blank_region(pos,0,pos,graph_height);
}
/*}}}*/

/*{{{void init_graphics( ... )*/
void init_graphics(int  want_use_root,Window  use_window,int  use_background,int  want_clear,Graph *gptr,Gun *red,Gun *green,Gun *blue )
{
/*{{{defs*/
  Visual *vis;

  
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
  XWindowAttributes xgwa;
  
  int i;
  int newmap=FALSE;

/*}}}*/

  if (use_window) use_root = 0;

  do_clear = want_clear;
  use_root = want_use_root;
  pixmap_installed = use_background;
  graph_width = gptr->graph_width;
  graph_height = gptr->graph_height;
/*{{{open display*/

  dpy = XOpenDisplay(display);
  
  if( ! dpy )
  {
    fprintf(stderr,"failed to open display\n");
    exit(1);
  }
  screen = DefaultScreen(dpy);
  parent = (use_window ? use_window : RootWindow(dpy, screen));
/*}}}*/
/*{{{find appropriate vis*/
  /* map=defaultmap=DefaultColormap(dpy,screen); */
  XGetWindowAttributes (dpy, parent, &xgwa);
  map=defaultmap=xgwa.colormap;
  vis = DefaultVisual(dpy,screen);
  depth = DefaultDepth(dpy,screen);
/*}}}*/
/*{{{set colormap*/
  table = (XColor *)malloc(gptr->n_col * sizeof(XColor));
  if( NULL == table )
  {
    fprintf(stderr,"malloc failed for colour table\n");
    exit(1);
  }
  for(i = 0; i < gptr->n_col; i++)
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
  if( use_root || use_window )
  {
    win = (use_window ? use_window : parent);
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

  /* if we are going to install this as a root pixmap, throw away
   * the old one FIRST. this reduces fragmentation
   */
  if( use_background && (use_root || use_window))
  {
    XSetWindowBackgroundPixmap(dpy,win,None);
  }
  if( use_root )
  {
    /* in case of virtual window manager set to size of display */
/* This is bogus -- always obey the size of the window itself */
/*    graph_width = DisplayWidth(dpy,screen); */
/*    graph_height = DisplayHeight(dpy,screen); */
  }

  if(swosh)
  {
    if(gptr->repeat < 0)
    {
      gptr->pixmap_width = graph_width - gptr->repeat;
      bg_pix = XCreatePixmap(dpy, win, graph_width, graph_height, depth);
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
  pix = XCreatePixmap(dpy, win, gptr->pixmap_width, graph_height, depth);

/*}}}*/
  blank_region(0, 0, gptr->pixmap_width, graph_height); 

  if( use_background )
  {
    XSetWindowBackgroundPixmap(dpy,win,pix);
  }
  if( ! use_root )
  {
    XMapWindow(dpy, win );
  }
  XClearWindow(dpy,win);
  zap_events(0);
  
  gptr->graph_width = graph_width;
  gptr->graph_height = graph_height;
}
/*}}}*/

/*{{{void scroll_screen( int dist )*/
void scroll_screen(int dist )
{
  int reverse=FALSE;

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
    XCopyArea(dpy, pix, pix, gc, 0, 0, g.pixmap_width - dist, graph_height, dist, 0);
    /* blank new region */
    blank_region(0,0,dist,graph_height);
  }else{
    /* copy the data */
    XCopyArea(dpy, pix, pix, gc, dist, 0, g.pixmap_width - dist, graph_height, 0, 0);
    /* blank new region */
    blank_region(g.pixmap_width - dist, 0, dist, graph_height);
  }
  /* update the window to match */
  if( pixmap_installed )
  {
    /* this line never seems to be needed on any system I know about
     * but the manual says an X server
     * may make a copy of a background pixmap
     * so in principle we should re-install.
     */
    XSetWindowBackgroundPixmap(dpy,win,None);

    if(swosh && reverse)
    {
      XCopyArea(dpy, pix, bg_pix, gc, dist, 0, graph_width, graph_height, 0, 0);
      XSetWindowBackgroundPixmap(dpy, win, bg_pix);
    }
    else
    {
      XSetWindowBackgroundPixmap(dpy,win,pix);
    }
    XClearWindow(dpy,win);
  }else{
    if(reverse)
    {
      XCopyArea(dpy, pix, win, gc, dist, 0, graph_width, graph_height, 0, 0);
    }
    else
    {
      XCopyArea(dpy, pix, win, gc, 0, 0, graph_width, graph_height, 0, 0);
    }
  }

}
/*}}}*/

/*{{{void plot_pixel( int x, int y, Gun value )*/
void plot_pixel(int x,int  y,Gun value )
{
  int do_draw, draw_x, draw_y1, draw_y2;
  unsigned long draw_colour;
  
  /* if x is negative this means flush the stored request */
  if(! plot_saved) /* no stored values */
  {
    plot_x = x;
    plot_y1=plot_y2 = y;
    plot_col=table[value].pixel;
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
      if( (x==plot_x) && (plot_col == table[value].pixel)) /* add to line */
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
        plot_col=table[value].pixel;
      }
    }
  }
  if( do_draw )
  {
    XSetForeground(dpy,gc,draw_colour);
    if( draw_y1 == draw_y2 )
    {
      XDrawPoint(dpy,pix,gc,draw_x,draw_y1);
    }else{
      XDrawLine(dpy,pix,gc,draw_x,draw_y1,draw_x,draw_y2);
    }
  }
}
/*}}}*/

/*{{{void flush_region(int x, int y, int w, int h)*/
void flush_region(int x,int  y,int  w,int  h)
{
  /* flush outstanding plots */
  plot_pixel(-1,0,0);
  if(!swosh)
  {
    XCopyArea(dpy,pix,win,gc,x,y,w,h,x,y);
  }
}
/*}}}*/

