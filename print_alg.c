#include <stdio.h>

#define P(A) fprintf(stderr,"%s\n",A)
void print_algorithm()
{
P("This program uses a modified form of the mid-point displacement algorithm");
P("");
P("The mid-point displacement algorithm is a recursive algorithm, each iteration");
P("doubles the resolution of the grid. This is done in 2 stages.");
P("");
P("A        B                  A           B                A     F     B");
P("               stage1                        stage2");
P("              --------->          E         -------->    G     E     H");
P("");
P("C        D                  C           D                C     I     D");
P("");
P("The new points are generated by taking an average of the surrounding points");
P("and adding a random offset.");
P("The modifications to the standard algorithm are as follows:");
P("There are three optional regeneration steps to reduce \"creasing\". A");
P("regeneration step recalculates the height of existing points using an");
P("average and offset from a newer generation of points. The three");
P("regeneration steps are:");
P("  rg1:  recalculate corner points (A,B,C,D) from the midpoints (E)");
P("        after the stage1 update.");
P("  rg2:  recalculate midpoints (E) from the edge points (F,G,H,I)");
P("        after the stage2 update");
P("  rg3:  recalculate corner points (A,B,C,D) from the edge points (F,G,H,I)");
P("        after the stage2 update");
P("The regeneration stages are turned on by the smoothing parameter (-s flag)");
P("");
P("  flag    rg3   rg2   rg1");
P("  0       off   off   off");
P("  1       on    off   off");
P("  2       off   on    off");
P("  3       on    on    off");
P("  4       off   off   on");
P("  5       on    off   on");
P("  6       off   on    on");
P("  7       on    on    on");
P("");
P("When performing the regeneration steps the random offset is added to a");
P("weighted average of the previous value of the point and a the average of");
P("the new points. The weighting factors are controlled by the -X and -Y flags.");
P("");
P("The -x flag (cross update) controls whether the midpoints (E) are included");
P("in the average when performing the stage2 update or if only the corner");
P("points are used.");
P("");
}
