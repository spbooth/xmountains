#include <stdio.h>
#include <signal.h>
#include "crinkle.h"
#include "paint.h"
#include "global.h"
#include "patchlevel.h"
#include "copyright.h"

#define VERSION 2
#define SIDE 1.0

char scroll_Id[]="$Id: xmountains.c,v 1.36 1997/03/24 11:37:33 spb Exp $";

extern char *display;
extern char *geom;

/*{{{my version on getopt*/
int optind=1;
char *optarg;
int opterr=1;

int my_getopt (argc, argv, pat)
int argc;
char **argv;
char *pat;
{
  char *flag;
  
  if((optind >= argc) || (argv[optind][0] != '-'))
  {
    return -1;
  }
  if( argv[optind][1] == '-' )
  {
    optind++;
    return -1;
  }
  if( argv[optind][1] == ':' )
  {
    if( opterr )
    {
      fprintf(stderr,"getopt: found \":\" in optstring\n");
    }
    return '?';
  }
  for(flag=pat;*flag;flag++)
  {
    if( *flag == argv[optind][1] )
    {
      optind++;
      if( *(flag+1) == ':' )
      {
        if(optind >= argc )
        {
          if( opterr )
          {
            fprintf(stderr,"getopt: no option for flag %c\n",*flag);
          }
          return '?';
        }
        optarg = argv[optind];
        optind++;
      }
      return *flag;
    }
      
  }
  if( opterr )
  {
    fprintf(stderr,"getopt: flag %s not recognized\n",argv[optind]);
  }
  optind++;
  return '?';
}
/*}}}*/

/*{{{  Col *next_col(int paint, int reflec) */
Col *next_col (paint, reflec)
int paint;
int reflec;
{
  Col *res;
  int i,offset=0;
  
  /*{{{  update strips */
  if(paint)
  {
    if(reflec)
    {
      res = mirror( a_strip,b_strip,shadow);
    }else{
      res = camera( a_strip,b_strip,shadow);
    }
  }else{
    res = makemap(a_strip,b_strip,shadow);
  }
  free(a_strip);
  a_strip=b_strip;
  b_strip = extract( next_strip(top) );
  /*}}}*/

  /*{{{update the shadows*/
  /* shadow_slip is the Y component of the light vector.
   * The shadows can only step an integer number of points in the Y
   * direction so we maintain shadow_register as the deviation between
   * where the shadows are and where they should be. When the magnitude of
   * this gets larger then 1 the shadows are slipped by the required number of
   * points.
   * This will not work for very oblique angles so the horizontal angle
   * of illumination should be constrained.
   */
  shadow_register += shadow_slip;
  if( shadow_register >= 1.0 )
  {
    /*{{{negative offset*/
    while( shadow_register >= 1.0 )
    {
      shadow_register -= 1.0;
      offset++;
    }
    for(i=width-1 ; i>=offset ; i--)
    {
      shadow[i] = shadow[i-offset]-delta_shadow;
      if( shadow[i] < b_strip[i] )
      {
        shadow[i] = b_strip[i];
      }
      /*{{{  stop shadow at sea level */
      if( shadow[i] < sealevel )
      {
        shadow[i] = sealevel;
      }
      /*}}}*/
    }
    for(i=0;i<offset;i++)
    {
      shadow[i] = b_strip[i];
      /*{{{  stop shadow at sea level*/
      if( shadow[i] < sealevel )
      {
        shadow[i] = sealevel;
      }
      /*}}}*/
    }
    /*}}}*/
  }else if( shadow_register <= -1.0 ){
    /*{{{positive offset*/
    while( shadow_register <= -1.0 )
    {
      shadow_register += 1.0;
      offset++;
    }
    for(i=0 ; i<width-offset ; i++)
    {
      shadow[i] = shadow[i+offset]-delta_shadow;
      if( shadow[i] < b_strip[i] )
      {
        shadow[i] = b_strip[i];
      }
      /*{{{  stop shadow at sea level */
      if( shadow[i] < sealevel )
      {
        shadow[i] = sealevel;
      }
      /*}}}*/
    }
    for(;i<width;i++)
    {
      shadow[i] = b_strip[i];
      /*{{{  stop shadow at sea level*/
      if( shadow[i] < sealevel )
      {
        shadow[i] = sealevel;
      }
      /*}}}*/
    }
    /*}}}*/
  }else{
    /*{{{no offset*/
    for(i=0 ; i<width ; i++)
    {
      shadow[i] -= delta_shadow;
      if( shadow[i] < b_strip[i] )
      {
        shadow[i] = b_strip[i];
      }
      /*{{{  stop shadow at sea level */
      if( shadow[i] < sealevel )
      {
        shadow[i] = sealevel;
      }
      /*}}}*/
    }
    /*}}}*/
  }
  /*}}}*/
  
  return(res);
}
/*}}}*/
double atof();
#ifdef ANSI
void init_graphics (int, int, int *, int *, int, Gun *, Gun *, Gun *);
void clear_col( int );
void finish_graphics();
void plot_pixel (int, int, unsigned char);
void scroll_screen ( int );
void zap_events( int );
#else
void init_graphics ();
void clear_col();
void finish_graphics();
void plot_pixel ();
void scroll_screen ();
void zap_events();
#endif

void finish_prog();

int s_height=768, s_width=1024;
int mapwid;


/*{{{void plot_column(p,map,reflec,snooze)*/
void plot_column(p,map,reflec,snooze)
int p;
int map;
int reflec;
int snooze;
{
  Col *l;
  int j;
  
  l = next_col(1-map,reflec); 
  if( map )
  {
    for( j=0 ;j<(s_height-mapwid); j++)
    {
      plot_pixel(p,((s_height-1)-j),BLACK);
    }
    for(j=0; j<mapwid ; j++)
    {
      plot_pixel(p,((mapwid-1)-j),l[j]);
    }
  }else{
    for(j=0 ; j<height ; j++)
    {
      /* we assume that the scroll routine fills the
       * new region with a SKY value. This allows us to
       * use a testured sky for B/W displays
       */
      if( l[j] != SKY )
      {
        plot_pixel(p,((s_height-1)-j),l[j]);
      }
    }
  }
  free(l);
  flush_region(p,0,1,height,p,0);
  zap_events(snooze);
}
/*}}}*/

main (argc,argv)
int argc;
char **argv;
{
  int i,p;

  int repeat=20;
  int map = 0;
  int reflec = 0;
  int root= 0;

  int c, errflg=0;
  extern char *optarg;
  extern int optind;
  char *mesg[2];
  Gun *clut[3];
  FILE *pidfile;

  /*{{{handle command line flags*/
  mesg[0]="false";
  mesg[1]="true";
  while((c = my_getopt(argc,argv,"bxmqMEHl:r:f:t:I:A:S:T:W:C:a:p:B:n:R:g:d:c:e:v:Z:s:X:Y:P:F:G:"))!= -1)
  {
    switch(c){
      case 'b':
        root = 1- root;
        break;                      /* run on root window */
      case 'x':
        cross = 1- cross;
        break;                      /* use cross updates */
      case 'E':
        e_events = 1 - e_events;
        break;
      case 'q':
        request_clear = 1 - request_clear;
        break;
      case 'm':                     /* Map view only */
        map = 1 - map;
        break;
      case 'M':                     /* put in reflections */
        reflec = 1 - reflec;
        break;
      case 'l':                     /* Set # levels of recursion */
         levels = atoi( optarg );
         if( levels < 2 )
         {
           levels = 2;
         }
         break;
      case 'F':                     /* Set # levels to force front to mean */
         slope = atoi( optarg );
         break;
      case 's':                     /* Set smoothing parameter */
         smooth = atoi( optarg );
         break;
      case 't':                     /* Set width of lowest level */
         stop = atoi( optarg );
         if( stop < 0 )
         {
           stop = 0;
         }
         break;
      case 'r':
         repeat = atoi( optarg );
         if( repeat < 0 )
         {
           repeat = -repeat;
           i= -1;
         }else{
           i=1;
         }
         /* we want repeat to be a multiple of 2 as we are using
          * a textured field for the sky.
          */
         repeat = i*(2 * ((repeat +1)/2));
         break;
      case 'B':                     /* set band_size */
         band_size = atoi( optarg );
         if( band_size < 2 )
         {
           band_size=2;
         }
         n_col = (BAND_BASE + (N_BANDS * band_size));
         break;
      case 'n':                     /* set max number of colours */
         n_col = atoi( optarg );
         if( n_col < MIN_COL )
         {
           n_col = MIN_COL;
         }
         band_size = (n_col - BAND_BASE)/N_BANDS;
         n_col = (BAND_BASE + (N_BANDS * band_size));
         break;
      case 'R':                     /* set seed, read clock if 0 */
         seed = atoi( optarg );
         break;
      case 'Z':                     /* put sleep into wait events */
         snooze_time = atoi( optarg );
         if( snooze_time < 0 )
         {
           snooze_time = 0;
         }
         break;
      case 'P':
         pidfile = fopen(optarg,"w");
         if( pidfile )
         {
           fprintf(pidfile,"%d\n",getpid());
           fclose(pidfile);
         }else{
           perror(optarg);
         }
         break;
      case 'f':                     /* set fractal dimension */
         fdim = atof( optarg );
         if( fdim < 0.5 )
         {
          fdim=0.5;
         }
         if( fdim > 1.0 )
         {
          fdim=1.0;
         }
         break;
      case 'I':                     /* set Illumination angle */
         phi = ((PI * atof( optarg ))/180.0);
         if ( phi < 0.0 )
         {
           phi=0.0;
         }
         if( phi > PI/2.0 )
         {
           phi = PI/2.0;
         }
         break;
      case 'A':                     /* set Illumination angle (horizontal)*/
         alpha = ((PI * atof( optarg ))/180.0);
         if( alpha < -PI/3.0 )
         {
           alpha = -PI/3.0;
         }
         if( alpha > PI/3.0 )
         {
           alpha = PI/3.0;
         }
         break;
      case 'X':                     /* set mix */
         mix = atof( optarg );
         break;
      case 'Y':                     /* set midmix */
         midmix = atof( optarg );
         break;
      case 'S':                     /* set stretch */
         stretch = atof( optarg );
         break;
      case 'W':                     /* set sealevel */
         sealevel = atof( optarg );
         break;
      case 'G':                     /* set forceheight */
         forceheight = atof( optarg );
         break;
      case 'T':                     /* set shift */
         shift = atof( optarg );
         break;
      case 'C':
         contour = atof( optarg );
         break;
      case 'a':                     /* set altitude */
         altitude = atof( optarg );
         break;
      case 'p':                     /* set distance */
         distance = atof( optarg );
         break;
      case 'c':
         contrast = atof( optarg );
         if( contrast < 0.0 )
         {
          contrast=0.0;
         }
         break;
      case 'e':
         ambient = atof( optarg );
         if( ambient < 0.0 )
         {
          ambient = 0.0;
         }
         if( ambient > 1.0 )
         {
          ambient=1.0;
         }
         break;
      case 'v':
         vfract = atof( optarg );
         if( vfract < 0.0 )
         {
          vfract = 0.0;
         }
         break;
      case 'g':
         geom = optarg;
         break;
      case 'd':
         display = optarg;
         break;
      case 'H':
         print_algorithm();
         errflg++;
         break;
      case '?':
         errflg++;
    }
  }
  if( errflg )
  {
    fprintf(stderr,"%s: version %d.%d\n",argv[0],VERSION,PATCHLEVEL);
    fprintf(stderr,"usage: %s -[bqgdPEmMrBZIASFTCapcevfRltxsXYH]\n",argv[0]);
    fprintf(stderr," -b       [%s] use root window \n",mesg[root]);
    fprintf(stderr," -q       [%s] reset root window on exit\n",mesg[request_clear]);
    fprintf(stderr," -g string     window geometry\n");
    fprintf(stderr," -d string     display\n");
    fprintf(stderr," -P filename   write PID to file\n");
    fprintf(stderr," -E       [%s] toggle explicit expose events \n",mesg[e_events]);
    fprintf(stderr," -m       [%s] print map \n",mesg[map]);
    fprintf(stderr," -M       [%s] implement reflections \n",mesg[reflec]);
    fprintf(stderr," -r int   [%d] # columns before scrolling \n",repeat);
    fprintf(stderr," -B int   [%d] # shades in a colour band\n",band_size);
    fprintf(stderr," -n int   [%d] # number of colours\n",n_col);
    fprintf(stderr," -Z int   [%d] time to sleep before scrolling\n",snooze_time);
    fprintf(stderr," -I float [%f] vertical angle of light \n",(phi*180.0)/PI);
    fprintf(stderr," -A float [%f] horizontal angle of light \n",(alpha*180.0)/PI);
    fprintf(stderr," -S float [%f] vertical stretch \n",stretch);
    fprintf(stderr," -T float [%f] vertical shift \n",shift);
    fprintf(stderr," -W float [%f] sealevel \n",sealevel);
    fprintf(stderr," -F int   [%d] reduce variation in the foreground \n",slope);
    fprintf(stderr," -G float [%f] average foreground height \n",forceheight);
    fprintf(stderr," -C float [%f] contour parameter \n",contour);
    fprintf(stderr," -a float [%f] altitude of viewpoint \n",altitude);
    fprintf(stderr," -p float [%f] distance of viewpoint \n",distance);
    fprintf(stderr," -c float [%f] contrast\n",contrast);
    fprintf(stderr," -e float [%f] ambient light level\n",ambient);
    fprintf(stderr," -v float [%f] vertical light level\n",vfract);
    fprintf(stderr,"Fractal options:\n");
    fprintf(stderr," -f float [%f] fractal dimension \n",fdim);
    fprintf(stderr," -R int   [%d] rng seed, read clock if 0 \n",seed);
    fprintf(stderr," -l int   [%d] # levels of recursion \n",levels);
    fprintf(stderr," -t int   [%d] # non fractal iterations \n",stop);
    fprintf(stderr," -x       [%s] cross update \n",mesg[cross]);
    fprintf(stderr," -s       [%x] smoothing (0-7)\n",smooth);
    fprintf(stderr," -X float [%f] fraction of old value for rg2 & rg3\n",mix);
    fprintf(stderr," -Y float [%f] fraction of old value for rg1\n",midmix);
    fprintf(stderr," -H            print short description of algorithm.\n");
    exit(1);
  }
  /*}}}*/
  for(i=0 ;i<3 ;i++)
  {
    clut[i] = (Gun *) malloc(n_col * sizeof(Gun));
    if( ! clut[i] )
    {
      fprintf(stderr,"malloc failed for clut\n");
      exit(1);
    }
  }
  set_clut(n_col,clut[0], clut[1], clut[2]);
  init_graphics(root,(! e_events),request_clear,&s_width,&s_height,n_col,clut[0],clut[1],clut[2]);
  for(i=0;i<3;i++)
  {
    free(clut[i]);
  }

  height = s_height;
    
  seed_uni(seed);

  init_artist_variables();
  if( -1 == (int) signal(SIGINT, finish_prog ))
  {
    perror(argv[0]);
    exit(1);
  }
  if( -1 == (int) signal(SIGTERM, finish_prog ))
  {
    perror(argv[0]);
    exit(1);
  }
  if( -1 == (int) signal(SIGHUP, finish_prog ))
  {
    perror(argv[0]);
    exit(1);
  }
  if( -1 == (int) signal(SIGQUIT, finish_prog ))
  {
    perror(argv[0]);
    exit(1);
  }

  if( s_height > width )
  {
    mapwid=width;
  }else{
    mapwid=s_height;
  }
  if( repeat >= 0 )
  {
    for(p=0 ; p < s_width ; p++)
    {
      plot_column(p,map,reflec,0);
    }
  }else{
    for(p=s_width-1 ; p >=0 ; p--)
    {
      plot_column(p,map,reflec,0);
    }
  }
  while( TRUE )
  {
    if( repeat == 0)
    {
      for(p=0 ; p < s_width-1 ; p++)
      {
        /* plot routines assume we are starting with sky */
        blank_col(p);
        plot_column(p,map,reflec,0);
      }
    }else{
      /* do the scroll */
      scroll_screen(repeat);
      if(repeat > 0)
      {
        for( p = s_width - repeat ; p < (s_width-1) ; p++ )
        {
          plot_column(p,map,reflec,0);
        }
      }else{
        for( p = -1 - repeat ; p >=0 ; p-- )
        {
          plot_column(p,map,reflec,0);
        }
      }
    }
    plot_column(p,map,reflec,snooze_time);
  }
}

    
extern int quit_xmount;

void finish_prog()
{
  /* The next time zap_events is called the program will quit */
  quit_xmount=TRUE;
}

