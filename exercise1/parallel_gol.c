#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <mpi.h>
#include <omp.h>



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



int read_pgm_header(unsigned int*, const char*);



int main(int argc, char **argv) {



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

        if (check != 0) {
            printf("--- AN I/O ERROR OCCURRED WHILE WRITING THE HEADER ---\n");
            check = 0;
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

        if (check != 0)
            printf("--- AN I/O ERROR OCCURRED ON PROCESS %d WHILE WRITING THE INITIAL PLAYGROUND ---\n", my_id);

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

        
        /* error variables for I/O */
        int check = 0;   // error checker
        short int error_control_1 = 0;   // needed for the for loop
        short int error_control_2 = 0;   //
        short int error_control_3 = 0;   //


        /* reading initial playground from pmg file */

        /* reading the header */
        int header_content[4];
	    for (int i=0; i<4; i++)
	        header_content[i] = 0;
        
        if (my_id == 0) {

            check += read_pgm_header(header_content, fname);
            
            if (check != 0) {
                printf("--- AN ERROR OCCURRED WHILE READING THE HEADER OF THE PGM FILE ---\n");
                check = 0;
            }
        }

        /* distributing header's information to all processes */
        check += MPI_Bcast(header_content, 4, MPI_INT, 0, MPI_COMM_WORLD);

	    if (check != 0) {
	        printf("--- AN ERROR OCCURRED WHILE BROADCASTING HEADER'S INFORMATIONS ON %d PROCESS ---\n", my_id);
	        check = 0;
	    }

        /* 'unpacking' header's information */
        const int color_maxval = header_content[0];
        const int x_size = header_content[1];
        const int y_size = header_content[2];
        const int header_size = header_content[3];

        const unsigned int n_cells = x_size*y_size;   // total number of cells


        /* distributing work among MPI processes */

        /* setting process 'personal' varables */
        unsigned int my_y_size = y_size/n_procs;   // number of rows assigned to the process
        const int y_size_rmd = y_size%n_procs;
        /* assigning remaining rows */
        if (my_id < y_size_rmd)
            my_y_size++;
        const unsigned int my_n_cells = x_size*my_y_size;   // number of cells assigned to the process

        /* grid to store cells status and two more rows for neighbor cells */ 
        BOOL* above_line = (BOOL*) malloc(x_size*sizeof(BOOL));
        BOOL* my_grid = (BOOL*) malloc(my_n_cells*sizeof(BOOL));
        BOOL* below_line = (BOOL*) malloc(x_size*sizeof(BOOL));

       	//BOOL* above_line = (BOOL*) calloc(x_size, sizeof(BOOL));
        //BOOL* my_grid = (BOOL*) calloc(my_n_cells, sizeof(BOOL));
        //BOOL* below_line = (BOOL*) calloc(x_size, sizeof(BOOL));


        MPI_File f_handle;   // pointer to file

        /* opening file in parallel */
	    int access_mode = MPI_MODE_RDONLY;
        check += MPI_File_open(MPI_COMM_WORLD, fname, access_mode, MPI_INFO_NULL, &f_handle);

        /* computing offsets */
	    int offset = header_size;
	    if (my_id < y_size_rmd) {
	        offset += my_id*my_n_cells;
	    } else {
	        offset += y_size_rmd*k + my_id*my_n_cells;
	    }


        /* setting the file pointer to offset */
        check += MPI_File_seek(f_handle, offset, MPI_SEEK_SET);
        /* reading in parallel */
        check += MPI_File_read_all(f_handle, my_grid, my_n_cells, MPI_CHAR, &status);

        check += MPI_File_close(&f_handle); 

        if (check != 0) {
            printf("--- AN I/O ERROR OCCURRED ON PROCESS %d WHILE READING THE INITIAL PLAYGROUND ---\n", my_id);
            check = 0;
        }

        /* setting tags, destinations and sources for future communications among MPI processes */	
        const int prev = (my_id != 0) * (my_id-1) + (my_id == 0) * (n_procs-1);
        const int succ = (my_id != n_procs-1) * (my_id+1);
        const int tag_send = my_id*10;
        const int tag_recv_p = prev*10;
        const int tag_recv_s = succ*10;


        /* string to store the name of the snapshot files */
        char* snap_name = (char*) malloc(29*sizeof(char));



        if (e == ORDERED) {


		printf("ordered evolution\n");


        } else if (e == STATIC) {
            

            /* auxiliary grid to store cells' status */
            BOOL* my_grid_aux = (BOOL*) malloc(my_n_cells*sizeof(BOOL));
            /* temporary pointer used for grid switching */
            void* temp=NULL;

            
            /* evolution */

            for (int gen=0; gen<n; gen++) {

 
                /* communicating neighbor cells' status to neighbor processes */ 
                
                if (my_id % 2 == 0) {

                    check += MPI_Send(my_grid, x_size, MPI_CHAR, prev, tag_send, MPI_COMM_WORLD);
                    check += MPI_Recv(below_line, x_size, MPI_CHAR, succ, tag_recv_s, MPI_COMM_WORLD, &status);
                    check += MPI_Send(my_grid+my_n_cells-x_size, x_size, MPI_CHAR, succ, tag_send, MPI_COMM_WORLD);
                    check += MPI_Recv(above_line, x_size, MPI_CHAR, prev, tag_recv_p, MPI_COMM_WORLD, &status);

                } else {
                    
                    check += MPI_Recv(below_line, x_size, MPI_CHAR, succ, tag_recv_s, MPI_COMM_WORLD, &status);
                    check += MPI_Send(my_grid, x_size, MPI_CHAR, prev, tag_send, MPI_COMM_WORLD);
                    check += MPI_Recv(above_line, x_size, MPI_CHAR, prev, tag_recv_p, MPI_COMM_WORLD, &status);
                    check += MPI_Send(my_grid+my_n_cells-x_size, x_size, MPI_CHAR, succ, tag_send, MPI_COMM_WORLD);

                }

                if (check != 0 && error_control_1 == 0) {
                    printf("--- AN ERROR ON COMMUNICATION TO OR FROM PROCESS %d OCCURRED ---\n", my_id);
                    error_control_1 = 1;   // to avoid a large number of error messages
                    check = 0;
                }


                /* updating first row */
                
                /* first element of the first row */
                char count = 0;

                count += my_grid[1];
                for (int b=x_size-1; b<x_size+2; b++) {
                    count += my_grid[b];
                }
                count += my_grid[2*x_size-1];
                for (int b=0; b<2; b++) {
                    count += above_line[b];
                }
                count += above_line[x_size-1];

                if (count == 2 || count == 3) {
                    my_grid_aux[0] = 1;
                } else {
                    my_grid_aux[0] = 0;
                }

                /* internal elements of the first row */
                for (int j=1; j<x_size-1; j++) {

                    count = 0;
                    count += my_grid[j-1];
                    count += my_grid[j+1];
                    for (int b=x_size+j-1; b<x_size+j+2; b++) {
                        count += my_grid[b];
                    }
                    for (int b=j-1; b<j+2; b++) {
                        count += above_line[b];
                    }

                    if (count == 2 || count == 3) {
                        my_grid_aux[j] = 1;
                    } else {
                        my_grid_aux[j] = 0;
                    }
                }

                /* last element of the first row */
                count = 0;
                count += my_grid[0];
                count += my_grid[x_size-2];
                count += my_grid[x_size];
                for (int b=2*x_size-2; b<2*x_size; b++) {
                    count += my_grid[b];
                }
                count += above_line[0];
                for (int b=x_size-2; b<x_size; b++) {
                    count += above_line[b];
                }

                if (count == 2 || count == 3) {
                    my_grid_aux[x_size-1] = 1;
                } else {
                    my_grid_aux[x_size-1] = 0;
                }


                /* updating internal cells */ 

                /* iteration on internal rows */
                for (int i=1; i<my_y_size-1; i++) {

                    /* first element of the row */
                    count = 0;
                    for (int b=(i-1)*x_size; b<(i-1)*x_size+2; b++) {
                        count += my_grid[b];
                    }
                    count += my_grid[i*x_size-1];
                    count += my_grid[i*x_size+1];
                    for (int b=(i+1)*x_size-1; b<(i+1)*x_size+2; b++) {
                        count += my_grid[b];
                    }
                    count += my_grid[(i+2)*x_size-1]; 

                    if (count == 2 || count == 3) {
                        my_grid_aux[i*x_size] = 1;
                    } else {
                        my_grid_aux[i*x_size] = 0;
                    }

                    /* iteration on internal columns */
                    for (int j=1; j<x_size-1; j++) {

                        count = 0;
                        for (int b=(i-1)*x_size+j-1; b<(i-1)*x_size+j+2; b++) {
                            count += my_grid[b];
                        }
                        count += my_grid[i*x_size+j-1];
                        count += my_grid[i*x_size+j+1];
                        for (int b=(i+1)*x_size+j-1; b<(i+1)*x_size+j+2; b++) {
                            count += my_grid[b];
                        }

                        if (count == 2 || count == 3) {
                            my_grid_aux[i*x_size+j] = 1;
                        } else {
                            my_grid_aux[i*x_size+j] = 0;
                        }
                    }

                    /* last element of the row */
                    count = 0;
                    count += my_grid[(i-1)*x_size];
                    for (int b=i*x_size-2; b<i*x_size+1; b++) {
                        count += my_grid[b];
                    }
                    count += my_grid[(i+1)*x_size-2];
                    count += my_grid[(i+1)*x_size];
                    for (int b=(i+2)*x_size-2; b<(i+2)*x_size; b++) {
                        count += my_grid[b];
                    }

                    if (count == 2 || count == 3) {
                        my_grid_aux[(i+1)*x_size-1] = 1;
                    } else {
                        my_grid_aux[(i+1)*x_size-1] = 0;
                    }
                }


                /* updating last row */
                
                /* first element of the last row */
                count = 0;
                for (int b=(my_y_size-2)*x_size; b<(my_y_size-2)*x_size+2; b++) {
                    count += my_grid[b];
                }
                count += my_grid[my_n_cells-x_size-1];
                count += my_grid[my_n_cells-x_size+1];
                count += my_grid[my_n_cells-1];
                for (int b=0; b<2; b++) {
                    count += below_line[b];
                }
                count += below_line[x_size-1];

                if (count == 2 || count == 3) {
                    my_grid_aux[my_n_cells-x_size] = 1;
                } else {
                    my_grid_aux[my_n_cells-x_size] = 0;
                }

                /* internal elements of the last row */
                for (int j=1; j<x_size-1; j++) {

                    count = 0;
                    for (int b=my_n_cells-2*x_size+j-1; b<my_n_cells-2*x_size+j+2; b++) {
                        count += my_grid[b];
                    }
                    count += my_grid[my_n_cells-x_size+j-1];
                    count += my_grid[my_n_cells-x_size+j+1];
                    for (int b=j-1; b<j+2; b++) {
                        count += below_line[b];
                    }

                    if (count == 2 || count == 3) {
                        my_grid_aux[my_n_cells-x_size+j] = 1;
                    } else {
                        my_grid_aux[my_n_cells-x_size+j] = 0;
                    }
                }

                /* last element of the last row */
                count = 0;
                count += my_grid[my_n_cells-2*x_size];
                for (int b=my_n_cells-x_size-2; b<my_n_cells-x_size+1; b++) {
                    count += my_grid[b];
                }
                count += my_grid[my_n_cells-2];
                count += below_line[0];
                for (int b=x_size-2; b<x_size; b++) {
                    count += below_line[b];
                }

                if (count == 2 || count == 3) {
                    my_grid_aux[my_n_cells-1] = 1;
                } else {
                    my_grid_aux[my_n_cells-1] = 0;
                }
         

                /* switching pointers to grid and grid_aux */
                
                temp = my_grid;
                my_grid = my_grid_aux;
                my_grid_aux = temp;
                temp = NULL;


                /* writing a dump of the system */ 

                if (s != 0) {

                    if (gen % s == 0) {


                        sprintf(snap_name, "snapshots/snapshot_%05d.pgm", gen+1);

                        /* formatting the PGM file */ 
                        if (my_id == 0) {

                            /* setting the header */
                            char header[header_size];
                            sprintf(header, "P5 %d %d\n%d\n", x_size, y_size, color_maxval);

                            /* writing the header */
                            access_mode = MPI_MODE_CREATE | MPI_MODE_WRONLY;
                            check += MPI_File_open(MPI_COMM_SELF, snap_name, access_mode, MPI_INFO_NULL, &f_handle);
                            check += MPI_File_write_at(f_handle, 0, header, header_size, MPI_CHAR, &status);
                            check += MPI_File_close(&f_handle);
                        
                            if (check != 0 && error_control_2 == 0) {
                                printf("--- AN ERROR OCCURRED WHILE WRITING THE HEADER OF THE SYSTEM DUMP NUMBER %d ---\n", gen/s);
                                error_control_2 = 1;   // to avoid a large number of error messages
                                check = 0; 
                            }
                        }


                        /* needed to make sure that all processes are actually 
                        * wrtiting on an already formatted PGM file */
                        MPI_Barrier(MPI_COMM_WORLD);

                
                        /* opening file in parallel */
                        access_mode = MPI_MODE_APPEND | MPI_MODE_WRONLY;
                        check += MPI_File_open(MPI_COMM_WORLD, snap_name, access_mode, MPI_INFO_NULL, &f_handle);

                        /* computing offsets */
	                    offset = header_size;
	                    if (my_id < y_size_rmd) {
		                    offset += my_id*my_n_cells;
	                    } else {
		                    offset += y_size_rmd*x_size + my_id*my_n_cells;
	                    }

                        /* writing in parallel */
                        check += MPI_File_write_at_all(f_handle, offset, my_grid, my_n_cells, MPI_CHAR, &status);

                        check += MPI_File_close(&f_handle);

                        if (check != 0 && error_control_3 == 0) {
                            printf("--- AN I/O ERROR OCCURRED ON PROCESS %d WHILE WRITING THE SYSTEM DUMP NUMBER %d ---\n", my_id, gen/s);
                            error_control_3 = 1;   // to avoid a large number of error messages
                            check = 0;
                        }

                    }
                }

            }

	}


	/* writing the final state */

	sprintf(snap_name, "snapshots/final_state.pgm");

        /* formatting the PGM file */ 
        if (my_id == 0) {

            /* setting the header */
            char header[header_size];
            sprintf(header, "P5 %d %d\n%d\n", x_size, y_size, color_maxval);

            /* writing the header */
            access_mode = MPI_MODE_CREATE | MPI_MODE_WRONLY;
            check += MPI_File_open(MPI_COMM_SELF, snap_name, access_mode, MPI_INFO_NULL, &f_handle);
            check += MPI_File_write_at(f_handle, 0, header, header_size, MPI_CHAR, &status);
 		        check += MPI_File_close(&f_handle);

            if (check != 0 && error_control_2 == 0) {
                printf("--- AN ERROR OCCURRED WHILE WRITING THE HEADER OF THE FINAL STATE OF THE SYSTEM ---\n");
                error_control_2 = 1;   // to avoid a large number of error messages
                check = 0; 
            }
        }


        /* needed to make sure that all processes are actually 
        * wrtiting on an already formatted PGM file */
        MPI_Barrier(MPI_COMM_WORLD);
            

        /* opening file in parallel */
        access_mode = MPI_MODE_APPEND | MPI_MODE_WRONLY;
        check += MPI_File_open(MPI_COMM_WORLD, snap_name, access_mode, MPI_INFO_NULL, &f_handle);

        /* computing offsets */
	offset = header_size;
	if (my_id < y_size_rmd) {
	    offset += my_id*my_n_cells;
	} else {
	    offset += y_size_rmd*x_size + my_id*my_n_cells;
	}

        /* writing in parallel */
        check += MPI_File_write_at_all(f_handle, offset, my_grid, my_n_cells, MPI_CHAR, &status);

	check += MPI_File_close(&f_handle);

	if (check != 0 && error_control_3 == 0) {
            printf("--- AN I/O ERROR OCCURRED ON PROCESS %d WHILE WRITING THE FINAL STATE OF THE SYSTEM ---\n", my_id);
            check = 0;
	}
            
	    
	free(snap_name);
	free(above_line);
        free(my_grid);
	free(below_line);


    }



    MPI_Finalize();


    if (fname != NULL) {
        free(fname);
        fname = NULL;
    }


    return 0;
}



/* function to get content and length of the header of a pgm file (modified version of prof. Tornatore's
 * function to read from pgm) */
int read_pgm_header(unsigned int* head, const char* fname) {


    FILE* image_file;
    image_file = fopen(fname, "r"); 

    head[0] = head[1] = head[2] = head[3] = 0;

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

        k = sscanf(line, "%d%*c%d%*c%d%*c", &head[1], &head[2], &head[0]);
        if (k < 3)
            fscanf(image_file, "%d%*c", &head[0]);
    
    } else {

        fclose(image_file);
        free(line);
        return 1;   /* error in reading the header */
    }

    /* getting header size */ 
    fseek(image_file, 0L, SEEK_END);
    long unsigned int total_size = ftell(image_file);
    head[3] = total_size - head[1]*head[2];


    fclose(image_file);
    free(line);
    return 0;
}
