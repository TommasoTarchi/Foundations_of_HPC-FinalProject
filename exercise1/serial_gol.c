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



void write_pgm_image(BOOL*, int, int, int, const char*); 



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

         
        grid = (BOOL*) malloc(n_cells*sizeof(BOOL));

        srand(time(NULL));
        double randmax_inv = 1.0/RAND_MAX;
        for (int i=0; i<n_cells; i++) {
            /* producing a random number between 0 and 1 */
            short int rand_bool = (short int) (rand()*randmax_inv+0.5);
            /* converting random number to char */
            grid[i] = (BOOL) rand_bool;
        }

        write_pgm_image(grid, 1, k, k, fname);

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
        //


        ///////////////////////////////////////////////

        grid = (BOOL*) malloc(n_cells*sizeof(BOOL));

        srand(time(NULL));
        double randmax_inv = 1.0/RAND_MAX;
        for (int i=0; i<n_cells; i++) {
            /* producing a random number between 0 and 1 */
            short int rand_bool = (short int) (rand()*randmax_inv+0.5);
            /* converting random number to char */
            grid[i] = (BOOL) rand_bool;
        }

        ///////////////////////////////////////////////


        /* string to store the name of the snapshot files */
        char* snap_name = (char*) malloc(25*sizeof(char));



        /* ordered evolution */
        if (e == ORDERED) {


            for (int gen=0; gen<n; gen++) {


                /* updating first row */
                
                /* first element of the first row */
                short int count = 0;
                if (grid[1] == 1)
                    count++;
                for (int b=k; b<k+2; b++) {
                    if (grid[b] == 1)
                        count ++;
                }

                if (count == 2 || count == 3) {
                    grid[0] = 1;
                } else {
                    grid[0] = 0;
                }

                /* internal elements of the first row */
                for (int j=1; j<k-1; j++) {

                    count = 0;
                    for (int a=0; a<2; a++) {
                        for (int b=j-1; b<j+2; b++) {
                            if (grid[a*k+b] == 1)
                                count++;
                        }
                    }
                    if (grid[j] == 1)
                        count--;

                    if (count == 2 || count == 3) {
                        grid[j] = 1;
                    } else {
                        grid[j] = 0;
                    }
                }

                /* last element of the first row */
                count = 0;
                if (grid[k-2] == 1)
                    count++;
                for (int b=2*k-2; b<2*k; b++) {
                    if (grid[b] == 1)
                        count++;
                }

                if (count == 2 || count == 3) {
                    grid[k-1] = 1;
                } else {
                    grid[k-1] = 0;
                }


                /* updating internal cells */ 

                /* iteration on internal rows */
                for (int i=1; i<k-1; i++) {

                    /* first element of the row */
                    count = 0;
                    for (int a=i-1; a<i+2; a++) {
                        for (int b=0; b<2; b++) {
                            if (grid[a*k+b] == 1)
                                count++;
                        }
                    }
                    if (grid[i*k] == 1)
                        count--;

                    if (count == 2 || count == 3) {
                        grid[i*k] = 1;
                    } else {
                        grid[i*k] = 0;
                    }

                    /* iteration on internal columns */
                    for (int j=1; j<k-1; j++) {

                        count = 0;
                        for (int a=i-1; a<i+2; a++) {
                            for (int b=j-1; b<j+2; b++) {
                                if (grid[a*k+b] == 1)
                                    count++;
                            }
                        }
                        if (grid[i*k+j] == 1)
                            count--;

                        if (count == 2 || count == 3) {
                            grid[i*k+j] = 1;
                        } else {
                            grid[i*k+j] = 0;
                        }
                    }

                    /* last element of the row */
                    count = 0;
                    for (int a=i; a<i+3; a++) {
                        for (int b=-2; b<0; b++) {
                            if (grid[a*k+b] == 1)
                                count++;
                        }
                    }
                    if (grid[(i+1)*k-1] == 1)
                        count--;

                    if (count == 2 || count == 3) {
                        grid[(i+1)*k-1] = 1;
                    } else {
                        grid[(i+1)*k-1] = 0;
                    }
                }
                

                /* updating last row */
                
                /* first element of the last row */
                count = 0;
                for (int b=(k-2)*k; b<(k-2)*k+2; b++) {
                    if (grid[b] == 1)
                        count++;
                }
                if (grid[n_cells-k+1] == 1)
                    count++;
                
                if (count == 2 || count == 3) {
                    grid[n_cells-k] = 1;
                } else {
                    grid[n_cells-k] = 0;
                }

                /* internal elements of the last row */
                for (int j=1; j<k-1; j++) {

                    count = 0;
                    for (int a=k-2; a<k; a++) {
                        for (int b=j-1; b<j+2; b++) {
                            if (grid[a*k+b] == 1)
                                count++;
                        }
                    }
                    if (grid[n_cells-k+j] == 1)
                        count--;

                    if (count == 2 || count == 3) {
                        grid[n_cells-k+j] = 1;
                    } else {
                        grid[n_cells-k+j] = 0;
                    }
                }

                /* last element of the last row */
                count = 0;
                for (int b=n_cells-k-2; b<n_cells-k; b++) {
                    if (grid[b] == 1)
                        count++;
                }
                if (grid[n_cells-2] == 1)
                    count++;

                if (count == 2 || count == 3) {
                    grid[n_cells-1] = 1;
                } else {
                    grid[n_cells-1] = 0;
                }


                /* saving a snapshot of the system */
                if (gen % s == 0) {

                    sprintf(snap_name, "snapshots/snapshot_%05d", gen);

                    write_pgm_image(grid, 1, k, k, snap_name);
                }

            }



        /* static evolution */
        } else if (e == STATIC) {


            /* auxiliary grid to store cells' status */
            BOOL* grid_aux = (BOOL*) malloc(n_cells*sizeof(BOOL));
            /* temporary pointer used for grid switching */
            void* temp=NULL;


            for (int gen=0; gen<n; gen++) {


                /* updating first row */
                
                /* first element of the first row */
                short int count = 0;
                if (grid[1] == 1)
                    count++;
                for (int b=k; b<k+2; b++) {
                    if (grid[b] == 1)
                        count ++;
                }

                if (count == 2 || count == 3) {
                    grid_aux[0] = 1;
                } else {
                    grid_aux[0] = 0;
                }

                /* internal elements of the first row */
                for (int j=1; j<k-1; j++) {

                    count = 0;
                    for (int a=0; a<2; a++) {
                        for (int b=j-1; b<j+2; b++) {
                            if (grid[a*k+b] == 1)
                                count++;
                        }
                    }
                    if (grid[j] == 1)
                        count--;

                    if (count == 2 || count == 3) {
                        grid_aux[j] = 1;
                    } else {
                        grid_aux[j] = 0;
                    }
                }

                /* last element of the first row */
                count = 0;
                if (grid[k-2] == 1)
                    count++;
                for (int b=2*k-2; b<2*k; b++) {
                    if (grid[b] == 1)
                        count++;
                }

                if (count == 2 || count == 3) {
                    grid_aux[k-1] = 1;
                } else {
                    grid_aux[k-1] = 0;
                }


                /* updating internal cells */ 

                /* iteration on internal rows */
                for (int i=1; i<k-1; i++) {

                    /* first element of the row */
                    count = 0;
                    for (int a=i-1; a<i+2; a++) {
                        for (int b=0; b<2; b++) {
                            if (grid[a*k+b] == 1)
                                count++;
                        }
                    }
                    if (grid[i*k] == 1)
                        count--;

                    if (count == 2 || count == 3) {
                        grid_aux[i*k] = 1;
                    } else {
                        grid_aux[i*k] = 0;
                    }

                    /* iteration on internal columns */
                    for (int j=1; j<k-1; j++) {

                        count = 0;
                        for (int a=i-1; a<i+2; a++) {
                            for (int b=j-1; b<j+2; b++) {
                                if (grid[a*k+b] == 1)
                                    count++;
                            }
                        }
                        if (grid[i*k+j] == 1)
                            count--;

                        if (count == 2 || count == 3) {
                            grid_aux[i*k+j] = 1;
                        } else {
                            grid_aux[i*k+j] = 0;
                        }
                    }

                    /* last element of the row */
                    count = 0;
                    for (int a=i; a<i+3; a++) {
                        for (int b=-2; b<0; b++) {
                            if (grid[a*k+b] == 1)
                                count++;
                        }
                    }
                    if (grid[(i+1)*k-1] == 1)
                        count--;

                    if (count == 2 || count == 3) {
                        grid_aux[(i+1)*k-1] = 1;
                    } else {
                        grid_aux[(i+1)*k-1] = 0;
                    }
                }
                

                /* updating last row */
                
                /* first element of the last row */
                count = 0;
                for (int b=(k-2)*k; b<(k-2)*k+2; b++) {
                    if (grid[b] == 1)
                        count++;
                }
                if (grid[n_cells-k+1] == 1)
                    count++;
                
                if (count == 2 || count == 3) {
                    grid_aux[n_cells-k] = 1;
                } else {
                    grid_aux[n_cells-k] = 0;
                }

                /* internal elements of the last row */
                for (int j=1; j<k-1; j++) {

                    count = 0;
                    for (int a=k-2; a<k; a++) {
                        for (int b=j-1; b<j+2; b++) {
                            if (grid[a*k+b] == 1)
                                count++;
                        }
                    }
                    if (grid[n_cells-k+j] == 1)
                        count--;

                    if (count == 2 || count == 3) {
                        grid_aux[n_cells-k+j] = 1;
                    } else {
                        grid_aux[n_cells-k+j] = 0;
                    }
                }

                /* last element of the last row */
                count = 0;
                for (int b=n_cells-k-2; b<n_cells-k; b++) {
                    if (grid[b] == 1)
                        count++;
                }
                if (grid[n_cells-2] == 1)
                    count++;

                if (count == 2 || count == 3) {
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
                if (gen % s == 0) {

                    sprintf(snap_name, "snapshots/snapshot_%05d", gen);

                    write_pgm_image(grid, 1, k, k, snap_name);
                }

            }

            free(grid_aux);
        }

        
        free(snap_name);
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



void write_pgm_image(BOOL* image, int maxval, int xsize, int ysize, const char *image_name) {

    FILE* image_file;

    image_file = fopen(image_name, "w");
    
    fprintf(image_file, "P5 %d %d\n%d\n", xsize, ysize, maxval);

    fwrite(image, 1, xsize*ysize, image_file);

    fclose(image_file);

    return;
}
