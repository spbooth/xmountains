#ifndef RANDOM_H
#define RANDOM_H
float uni();
void rstart(int i,int j,int k,int l);
void seed_uni(int ijkl);
float gaussian();
#define MAX_SEED 900000000
#endif
