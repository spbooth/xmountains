Ymountains
==========

This is an experimental port of the graphics back-end to wayland.
X_graphics.c is replaced with Y_graphics.c and the code is build using Makefile.wayland
Otherwise all the code is common with Xmountains.

This is very much a work in progress:
* There is no support for window decorations, resizing input etc.
* The command line arguments are still those for Xmountains and many of which do nothing in ymountains

As a raster image application its actually a fairly good fit for wayland. The code follows the example code from the wayland book very closely.
Inevitably the biggest lack are those functions of the X window manager that have been transfered to the client. 

Current features:
* window will quit on ESC, right-mouse or close window (if you can generate one without decorations)
* left-mouse will move the window

Issues:
* on my test platform (Ubuntu under WSL) I need to right click once before any mouse events passed on. This may be a compositor issue.


Its called Ymountains as I'm reserving the name Wmountains for a Win32 port.