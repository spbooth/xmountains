
#include <stdio.h>
#include <signal.h>
#include "crinkle.h"

Parm fold_param;

#include <sys/time.h>

long get_millis()
{

  struct timeval ts;
    long t;
      int err;

        err = gettimeofday(&ts, NULL);

          t = (ts.tv_sec *1000)  +  ts.tv_usec/1000 ;

            return t;

}

void main(int argc, char *argv[]){
  Fold *f;
  int i;
  double sum;
  Strip *s;
  long stop,start;


  seed_uni(121265);
  f = make_fold(NULL,&fold_param,10,2,1.0);
  for(i=0;i<1000;i++){
    s = next_strip(f);
    free_strip(s);
  }
  start = get_millis();
  for(i=0;i<1000;i++){
    s = next_strip(f);
    free_strip(s);
  }  
  stop = get_millis();
 printf("time was %ld \n",(stop - start));  
}
