#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
 

#define INIT 1
#define RUN  2

#define K_DFLT 100

#define ORDERED 0
#define STATIC  1

#define BOOL char


char fname_deflt[] = "game_of_life.pgm";

int   action = 0;
int   k      = K_DFLT;
int   e      = ORDERED;
int   n      = 10000;
int   s      = 1;
char *fname  = NULL;



int main(int argc, char **argv) {


    /* getting options */
    int action = 0;
    char *optstring = "irk:e:f:n:s:";

    int c;
    while ((c = getopt(argc, argv, optstring)) != -1) {
    
        switch(c) {
            case 'i':
                action = INIT;
                break;

            case 'r':
                action = RUN;
                break;
            
            case 'k':
                k = atoi(optarg);
                break;
            
            case 'e':
                e = atoi(optarg);
                break;
            
            case 'f':
                fname = (char*) malloc(sizeof(optarg)+1);
                sprintf(fname, "%s", optarg);
                break;
            
            case 'n':
                n = atoi(optarg);
                break;
            
            case 's':
                s = atoi(optarg);
                break;
            
            default:
                printf("argument -%c not known\n\n-- we suggest to rerun the program with the right arguments\n\n", c);
                break;

        }
    }


    BOOL* grid = NULL;
    const int n_cells = k*k;


    /* initializing grid */
    if (action == INIT) {

        if (fname != NULL) {

            // add PMG-read

        /* if no file name is passed the grid is initialized randomly */
        } else {

            printf("-- no playground to read was passed - it will be randomly initialized\n\n");
         
            srand(time(NULL));

            grid = (BOOL*) malloc(n_cells*sizeof(BOOL));

            double randmax_inv = 1.0/RAND_MAX;
            for (int i=0; i<n_cells; i++) {
                /* producing a random number between 0 and 1 */
                double temp = rand()*randmax_inv;
                /* producing a random integer among 48 and 49
                 * (ASCII codes of '0' and '1') */
                int rand_bool = (int)(temp + 48.5);
                /* converting random number to char */
                grid[i] = (BOOL)rand_bool;
            }

        }
    }
    


    if (grid != NULL) {
        for (int i=0; i<k; i++) {
            for (int j=0; j<k; j++)
                printf("%c  ", grid[i*k+j]);
            printf("\n");
        }
    }






    if (fname != NULL) {
        free(fname);
        fname = NULL;
    }

    if (grid != NULL) {
        free(grid);
        grid = NULL;
    }

    return 0;
}
