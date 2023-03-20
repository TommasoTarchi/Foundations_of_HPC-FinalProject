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
                printf("\nargument -%c not known\n\n", c);
                break;

        }
    }


    BOOL* grid = NULL;
    const int n_cells = k*k;





    /* initializing grid */
    if (action == INIT) {

        /* assigning default name to file in case none was passed */
        if (fname == NULL) {

            printf("-- no output file was passed - initial conditions will be written to %s\n\n", fname_deflt);

            fname = (char*) malloc(sizeof(fname_deflt));
            sprintf(fname, "%s", fname_deflt);
        }

         
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


        // sostituire con scrittura su PMG
        if (grid != NULL) {
            for (int i=0; i<k; i++) {
                for (int j=0; j<k; j++)
                    printf("%c  ", grid[i*k+j]);
                printf("\n");
            }
            printf("\n");
        }


    }

    



    /* running game of life */
    if (action == RUN) {

        /* assigning default name to file in case none was passed */
        if (fname == NULL) {

            printf("-- no file with initial playground was passed - the program will try to read from %s\n\n", fname_deflt);

            fname = (char*) malloc(sizeof(fname_deflt));
            sprintf(fname, "%s", fname_deflt);
        }

        // aggiungere lettura da PMG
        //
        //
        //
        //


       


//////// just for testing   //////////////////////////////////



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


       
        if (grid != NULL) {
            for (int i=0; i<k; i++) {
                for (int j=0; j<k; j++)
                    printf("%c  ", grid[i*k+j]);
                printf("\n");
            }
            printf("\n");
        }



//////////////////////////////////////////////////////////






        if (e == ORDERED) {

            for (int gen=0; gen<n; gen++) {

                /* iteration on rows */
                for (int i=1; i<k-1; i++) {
                    /* iteration on columns */
                    for (int j=1; j<k-1; j++) {

                        short int count = 0;
                        for (int a=i-1; a<i+2; a++) {
                            for (int b=j-1; b<j+2; b++) {
                                if (grid[a*k+b] == '1') {
                                    count++;
                                }
                            }
                        }

                        if (grid[i*k+j] == '1')
                            count--;

                        if (count == 2 || count == 3) {
                            grid[i*k+j] = '1';
                        } else {
                            grid[i*k+j] = '0';
                        }
                    }
                }

            }

        } else if (e == STATIC) {

            printf("static\n");

        }




////////////// just for testing ///////////////////

if (grid != NULL) {
            for (int i=0; i<k; i++) {
                for (int j=0; j<k; j++)
                    printf("%c  ", grid[i*k+j]);
                printf("\n");
            }
            printf("\n");
        }

    
//////////////////////////////////////////////////////



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
