#include <stdio.h>
#include <signal.h>
#include "crinkle.h"
#include "paint.h"
#include "global.h"
#include "patchlevel.h"
#include "copyright.h"

#define SIDE 1.0
#define VERSION 1

char scroll_Id[]="$Id: xmountains.c,v 1.17 1994/02/07 14:08:15 spb Exp $";

extern char *display;
extern char *geom;

/*{{{my version on getopt*/
int optind=1;
char *optarg;
int opterr=1;

int getopt (argc, argv, pat)
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

/*{{{  Col *next_col(int paint) */
Col *next_col (paint)
int paint;
{
  Col *res;
  int i;
  
  /*{{{  update strips */
  if(paint)
  {
    res = camera( a_strip,b_strip,shadow);
  }else{
    res = makemap(a_strip,b_strip,shadow);
  }
  free(a_strip);
  a_strip=b_strip;
  b_strip = extract( next_strip(top) );
  /*}}}*/

  for(i=0 ; i<width ; i++)
  {
    shadow[i] -= (tan_phi * SIDE);
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
  return(res);
}
/*}}}*/
double atof();
#ifdef ANSI
void init_graphics (int, int *, int *, int, Gun *, Gun *, Gun *);
void finish_graphics();
void plot_pixel (int, int, unsigned char);
void scroll_screen ( int );
#else
void init_graphics ();
void finish_graphics();
void plot_pixel ();
void scroll_screen ();
#endif

void finish_prog();

int s_height=768, s_width=1024;
int mapwid;


void plot_column(p,map)
int p;
int map;
{
  Col *l;
  int j;
  
  l = next_col(1-map); 
  if( map )
  {
    for(j=0; j<mapwid ; j++)
    {
      plot_pixel(p,((mapwid-1)-j),l[j]);
    }
    for( j=0 ;j<(s_height-mapwid); j++)
    {
      plot_pixel(p,((s_height-1)-j),BLACK);
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
    free(l);
  }
  flush_region(p,0,1,height,p,0);
  zap_events();
}

main (argc,argv)
int argc;
char **argv;
{
  int i,j,p,code;

  int repeat=20;
  int map = 0;
  int root= 0;

  int c, errflg=0;
  extern char *optarg;
  extern int optind;
  char *mesg[2];
  Gun red[MAX_COL], green[MAX_COL], blue[MAX_COL];

  /*{{{handle command line flags*/
  mesg[0]="false";
  mesg[1]="true";
  while((c = getopt(argc,argv,"bxmsl:r:f:t:I:S:T:C:a:p:R:g:d:c:e:v:"))!= -1)
  {
    switch(c){
      case 'b':
        root = 1- root;
        break;                      /* run on root window */
      case 's':                     /* Toggle smoothing */
        smooth = 1 - smooth;
        break;
      case 'x':                     /* Toggle fractal Start */
        frac_start = 1 - frac_start;
        break;
      case 'm':                     /* Map view only */
        map = 1 - map;
        break;
      case 'l':                     /* Set # levels of recursion */
         levels = atoi( optarg );
         if( levels < 2 )
         {
           levels = 2;
         }
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
         if( repeat < 1 )
         {
           repeat = 1;
         }
         /* we want repeat to be a multiple of 2 as we are using
          * a textured field for the sky.
          */
         repeat = (2 * ((repeat +1)/2));
         break;
      case 'R':                     /* set seed, read clock if 0 */
         seed = atoi( optarg );
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
      case 'S':                     /* set stretch */
         stretch = atof( optarg );
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
      case '?':
         errflg++;
    }
  }
  if( errflg )
  {
    fprintf(stderr,"%s: version %d.%d\n",argv[0],VERSION,PATCHLEVEL);
    fprintf(stderr,"usage: %s -[bxmslrftISTRapcegd]\n",argv[0]);
    fprintf(stderr," -b       [%s] use root window \n",mesg[root]);
    fprintf(stderr," -x       [%s] flat start \n",mesg[1-frac_start]);
    fprintf(stderr," -m       [%s] print map \n",mesg[map]);
    fprintf(stderr," -s       [%s] toggle smoothing \n",mesg[smooth]);
    fprintf(stderr," -l int   [%d] # levels of recursion \n",levels);
    fprintf(stderr," -t int   [%d] # non fractal iterations \n",stop);
    fprintf(stderr," -r int   [%d] # columns before scrolling \n",repeat);
    fprintf(stderr," -R int   [%d] rng seed, read clock if 0 \n",seed);
    fprintf(stderr," -f float [%f] fractal dimension \n",fdim);
    fprintf(stderr," -I float [%f] angle of light \n",(phi*180.0)/PI);
    fprintf(stderr," -S float [%f] vertical stretch \n",stretch);
    fprintf(stderr," -T float [%f] vertical shift \n",shift);
    fprintf(stderr," -C float [%f] contour parameter \n",contour);
    fprintf(stderr," -a float [%f] altitude of viewpoint \n",altitude);
    fprintf(stderr," -p float [%f] distance of viewpoint \n",distance);
    fprintf(stderr," -c float [%f] contrast\n",contrast);
    fprintf(stderr," -e float [%f] ambient light level\n",ambient);
    fprintf(stderr," -v float [%f] vertical light level\n",vfract);
    fprintf(stderr," -g string     window geometry\n");
    fprintf(stderr," -d string     display\n");
    exit(1);
  }
  /*}}}*/
  set_clut(red, green, blue);
  init_graphics(root,&s_width,&s_height,MAX_COL,red,green,blue);

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
  for(p=0 ; p < s_width ; p++)
  {
    plot_column(p,map);
  }
  while( TRUE )
  {
      /* do the scroll */
      scroll_screen(repeat);
      for( p = s_width - repeat ; p < s_width ; p++ )
      {
        plot_column(p,map);
      }
  }
}

    
extern int quit_xmount;

void finish_prog()
{
  /* The next time zap_events is called the program will quit */
  quit_xmount=TRUE;
}

