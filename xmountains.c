#include <stdio.h>
#include <signal.h>
#include "crinkle.h"
#include "paint.h"
#include "global.h"

#define SIDE 1.0

char scroll_Id[]="$Id: xmountains.c,v 1.4 1994/01/07 19:38:39 spb Exp $";

/*{{{  Col *next_col(int paint) */
Col *next_col(int paint)
{
  Col *res;
  Col *map;
  int i;
  
  map = artist(a_strip,b_strip,shadow);
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
  /*{{{  update strips */
  if(paint)
  {
    res = camera( a_strip, map );
    free(map);
  }else{
    res = map;
  }
  free(a_strip);
  a_strip=b_strip;
  b_strip = extract( next_strip(top) );
  /*}}}*/
  return(res);
}
/*}}}*/
double atof();
void init_graphics(int, int *, int *);
void finish_graphics();
void install_clut( int, unsigned char *, unsigned char *, unsigned char *);
void plot_pixel(int, int, unsigned char);
void scroll_screen( int );

void finish_prog();

main(int argc, char **argv)
{
  int s_height=768, s_width=1024;
  int i,j,p,code;
  Col *l;

  int repeat=20;
  int map = 0;
  int root= 0;

  int c, errflg=0;
  extern char *optarg;
  extern int optind;
  char *mesg[2];

  mesg[0]="false";
  mesg[1]="true";
  while((c = getopt(argc,argv,"bxmsl:r:f:t:I:S:T:a:d:R:w:h:"))!= -1)
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
         break;
      case 'R':                     /* set seed, read clock if 0 */
         seed = atoi( optarg );
         break;
      case 'f':                     /* set fractal dimension */
         fdim = atof( optarg );
         break;
      case 'I':                     /* set Illumination angle */
         phi = atof( optarg );
         break;
      case 'S':                     /* set stretch */
         stretch = atof( optarg );
         break;
      case 'T':                     /* set shift */
         shift = atof( optarg );
         break;
      case 'a':                     /* set altitude */
         altitude = atof( optarg );
         break;
      case 'd':                     /* set distance */
         distance = atof( optarg );
         break;
      case 'w':
         s_width = atoi( optarg );
         break;
      case 'h':
         s_height = atoi( optarg );
         break;
      case '?':
         errflg++;
    }
  }
  if( errflg )
  {
    fprintf(stderr,"%s: illegal argument\n",argv[0]);
    fprintf(stderr,"usage, %s -[bxmslrftISTRadhw]\n",argv[0]);
    fprintf(stderr," -b       [%s] use root window \n",mesg[root]);
    fprintf(stderr," -x       [%s] flat start \n",mesg[1-frac_start]);
    fprintf(stderr," -m       [%s] print map \n",mesg[map]);
    fprintf(stderr," -s       [%s] toggle smoothing \n",mesg[smooth]);
    fprintf(stderr," -l int   [%d] # levels of recursion \n",levels);
    fprintf(stderr," -t int   [%d] # non fractal iterations \n",stop);
    fprintf(stderr," -r int   [%d] # columns before scrolling \n",repeat);
    fprintf(stderr," -R int   [%d] rng seed, read clock if 0 \n",seed);
    fprintf(stderr," -f float [%f] fractal dimension \n",fdim);
    fprintf(stderr," -I float [%f] angle of light \n",phi);
    fprintf(stderr," -S float [%f] vertical stretch \n",stretch);
    fprintf(stderr," -T float [%f] vertical shift \n",shift);
    fprintf(stderr," -a float [%f] altitude of viewpoint \n",altitude);
    fprintf(stderr," -d float [%f] distance of viewpoint \n",distance);
    fprintf(stderr," -w int   [%d] width of screen \n",s_width);
    fprintf(stderr," -h int   [%d] height of screen \n",s_height);
    exit(1);
  }
  init_graphics(root,&s_width,&s_height);
  set_clut();
  install_clut(MAX_COL+1,red,green,blue);

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

  while( TRUE )
  {
      /* do the scroll */
      scroll_screen(repeat);
      for( p = s_width - repeat ; p < s_width ; p++ )
      {
        l = next_col(1-map); 
        if( map )
        {
          for(j=0; (j<height)&&(j<width) ; j++)
          {
            plot_pixel(p,((s_height-1)-j),l[j]);
          }
          for( ;j<height; j++)
          {
            plot_pixel(p,((s_height-1)-j),BLACK);
          }
        }else{
          for(j=0 ; j<height ; j++)
          {
            plot_pixel(p,((s_height-1)-j),l[j]);
          }
          free(l);
        }
        flush_region(p,0,1,height,p,0);
        zap_events();
      }
  }
  finish_prog();
}
    
void finish_prog()
{
  finish_graphics();
  finish_artist();
  exit(0);
}
