


/////////////// MODIFICARE COPIANDO DA PARALLEL_IO





#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <mpi.h>
#include <omp.h>



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

#define BOOL char

char fname_deflt[] = "game_of_life.pgm";

int   action = ORDERED;
int   m      = M_DFLT;
int   k      = K_DFLT;
int   e      = ORDERED;
int   n      = 10000;
int   s      = 1;
char *fname  = NULL;



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
                fname = (char*) malloc(30);
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



    /* initializing a playground */
    if (action == INIT) {


        /* assigning default name to file in case none was passed */

        if (fname == NULL) {

            printf("-- no output file was passed - initial conditions will be written to %s\n\n", fname_deflt);

            fname = (char*) malloc(sizeof(fname_deflt));
            sprintf(fname, "%s", fname_deflt);
        }

        const unsigned int n_cells = m*k;



        /* setting up MPI and single processes' variables */

        int my_id, n_procs;
        MPI_Init(&argc, &argv);
        MPI_Status status;

        MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
        MPI_Comm_rank(MPI_COMM_WORLD, &my_id);

        /* distributing playground's rows to MPI processes */
        unsigned int my_m = m/n_procs;
        const int m_rmd = m%n_procs;
        /* remaining rows */
        if (my_id < m_rmd)
            my_m++;
        const int my_n_cells = my_m*k;



        /* initializing grid to random booleans */

        BOOL* my_grid = (BOOL*) malloc(my_n_cells*sizeof(BOOL));

        /* setting the seed (unique for each process) */
        struct drand48_data rand_gen;
        long int seed = my_id+1;
        srand48_r(seed, &rand_gen);

        for (int i=0; i<my_n_cells; i++) {
            /* producing a random integer among 0 and 1 */
            double random_number;
            drand48_r(&rand_gen, &random_number);
            short int rand_bool = (short int) (random_number+0.5);
            /* converting random number to BOOL */
            my_grid[i] = (BOOL) rand_bool;
        }



        /* writing down the playground */

        MPI_File f_handle;  
       
	    /* formatting the PGM file */ 
        const int header_size = 30;
        if (my_id == 0) {

            /* setting the header */
            const int color_maxval = 1;	    
            char header[header_size];
            sprintf(header, "P5 %d %d\n%d\n", k, m, color_maxval);

            /* writing the header */
            int access_mode = MPI_MODE_CREATE | MPI_MODE_WRONLY | MPI_MODE_APPEND;
            MPI_File_open(MPI_COMM_SELF, fname, access_mode, MPI_INFO_NULL, &f_handle);

            MPI_File_write_at(f_handle, 0, header, header_size, MPI_CHAR, &status);

            MPI_File_close(&f_handle);
        }

        /* needed to make sure that all processes are actually 
         * wrtiting on an already formatted PGM file */
        MPI_Barrier(MPI_COMM_WORLD);

        /* opening file in parallel */
        int access_mode = MPI_MODE_APPEND | MPI_MODE_WRONLY;
        MPI_File_open(MPI_COMM_WORLD, fname, access_mode, MPI_INFO_NULL, &f_handle);

        /* computing offsets */
	    int offset = header_size;
	    if (my_id < m_rmd) {
		   offset += my_id*my_n_cells;
	    } else {
		   offset += m_rmd*k + my_id*my_n_cells;
	    }

        /* writing in parallel */
        MPI_File_write_at_all(f_handle, offset, my_grid, my_m*k, MPI_CHAR, &status);

        MPI_File_close(&f_handle);
 
        

        MPI_Finalize();

        free(my_grid);

    }





///////////// aggiungere dichiarazione di grid quando ACTION==RUN
///////////// ricordare di aggiungere due righe extra per i processi vicini



    if (fname != NULL) {
        free(fname);
        fname = NULL;
    }


    return 0;
}
