#include <stdio.h>
#include<X11/Xlib.h>
#include<X11/Xutil.h>
#include<X11/Xatom.h>
#ifdef VROOT
#include"vroot.h"
#endif
char X_graphics_Id[]="$Id: Xgrab.c,v 1.1 2009/08/28 09:09:17 spb Exp $";

char *display=NULL;       /* name of display to open, NULL for default */
char *geom=NULL;          /* geometry of window, NULL for default */

#ifndef FALSE
#define TRUE 1
#define FALSE 0
#endif

  Display *dpy;
  int screen;

main(int argc, char *argv[]){
  int ierr;
  dpy = XOpenDisplay(display);
  
  if( ! dpy )
  {
    fprintf(stderr,"failed to open display\n");
    exit(1);
  }
  screen = DefaultScreen(dpy);
  printf("screen in %d\n",screen);
  if(ierr=XGrabServer(dpy)){
     printf("problem grabbing server %d \n",ierr);
  }
  sleep(10);
  XUngrabServer(dpy);

  XCloseDisplay(dpy);
}

