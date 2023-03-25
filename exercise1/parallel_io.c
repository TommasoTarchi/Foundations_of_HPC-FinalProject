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



void write_pgm_image(BOOL*, const int, int, int,int, const char*, const char*);
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
        


        /* initializing MPI processes */ 
        int my_id, n_procs;
        MPI_Status status;
        MPI_Init(&argc, &argv);

        MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
        MPI_Comm_rank(MPI_COMM_WORLD, &my_id);


        /* getting 'personal' data */ 
        unsigned int my_m = m/n_procs;
        const int m_rmd = m%n_procs;
        char more_row = 0;
        /* remaining rows */
        if (my_id < m_rmd) {
            my_m++;
            more_row = 1;
        }

        const int my_n_cells = my_m*k;


        /* initializing grid to random booleans */
        BOOL* my_grid = (BOOL*) malloc(my_n_cells*sizeof(BOOL));

        //srand(time(NULL));      //// CORREGGERE PER PROCESSI PARALLELI
        srand(my_id+1);
        double randmax_inv = 1.0/RAND_MAX;
        for (int i=0; i<my_n_cells; i++) {
            /* producing a random number between 0 and 1 */
            short int rand_bool = (short int) (rand()*randmax_inv+0.5);
            /* converting random number to char */
            my_grid[i] = (BOOL) rand_bool;
        }


        /* writing down the playground */
       
	    /* formatting the header */
        if (my_id == 0) {
		
		    FILE* image_file;
		    image_file = fopen(fname, "w");
	    	const int color_maxval = 1;
		
		    fprintf(image_file, "P5 %d %d\n%d\n", k, m, color_maxval);

		    fclose(image_file);
	    }

        MPI_File f_ptr;
	
        MPI_Barrier(MPI_COMM_WORLD);

	    /* opening the file in parallel */
        MPI_File_open(MPI_COMM_WORLD, fname, MPI_MODE_APPEND, MPI_INFO_NULL, &f_ptr);

        /* computing the offset */ 
        int offset;
        if (more_row == 1) {
            offset = my_id*my_m*k;
        } else {
            offset = m_rmd + my_id*my_m;
        }

        /* setting the pointer to file */
        MPI_File_seek(f_ptr, offset, MPI_SEEK_CUR);

        MPI_Barrier(MPI_COMM_WORLD);

        /* writing playground in parallel */
        //MPI_File_write(f_ptr, my_grid, my_m*k, MPI_CHAR, &status);
        MPI_File_write_all(f_ptr, my_grid, my_m*k, MPI_CHAR, &status);

        MPI_File_close(&f_ptr);

        free(my_grid);

        MPI_Finalize();

    }





///////////// aggiungere dichiarazione di grid quando ACTION==RUN
///////////// ricordare di aggiungere due righe extra per i processi vicini



    if (fname != NULL) {
        free(fname);
        fname = NULL;
    }


    return 0;
}


/* function to write the status of the system to pgm file */
void write_pgm_image(BOOL* image, const int maxval, int xsize, int ysize, int my_ysize, const char *image_name, const char* mod) {

    FILE* image_file;
    image_file = fopen(image_name, mod);
    
    /* formatting the header */
    if (mod == "w") 
        fprintf(image_file, "P5 %d %d\n%d\n", xsize, ysize, maxval);

    /* writing */
    const int end = xsize*my_ysize;
    for (int i=0; i<end; i++)
            fprintf(image_file, "%c", image[i]);

    fclose(image_file);

    return;
}
