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



int read_pgm_header(int*, unsigned int*, unsigned int*, int*, const char*);



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



    /* setting up MPI and single processes' variables */

    int my_id, n_procs;
    MPI_Init(&argc, &argv);
    MPI_Status status;
    
    MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_id);



    
    /* initializing a playground */
    if (action == INIT) {


        /* assigning default name to file in case none was passed */

        if (fname == NULL) {

            if (my_id == 0)
                printf("-- no output file was passed - initial conditions will be written to %s\n\n", fname_deflt);

            fname = (char*) malloc(sizeof(fname_deflt));
            sprintf(fname, "%s", fname_deflt);
        }

        const unsigned int n_cells = m*k;



        /* distributing playground's rows to MPI processes */

        unsigned int my_m = m/n_procs;   // number of rows assigned to the process
        const int m_rmd = m%n_procs;
        /* assigning remaining rows */
        if (my_id < m_rmd)
            my_m++;
        const int my_n_cells = my_m*k;   // number of cells assigned to the process



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

        MPI_File f_handle;   // pointer to file
        int check = 0;   // error checker
       
	    /* formatting the PGM file */ 
        const int header_size = 30;
        if (my_id == 0) {

            /* setting the header */
            const int color_maxval = 1;	    
            char header[header_size];
            sprintf(header, "P5 %d %d\n%d\n", k, m, color_maxval);

            /* writing the header */
            int access_mode = MPI_MODE_CREATE | MPI_MODE_WRONLY | MPI_MODE_APPEND;
            check += MPI_File_open(MPI_COMM_SELF, fname, access_mode, MPI_INFO_NULL, &f_handle);

            check += MPI_File_write_at(f_handle, 0, header, header_size, MPI_CHAR, &status);

            check += MPI_File_close(&f_handle);
        }

        /* needed to make sure that all processes are actually 
         * wrtiting on an already formatted PGM file */
        MPI_Barrier(MPI_COMM_WORLD);

        /* opening file in parallel */
        int access_mode = MPI_MODE_APPEND | MPI_MODE_WRONLY;
        check += MPI_File_open(MPI_COMM_WORLD, fname, access_mode, MPI_INFO_NULL, &f_handle);

        /* computing offsets */
	    int offset = header_size;
	    if (my_id < m_rmd) {
		   offset += my_id*my_n_cells;
	    } else {
		   offset += m_rmd*k + my_id*my_n_cells;
	    }

        /* writing in parallel */
        check += MPI_File_write_at_all(f_handle, offset, my_grid, my_m*k, MPI_CHAR, &status);

        check += MPI_File_close(&f_handle);
 

        if (my_id == 0 && check != 0)
            printf("--- AN I/O ERROR OCCURRED AT SOME POINT ---\n\n");

        free(my_grid);

    }




    /* running game of life */
    if (action == RUN) {


        /* assigning default name to file in case none was passed */
        if (fname == NULL) {

            if (my_id == 0)
                printf("-- no file with initial playground was passed - the program will try to read from %s\n\n", fname_deflt);

            fname = (char*) malloc(sizeof(fname_deflt));
            sprintf(fname, "%s", fname_deflt);
        }

        
        /* reading initial playground from pmg file */

        int check = 0;   // error checker

        int color_maxval;
        unsigned int x_size;
        unsigned int y_size;
        int header_size;
        
        check += read_pgm_header(&color_maxval, &x_size, &y_size, &header_size, fname);
        if (my_id == 0 && check != 0) {
            printf("-- AN ERROR OCCURRED WHILE READING THE HEADER OF THE PGM FILE --\n\n");
            check = 0;
        }

        const unsigned int n_cells = x_size*y_size;   // total number of cells



        /* string to store the name of the snapshot files */
        char* snap_name = (char*) malloc(29*sizeof(char));



        if (e == ORDERED) {


            if (my_id == 0)
                printf("ordered evolution\n");



        } else if (e == STATIC) {


            /* setting process 'personal' varables */
            unsigned int my_y_size = y_size/n_procs;   // number of rows assigned to the process
            const int y_size_rmd = y_size%n_procs;
            /* assigning remaining rows */
            if (my_id < y_size_rmd)
                my_y_size++;
            const unsigned int my_n_cells = x_size*my_y_size;   // number of cells assigned to the process


            /* getting initial cells' status */ 

            /* we alloc two more lines for the neighbor processes' cells */
            BOOL* my_grid = (BOOL*) malloc((my_n_cells+2*x_size)*sizeof(BOOL));

 


	    //////////// USARE COLOR_MAXVAL QUANDO DUMPING


            free(my_grid);

        }


    }



    MPI_Finalize();


    if (fname != NULL) {
        free(fname);
        fname = NULL;
    }


    return 0;
}



/* function to get content and length of the header of a pgm file */
int read_pgm_header(int* maxval, unsigned int* xsize, unsigned int* ysize, int* header_size, const char* fname) {


    FILE* image_file;
    image_file = fopen(fname, "r"); 

    *xsize = *ysize = *maxval = 0;

    char    MagicN[3];
    char   *line = NULL;
    size_t  k, n = 0;


    /* getting the Magic Number */
    k = fscanf(image_file, "%2s%*c", MagicN);

    /* skipping all the comments */
    k = getline(&line, &n, image_file);
    while (k > 0 && line[0]=='#')
        k = getline(&line, &n, image_file);

    /* getting the parameters */
    if (k > 0) {
        
        k = sscanf(line, "%d%*c%d%*c%d%*c", xsize, ysize, maxval);
        if (k < 3)
            fscanf(image_file, "%d%*c", maxval);
    
    } else {

        fclose(image_file);
        free(line);
        return 1;   /* error in reading the header */
    }


    /* getting header size */ 
    fseek(image_file, 0L, SEEK_END);
    long unsigned int total_size = ftell(image_file);
    *header_size = total_size - *xsize*(*ysize);


    fclose(image_file);
    free(line);
    return 0;
}
