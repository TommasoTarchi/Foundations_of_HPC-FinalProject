#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>



/* time definition */
#ifdef TIME
#define CPU_TIME (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts), (double) ts.tv_sec + (double) ts.tv_nsec * 1e-9)
#endif



/* 'actual' code definitions */
#define INIT 1
#define RUN  2

#define M_DFLT 100
#define K_DFLT 100

#define ORDERED 0
#define STATIC  1
#define STATIC_SAVE_MEM 2

#define BOOL char

char fname_deflt[] = "../../images/game_of_life.pgm";

int   action = ORDERED;
int   m      = M_DFLT;
int   k      = K_DFLT;
int   e      = ORDERED;
int   n      = 10000;
int   s      = 1;
char *fname  = NULL;



void write_pgm_image(BOOL*, const int, int, int, const char*, int);
void read_pgm_image(BOOL**, int*, int*, int*, const char*);



int main(int argc, char **argv) {



/* needed for timing */
#ifdef TIME
    struct timespec ts;
#endif



    /* getting options */
    int action = 0;
    char *optstring = "irm:k:e:f:n:s:";

    int c;
    while ((c = getopt(argc, argv, optstring)) != -1) {
    
        switch(c) {

            case 'i':
                action = INIT;
                break;

            case 'r':
                action = RUN;
                break;

            case 'm':
                m = atoi(optarg);
                break;
            
            case 'k':
                k = atoi(optarg);
                break;
            
            case 'e':
                e = atoi(optarg);
                break;
            
            case 'f':
                fname = (char*) malloc(50);
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


    /* initializing pointer to the system */
    BOOL* grid = NULL;


    /* initializing a playground */
    if (action == INIT) {


        /* assigning default name to file in case none was passed */
        if (fname == NULL) {

            printf("-- no output file was passed - initial conditions will be written to %s\n\n", fname_deflt);

            fname = (char*) malloc(sizeof(fname_deflt));
            sprintf(fname, "%s", fname_deflt);
        }


        /* initializing grid to random booleans */
        const unsigned int n_cells = m*k;
        grid = (BOOL*) malloc(n_cells*sizeof(BOOL));

        srand(time(NULL));
        double randmax_inv = 1.0/RAND_MAX;
        for (int i=0; i<n_cells; i++) {
            /* producing a random number between 0 and 1 */
            short int rand_bool = (short int) (rand()*randmax_inv+0.5);
            /* converting random number to char */
            grid[i] = (BOOL) rand_bool;
        }

        /* writing down playground */
        const int maxval_color = 1;
        write_pgm_image(grid, maxval_color, k, m, fname, 1);

    }



    /* running game of life */
    if (action == RUN) {


        /* assigning default name to file in case none was passed */
        if (fname == NULL) {

            printf("-- no file with initial playground was passed - the program will try to read from %s\n\n", fname_deflt);

            fname = (char*) malloc(sizeof(fname_deflt));
            sprintf(fname, "%s", fname_deflt);
        }

        
        /* reading initial playground from pmg file */
        int maxval_color;
        int x_size;
        int y_size;
        
        read_pgm_image(&grid, &maxval_color, &x_size, &y_size, fname);        
        
        const unsigned int n_cells = x_size*y_size;

    
        
        ///////// just for test (random grid) /////////
        ///////////////////////////////////////////////
        //
        //const int x_size = 20;
        //const int y_size = 10;
        //const int n_cells = x_size*y_size;
        //grid = (BOOL*) malloc(n_cells*sizeof(BOOL));
        //
        //srand(time(NULL));
        //double randmax_inv = 1.0/RAND_MAX;
        //for (int i=0; i<n_cells; i++) {
        //    short int rand_bool = (short int) (rand()*randmax_inv+0.5);
        //    grid[i] = (BOOL) rand_bool;
        //}
        //
        ///////////////////////////////////////////////


        //////////////// just for test ////////////////
        ///////////////////////////////////////////////
        //
        //for (int i=0; i<y_size; i++) {
        //    for (int j=0; j<x_size; j++)
        //        printf("%c ", grid[i*x_size+j]+48);
        //    printf("\n");
        //}
        //
        //printf("\n");
        //
        ///////////////////////////////////////////////


        
        /* string to store the name of the snapshot files */
        char* snap_name = (char*) malloc(50*sizeof(char));



        /* ordered evolution */
        if (e == ORDERED) {


            printf("using ordered evolution\n\n");


#ifdef TIME
    double t_start = CPU_TIME;
#endif


            for (int gen=0; gen<n; gen++) {


                /* updating first row */
                
                /* first element of the first row */
                char count = 0;

                count += grid[1];
                for (int b=x_size-1; b<x_size+2; b++) {
                    count += grid[b];
                }
                count += grid[2*x_size-1];
                for (int b=n_cells-x_size; b<n_cells-x_size+2; b++) {
                    count += grid[b];
                }
                count += grid[n_cells-1];

                if (count == 2 || count == 3) {
                    grid[0] = 1;
                } else {
                    grid[0] = 0;
                }

                /* internal elements of the first row */
                for (int j=1; j<x_size-1; j++) {

                    count = 0;
                    count += grid[j-1];
                    count += grid[j+1];
                    for (int b=j-1; b<j+2; b++) {
                        count += grid[x_size+b];
                    }
                    for (int b=n_cells-x_size+j-1; b<n_cells-x_size+j+2; b++) {
                        count += grid[b];
                    }

                    if (count == 2 || count == 3) {
                        grid[j] = 1;
                    } else {
                        grid[j] = 0;
                    }
                }

                /* last element of the first row */
                count = 0;
                count += grid[0];
                count += grid[x_size-2];
                count += grid[x_size];
                for (int b=2*x_size-2; b<2*x_size; b++) {
                    count += grid[b];
                }
                count += grid[n_cells-x_size];
                for (int b=n_cells-2; b<n_cells; b++) {
                    count += grid[b];
                }

                if (count == 2 || count == 3) {
                    grid[x_size-1] = 1;
                } else {
                    grid[x_size-1] = 0;
                }


                /* updating internal cells */ 

                /* iteration on internal rows */
                for (int i=1; i<y_size-1; i++) {

                    /* first element of the row */
                    count = 0;
                    for (int b=(i-1)*x_size; b<(i-1)*x_size+2; b++) {
                        count += grid[b];
                    }
                    count += grid[i*x_size-1];
                    count += grid[i*x_size+1];
                    for (int b=(i+1)*x_size-1; b<(i+1)*x_size+2; b++) {
                        count += grid[b];
                    }
                    count += grid[(i+2)*x_size-1]; 

                    if (count == 2 || count == 3) {
                        grid[i*x_size] = 1;
                    } else {
                        grid[i*x_size] = 0;
                    }

                    /* iteration on internal columns */
                    for (int j=1; j<x_size-1; j++) {

                        count = 0;
                        for (int b=(i-1)*x_size+j-1; b<(i-1)*x_size+j+2; b++) {
                            count += grid[b];
                        }
                        count += grid[i*x_size+j-1];
                        count += grid[i*x_size+j+1];
                        for (int b=(i+1)*x_size+j-1; b<(i+1)*x_size+j+2; b++) {
                            count += grid[b];
                        }

                        if (count == 2 || count == 3) {
                            grid[i*x_size+j] = 1;
                        } else {
                            grid[i*x_size+j] = 0;
                        }
                    }

                    /* last element of the row */
                    count = 0;
                    count += grid[(i-1)*x_size];
                    for (int b=i*x_size-2; b<i*x_size+1; b++) {
                        count += grid[b];
                    }
                    count += grid[(i+1)*x_size-2];
                    count += grid[(i+1)*x_size];
                    for (int b=(i+2)*x_size-2; b<(i+2)*x_size; b++) {
                        count += grid[b];
                    }

                    if (count == 2 || count == 3) {
                        grid[(i+1)*x_size-1] = 1;
                    } else {
                        grid[(i+1)*x_size-1] = 0;
                    }
                }


                /* updating last row */
                
                /* first element of the last row */
                count = 0;
                for (int b=(y_size-2)*x_size; b<(y_size-2)*x_size+2; b++) {
                    count += grid[b];
                }
                count += grid[n_cells-x_size-1];
                count += grid[n_cells-x_size+1];
                count += grid[n_cells-1];
                for (int b=0; b<2; b++) {
                    count += grid[b];
                }
                count += grid[x_size-1];

                if (count == 2 || count == 3) {
                    grid[n_cells-x_size] = 1;
                } else {
                    grid[n_cells-x_size] = 0;
                }

                /* internal elements of the last row */
                for (int j=1; j<x_size-1; j++) {

                    count = 0;
                    for (int b=j-1; b<j+2; b++) {
                        count += grid[n_cells-2*x_size+b];
                    }
                    count += grid[n_cells-x_size+j-1];
                    count += grid[n_cells-x_size+j+1];
                    for (int b=j-1; b<j+2; b++) {
                        count += grid[b];
                    }

                    if (count == 2 || count == 3) {
                        grid[n_cells-x_size+j] = 1;
                    } else {
                        grid[n_cells-x_size+j] = 0;
                    }
                }

                /* last element of the last row */
                count = 0;
                count += grid[n_cells-2*x_size];
                for (int b=n_cells-x_size-2; b<n_cells-x_size+1; b++) {
                    count += grid[b];
                }
                count += grid[n_cells-2];
                count += grid[0];
                for (int b=x_size-2; b<x_size; b++) {
                    count += grid[b];
                }

                if (count == 2 || count == 3) {
                    grid[n_cells-1] = 1;
                } else {
                    grid[n_cells-1] = 0;
                }
           

                /* saving a snapshot of the system */
                if (s != 0) {

                    if (gen % s == 0) {

                        sprintf(snap_name, "../../images/snapshots/snapshot_%05d.pgm", gen+1);
                        
                        write_pgm_image(grid, 1, x_size, y_size, snap_name, 1);
                    }
                }
                
            } 


#ifdef TIME
    double time = CPU_TIME - t_start;
    printf("elapsed time: %f sec\n\n", time);
#endif



        /* static evolution */
        } else if (e == STATIC) {


            printf("using static evolution\n\n");


#ifdef TIME
    double t_start = CPU_TIME;
#endif


            /* auxiliary grid to store cells' status */
            BOOL* grid_aux = (BOOL*) malloc(n_cells*sizeof(BOOL));
            /* temporary pointer used for grid switching */
            void* temp=NULL;


            for (int gen=0; gen<n; gen++) {

               
                /* updating first row */
                
                /* first element of the first row */
                char count = 0;

                count += grid[1];
                for (int b=x_size-1; b<x_size+2; b++) {
                    count += grid[b];
                }
                count += grid[2*x_size-1];
                for (int b=n_cells-x_size; b<n_cells-x_size+2; b++) {
                    count += grid[b];
                }
                count += grid[n_cells-1];

                if (count > 1 && count < 4) {
                    grid_aux[0] = 1;
                } else {
                    grid_aux[0] = 0;
                }

                /* internal elements of the first row */
                for (int j=1; j<x_size-1; j++) {

                    count = 0;
                    count += grid[j-1];
                    count += grid[j+1];
                    for (int b=j-1; b<j+2; b++) {
                        count += grid[x_size+b];
                    }
                    for (int b=n_cells-x_size+j-1; b<n_cells-x_size+j+2; b++) {
                        count += grid[b];
                    }

                    if (count > 1 && count < 4) {
                        grid_aux[j] = 1;
                    } else {
                        grid_aux[j] = 0;
                    }
                }

                /* last element of the first row */
                count = 0;
                count += grid[0];
                count += grid[x_size-2];
                count += grid[x_size];
                for (int b=2*x_size-2; b<2*x_size; b++) {
                    count += grid[b];
                }
                count += grid[n_cells-x_size];
                for (int b=n_cells-2; b<n_cells; b++) {
                    count += grid[b];
                }

                if (count > 1 && count < 4) {
                    grid_aux[x_size-1] = 1;
                } else {
                    grid_aux[x_size-1] = 0;
                }


                /* updating internal cells */ 

                /* iteration on internal rows */
                for (int i=1; i<y_size-1; i++) {

                    /* first element of the row */
                    count = 0;
                    for (int b=(i-1)*x_size; b<(i-1)*x_size+2; b++) {
                        count += grid[b];
                    }
                    count += grid[i*x_size-1];
                    count += grid[i*x_size+1];
                    for (int b=(i+1)*x_size-1; b<(i+1)*x_size+2; b++) {
                        count += grid[b];
                    }
                    count += grid[(i+2)*x_size-1];

                    if (count > 1 && count < 4) {
                        grid_aux[i*x_size] = 1;
                    } else {
                        grid_aux[i*x_size] = 0;
                    }

                    /* iteration on internal columns */
                    for (int j=1; j<x_size-1; j++) {

                        count = 0;
                        for (int b=(i-1)*x_size+j-1; b<(i-1)*x_size+j+2; b++) {
                            count += grid[b];
                        }
                        count += grid[i*x_size+j-1];
                        count += grid[i*x_size+j+1];
                        for (int b=(i+1)*x_size+j-1; b<(i+1)*x_size+j+2; b++) {
                            count += grid[b];
                        }

                        if (count > 1 && count < 4) {
                            grid_aux[i*x_size+j] = 1;
                        } else {
                            grid_aux[i*x_size+j] = 0;
                        }
                    }

                    /* last element of the row */
                    count = 0;
                    count += grid[(i-1)*x_size];
                    for (int b=i*x_size-2; b<i*x_size+1; b++) {
                        count += grid[b];
                    }
                    count += grid[(i+1)*x_size-2];
                    count += grid[(i+1)*x_size];
                    for (int b=(i+2)*x_size-2; b<(i+2)*x_size; b++) {
                        count += grid[b];
                    }

                    if (count > 1 && count < 4) {
                        grid_aux[(i+1)*x_size-1] = 1;
                    } else {
                        grid_aux[(i+1)*x_size-1] = 0;
                    }
                }


                /* updating last row */
                
                /* first element of the last row */
                count = 0;
                for (int b=(y_size-2)*x_size; b<(y_size-2)*x_size+2; b++) {
                    count += grid[b];
                }
                count += grid[n_cells-x_size-1];
                count += grid[n_cells-x_size+1];
                count += grid[n_cells-1];
                for (int b=0; b<2; b++) {
                    count += grid[b];
                }
                count += grid[x_size-1];

                if (count > 1 && count < 4) {
                    grid_aux[n_cells-x_size] = 1;
                } else {
                    grid_aux[n_cells-x_size] = 0;
                }

                /* internal elements of the last row */
                for (int j=1; j<x_size-1; j++) {

                    count = 0;
                    for (int b=j-1; b<j+2; b++) {
                        count += grid[n_cells-2*x_size+b];
                    }
                    count += grid[n_cells-x_size+j-1];
                    count += grid[n_cells-x_size+j+1];
                    for (int b=j-1; b<j+2; b++) {
                        count += grid[b];
                    }

                    if (count > 1 && count < 4) {
                        grid_aux[n_cells-x_size+j] = 1;
                    } else {
                        grid_aux[n_cells-x_size+j] = 0;
                    }
                }

                /* last element of the last row */
                count = 0;
                count += grid[n_cells-2*x_size];
                for (int b=n_cells-x_size-2; b<n_cells-x_size+1; b++) {
                    count += grid[b];
                }
                count += grid[n_cells-2];
                count += grid[0];
                for (int b=x_size-2; b<x_size; b++) {
                    count += grid[b];
                }

                if (count > 1 && count < 4) {
                    grid_aux[n_cells-1] = 1;
                } else {
                    grid_aux[n_cells-1] = 0;
                }
           

                /* switching pointers to grid and grid_aux */
                temp = grid;
                grid = grid_aux;
                grid_aux = temp;
                temp = NULL;


                /* saving a snapshot of the system */
                if (s != 0) {

                    if (gen % s == 0) {

                        sprintf(snap_name, "../../images/snapshots/snapshot_%05d.pgm", gen+1);
                        
                        write_pgm_image(grid, 1, x_size, y_size, snap_name, 1);
                    }
                }

                
            }

            free(grid_aux);


#ifdef TIME 
    double time = CPU_TIME - t_start;
    printf("elapsed time: %f sec\n\n", time);
#endif



        /* static evolution with single grid */
        } else if (e == STATIC_SAVE_MEM) {


            printf("using static evolution with a single grid in memory\n\n");


#ifdef TIME
    double t_start = CPU_TIME;
#endif



            for (int gen=0; gen<n; gen++) {


                /* needed for evolution without allocating auxiliary grid */
                char alive = 2 - gen % 2;
                char dead = 1 + gen % 2;
                char shift = gen % 2;


                /* updating first row */
                
                /* first element of the first row */
                char count = 0;

                count += (grid[1] >> shift) & 1;
                for (int b=x_size-1; b<x_size+2; b++) {
                    count += (grid[b] >> shift) & 1;
                }
                count += (grid[2*x_size-1] >> shift) & 1;
                for (int b=n_cells-x_size; b<n_cells-x_size+2; b++) {
                    count += (grid[b] >> shift) & 1;
                }
                count += (grid[n_cells-1] >> shift) & 1;

                if (count == 2 || count == 3) {
                    grid[0] |= alive;
                } else {
                    grid[0] &= dead;
                }

                /* internal elements of the first row */
                for (int j=1; j<x_size-1; j++) {

                    count = 0;
                    count += (grid[j-1] >> shift) & 1;
                    count += (grid[j+1] >> shift) & 1;
                    for (int b=j-1; b<j+2; b++) {
                        count += (grid[x_size+b] >> shift) & 1;
                    }
                    for (int b=n_cells-x_size+j-1; b<n_cells-x_size+j+2; b++) {
                        count += (grid[b] >> shift) & 1;
                    }

                    if (count == 2 || count == 3) {
                        grid[j] |= alive;
                    } else {
                        grid[j] &= dead;
                    }
                }

                /* last element of the first row */
                count = 0;
                count += (grid[0] >> shift) & 1;
                count += (grid[x_size-2] >> shift) & 1;
                count += (grid[x_size] >> shift) & 1;
                for (int b=2*x_size-2; b<2*x_size; b++) {
                    count += (grid[b] >> shift) & 1;
                }
                count += (grid[n_cells-x_size] >> shift) & 1;
                for (int b=n_cells-2; b<n_cells; b++) {
                    count += (grid[b] >> shift) & 1;
                }

                if (count == 2 || count == 3) {
                    grid[x_size-1] |= alive;
                } else {
                    grid[x_size-1] &= dead;
                }


                /* updating internal cells */ 

                /* iteration on internal rows */
                for (int i=1; i<y_size-1; i++) {

                    /* first element of the row */
                    count = 0;
                    for (int b=(i-1)*x_size; b<(i-1)*x_size+2; b++) {
                        count += (grid[b] >> shift) & 1;
                    }
                    count += (grid[i*x_size-1] >> shift) & 1;
                    count += (grid[i*x_size+1] >> shift) & 1;
                    for (int b=(i+1)*x_size-1; b<(i+1)*x_size+2; b++) {
                        count += (grid[b] >> shift) & 1;
                    }
                    count += (grid[(i+2)*x_size-1] >> shift) & 1;

                    if (count == 2 || count == 3) {
                        grid[i*x_size] |= alive;
                    } else {
                        grid[i*x_size] &= dead;
                    }

                    /* iteration on internal columns */
                    for (int j=1; j<x_size-1; j++) {

                        count = 0;
                        for (int b=(i-1)*x_size+j-1; b<(i-1)*x_size+j+2; b++) {
                            count += (grid[b] >> shift) & 1;
                        }
                        count += (grid[i*x_size+j-1] >> shift) & 1;
                        count += (grid[i*x_size+j+1] >> shift) & 1;
                        for (int b=(i+1)*x_size+j-1; b<(i+1)*x_size+j+2; b++) {
                            count += (grid[b] >> shift) & 1;
                        }

                        if (count == 2 || count == 3) {
                            grid[i*x_size+j] |= alive;
                        } else {
                            grid[i*x_size+j] &= dead;
                        }
                    }

                    /* last element of the row */
                    count = 0;
                    count += (grid[(i-1)*x_size] >> shift) & 1;
                    for (int b=i*x_size-2; b<i*x_size+1; b++) {
                        count += (grid[b] >> shift) & 1;
                    }
                    count += (grid[(i+1)*x_size-2] >> shift) & 1;
                    count += (grid[(i+1)*x_size] >> shift) & 1;
                    for (int b=(i+2)*x_size-2; b<(i+2)*x_size; b++) {
                        count += (grid[b] >> shift) & 1;
                    }

                    if (count == 2 || count == 3) {
                        grid[(i+1)*x_size-1] |= alive;
                    } else {
                        grid[(i+1)*x_size-1] &= dead;
                    }
                }


                /* updating last row */
                
                /* first element of the last row */
                count = 0;
                for (int b=(y_size-2)*x_size; b<(y_size-2)*x_size+2; b++) {
                    count += (grid[b] >> shift) & 1;
                }
                count += (grid[n_cells-x_size-1] >> shift) & 1;
                count += (grid[n_cells-x_size+1] >> shift) & 1;
                count += (grid[n_cells-1] >> shift) & 1;
                for (int b=0; b<2; b++) {
                    count += (grid[b] >> shift) & 1;
                }
                count += (grid[x_size-1] >> shift) & 1;

                if (count == 2 || count == 3) {
                    grid[n_cells-x_size] |= alive;
                } else {
                    grid[n_cells-x_size] &= dead;
                }

                /* internal elements of the last row */
                for (int j=1; j<x_size-1; j++) {

                    count = 0;
                    for (int b=j-1; b<j+2; b++) {
                        count += (grid[n_cells-2*x_size+b] >> shift) & 1;
                    }
                    count += (grid[n_cells-x_size+j-1] >> shift) & 1;
                    count += (grid[n_cells-x_size+j+1] >> shift) & 1;
                    for (int b=j-1; b<j+2; b++) {
                        count += (grid[b] >> shift) & 1;
                    }

                    if (count == 2 || count == 3) {
                        grid[n_cells-x_size+j] |= alive;
                    } else {
                        grid[n_cells-x_size+j] &= dead;
                    }
                }

                /* last element of the last row */
                count = 0;
                count += (grid[n_cells-2*x_size] >> shift) & 1;
                for (int b=n_cells-x_size-2; b<n_cells-x_size+1; b++) {
                    count += (grid[b] >> shift) & 1;
                }
                count += (grid[n_cells-2] >> shift) & 1;
                count += (grid[0] >> shift) & 1;
                for (int b=x_size-2; b<x_size; b++) {
                    count += (grid[b] >> shift) & 1;
                }

                if (count == 2 || count == 3) {
                    grid[n_cells-1] |= alive;
                } else {
                    grid[n_cells-1] &= dead;
                }


                /* saving a snapshot of the system */
                if (s != 0) {

                    if (gen % s == 0) {

                        sprintf(snap_name, "../../images/snapshots/snapshot_%05d.pgm", gen+1);
                        
                        write_pgm_image(grid, 1, x_size, y_size, snap_name, gen);
                    }
                }
                
            }


#ifdef TIME 
    double time = CPU_TIME - t_start;
    printf("elapsed time: %f sec\n\n", time);
#endif


        }

        
        free(snap_name);


        write_pgm_image(grid, 1, x_size, y_size, "../../images/snapshots/final_state.pgm", n-1);

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



/* function to write the status of the system to pgm file */
void write_pgm_image(BOOL* image, const int maxval, int xsize, int ysize, const char *image_name, int iter) {

    FILE* image_file;
    image_file = fopen(image_name, "w");
    
    /* formatting the header */
    fprintf(image_file, "P5 %d %d\n%d\n", xsize, ysize, maxval);   

    /* printing first or second bit depending generation */
    if (iter % 2 == 0) {
        for (int i=0; i<xsize*ysize; i++)
            fprintf(image_file, "%c", image[i] >> 1);
    } else {
        for (int i=0; i<xsize*ysize; i++)
            fprintf(image_file, "%c", image[i] & 1);
    }

    fclose(image_file);


    /////////////////////////// just for test ///////////////////////////// 
    //
    //if (iter % 2 == 0) {
    //    for (int i=0; i<ysize; i++){
    //        for (int j=0; j<xsize; j++)
    //            printf("%c ", (image[i*xsize+j] >> 1)+48);
    //        printf("\n");
    //    }
    //} else {
    //    for (int i=0; i<ysize; i++) {
    //        for (int j=0; j<xsize; j++)
    //            printf("%c ", (image[i*xsize+j] & 1)+48);
    //        printf("\n");
    //    }
    //}
    //
    //printf("\n");
    //
    //////////////////////////////////////////////////////////////////////


    return;
}


/* function to read the status of the system from pgm file */
void read_pgm_image(BOOL **image, int *maxval, int *xsize, int *ysize, const char *image_name) {
/*
 * image        : a pointer to the pointer that will contain the image
 * maxval       : a pointer to the int that will store the maximum intensity in the image
 * xsize, ysize : pointers to the x and y sizes
 * image_name   : the name of the file to be read
 */ 
    FILE* image_file;
    image_file = fopen(image_name, "r"); 

    *image = NULL;
    *xsize = *ysize = *maxval = 0;

    char    MagicN[3];
    char   *line = NULL;
    size_t  k, n = 0;

    // get the Magic Number 
    k = fscanf(image_file, "%2s%*c", MagicN );

    // skip all the comments 
    k = getline( &line, &n, image_file);
    while ( (k > 0) && (line[0]=='#') )
        k = getline( &line, &n, image_file);

    if (k > 0) {
        k = sscanf(line, "%d%*c%d%*c%d%*c", xsize, ysize, maxval);
        if ( k < 3 )
            fscanf(image_file, "%d%*c", maxval);
    } else {
        *maxval = -1;         // this is the signal that there was an I/O error
			                  // while reading the image header 
        free( line );
        return;
    }
    
    free( line );
    
    unsigned int size = *xsize * *ysize;
  
    if ( (*image = (char*)malloc( size )) == NULL ) {
        fclose(image_file);
        *maxval = -2;         // this is the signal that memory was insufficient 
        *xsize  = 0;
        *ysize  = 0;
        return;
    }
  
    if ( fread( *image, 1, size, image_file) != size ) {
        free( image );
        image   = NULL;
        *maxval = -3;         // this is the signal that there was an i/o error 
        *xsize  = 0;
        *ysize  = 0;
    }  

    fclose(image_file);
    return;
}
