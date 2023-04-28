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



int read_pgm_header(unsigned int*, const char*);



int main(int argc, char **argv) {



    /* getting options */
    int action = 0;
    char *optstring = "irm:k:e:f:n:s:";
    int m_length;   // needed for header size
    int k_length;   //

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
	       	if (m < 100) {
                    printf("\n--- MATRIX DIMENSIONS TOO SMALL: MUST BE AT LEAST 100x100 ---\n\n");
                    return 1;
                }
 		m_length = strlen(optarg);
                break;
            
            case 'k':
                k = atoi(optarg);
	       	if (k < 100) {
                    printf("\n--- MATRIX DIMENSIONS TOO SMALL: MUST BE AT LEAST 100x100 ---\n\n");
                    return 1;
                }
		k_length = strlen(optarg);
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

    int check = 0;   // error checker for MPI communications and I/O
    
    MPI_Comm_size(MPI_COMM_WORLD, &n_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_id);


    // test
    printf("I'm process %d\n", my_id);



    /* needed for timing */
#ifdef TIME
    struct timespec ts;
    double t_start;
#endif



    /* initializing a playground */
    if (action == INIT) {



#ifdef TIME
    MPI_Barrier(MPI_COMM_WORLD);
    if (my_id == 0) {
        t_start = CPU_TIME;
    }
#endif



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

        /* setting the seed */
        struct drand48_data rand_gen;
        /* generating a unique seed for each process */
        long int seed;
        if (my_id == 0)   // in this way we are sure that the seed will be different for all processes
            seed = time(NULL);
        check += MPI_Bcast(&seed, 1, MPI_LONG, 0, MPI_COMM_WORLD);
        if (check != 0) {
            printf("\n--- AN ERROR OCCURRED WHILE BROADCASTING THE SEED ---\n\n");
            check = 0;
        }
        seed += my_id;
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
        const int color_maxval = 1;
        /* the third number added to header_size must be equal to the total length 
         * in bytes of the header, spaces and new-line characters included and 
         * length of m and k excluded */
     	const int header_size = m_length+k_length+7;

        if (my_id == 0) {

            /* setting the header */
            char header[header_size];
            sprintf(header, "P5 %d %d\n%d\n", k, m, color_maxval);

            /* writing the header */
            int access_mode = MPI_MODE_CREATE | MPI_MODE_WRONLY;
            check += MPI_File_open(MPI_COMM_SELF, fname, access_mode, MPI_INFO_NULL, &f_handle);
            check += MPI_File_write_at(f_handle, 0, header, header_size, MPI_CHAR, &status);
            check += MPI_File_close(&f_handle);
        }

        if (check != 0) {
            printf("\n--- AN I/O ERROR OCCURRED WHILE WRITING THE HEADER ---\n\n");
            check = 0;
        }


        /* needed to make sure that all processes are actually 
         * wrtiting on an already formatted PGM file */
        MPI_Barrier(MPI_COMM_WORLD);


        /* opening file in parallel */
        int access_mode = MPI_MODE_WRONLY;
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
            printf("\n--- AN I/O ERROR OCCURRED ON PROCESS %d WHILE WRITING THE INITIAL PLAYGROUND ---\n\n", my_id);

        free(my_grid);



#ifdef TIME
    MPI_Barrier(MPI_COMM_WORLD);
    if (my_id == 0) {
        double time = CPU_TIME - t_start;
        printf("elapsed time for initialization: %f sec\n\n", time);
    }
#endif



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
                printf("\n--- AN ERROR OCCURRED WHILE READING THE HEADER OF THE PGM FILE ---\n\n");
                check = 0;
            }
        }

        /* distributing header's information to all processes */
        check += MPI_Bcast(header_content, 4, MPI_INT, 0, MPI_COMM_WORLD);

	    if (check != 0) {
	        printf("\n--- AN ERROR OCCURRED WHILE BROADCASTING HEADER'S INFORMATIONS ON %d PROCESS ---\n\n", my_id);
	        check = 0;
	    }

        /* 'unpacking' header's information */
        const int color_maxval = header_content[0];
        const int x_size = header_content[1];
        const int y_size = header_content[2];
        const int header_size = header_content[3];

        const unsigned int n_cells = x_size*y_size;   // total number of cells
                                                      

        /* checking minimal dimensions */
        if (x_size < 100 || y_size < 100) {
            printf("\n--- MATRIX DIMENSIONS TOO SMALL: MUST BE AT LEAST 100x100 ---\n\n");
            return 1;
        }


        /* distributing work among MPI processes */

        /* setting process 'personal' varables */
        unsigned int my_y_size = y_size/n_procs;   // number of rows assigned to the process
        const int y_size_rmd = y_size%n_procs;
        /* assigning remaining rows */
        if (my_id < y_size_rmd)
            my_y_size++;
        const unsigned int my_n_cells = x_size*my_y_size;   // number of cells assigned to the process

        /* grid to store cells status and two more rows for neighbor cells */ 
        BOOL* my_grid = (BOOL*) malloc((my_n_cells+2*x_size)*sizeof(BOOL));

        MPI_File f_handle;   // pointer to file

        /* opening file in parallel */
	    int access_mode = MPI_MODE_RDONLY;
        check += MPI_File_open(MPI_COMM_WORLD, fname, access_mode, MPI_INFO_NULL, &f_handle);

        /* computing offsets */
	    int offset = header_size;
	    if (my_id < y_size_rmd) {
	        offset += my_id*my_n_cells;
	    } else {
	        offset += y_size_rmd*x_size + my_id*my_n_cells;
	    }

        /* setting the file pointer to offset */
        check += MPI_File_seek(f_handle, offset, MPI_SEEK_SET);
        /* reading in parallel */
        check += MPI_File_read_all(f_handle, my_grid+x_size, my_n_cells, MPI_CHAR, &status);

        check += MPI_Barrier(MPI_COMM_WORLD);

        check += MPI_File_close(&f_handle);

        if (check != 0) {
            printf("\n--- AN I/O ERROR OCCURRED ON PROCESS %d WHILE READING THE INITIAL PLAYGROUND ---\n\n", my_id);
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



	int bit_control = 0;   // needed to control the state signaling bit in static evolution



        if (e == ORDERED) {

#ifdef TIME
    MPI_Barrier(MPI_COMM_WORLD);
    if (my_id == 0) {
        t_start = CPU_TIME;
    }
#endif



            /* evolution */

            for (int gen=0; gen<n; gen++) {


		/* writing a dump of the system */ 

                if (s != 0) {

                    if (gen % s == 0) {


                        sprintf(snap_name, "../../images/snapshots/snapshot_%05d.pgm", gen);

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
                                printf("\n--- AN ERROR OCCURRED WHILE WRITING THE HEADER OF THE SYSTEM DUMP NUMBER %d ---\n\n", gen/s);
                                error_control_2 = 1;   // to avoid a large number of error messages
                                check = 0; 
                            }
                        }


                        /* needed to make sure that all processes are actually 
                        * wrtiting on an already formatted PGM file */
                        MPI_Barrier(MPI_COMM_WORLD);

                
                        /* opening file in parallel */
                        access_mode = MPI_MODE_WRONLY;
                        check += MPI_File_open(MPI_COMM_WORLD, snap_name, access_mode, MPI_INFO_NULL, &f_handle);

                        /* computing offsets */
	                    offset = header_size;
	                    if (my_id < y_size_rmd) {
		                    offset += my_id*my_n_cells;
	                    } else {
		                    offset += y_size_rmd*x_size + my_id*my_n_cells;
	                    }

                        /* writing in parallel */
                        check += MPI_File_write_at_all(f_handle, offset, my_grid+x_size, my_n_cells, MPI_CHAR, &status);

                        check += MPI_File_close(&f_handle);

                        if (check != 0 && error_control_3 == 0) {
                            printf("\n--- AN I/O ERROR OCCURRED ON PROCESS %d WHILE WRITING THE SYSTEM DUMP NUMBER %d ---\n\n", my_id, gen/s);
                            error_control_3 = 1;   // to avoid a large number of error messages
                            check = 0;
                        }

                    }
                }


                /* iteration on processes */
                for (int proc=0; proc<n_procs; proc++) {


                    /* getting neighbor cells' status */

                    if (proc == 0) {

                        if (my_id == n_procs-1) {

                            check += MPI_Send(my_grid+my_n_cells, x_size, MPI_CHAR, succ, tag_send, MPI_COMM_WORLD);
                
                        } else if (my_id == 1) {

                            check += MPI_Send(my_grid+x_size, x_size, MPI_CHAR, prev, tag_send, MPI_COMM_WORLD);

                        } else if (my_id == 0) {

                            check += MPI_Recv(my_grid, x_size, MPI_CHAR, prev, tag_recv_p, MPI_COMM_WORLD, &status);
                            check += MPI_Recv(my_grid+x_size+my_n_cells, x_size, MPI_CHAR, succ, tag_recv_s, MPI_COMM_WORLD, &status);
                        }
                        
                        if (check != 0 && error_control_1 == 0) {
                            printf("\n--- AN ERROR OCCURRED WHILE COMMUNICATING NEIGHBOR CELLS' STATUS OF PROCESS %d ---\n\n", proc);
                            check = 0;
                            error_control_1 = 1;   // to avoid a large number of error messages
                        }

                    } else if (proc == n_procs-1) {

                        if (my_id == n_procs-2) {

                            check += MPI_Send(my_grid+my_n_cells, x_size, MPI_CHAR, succ, tag_send, MPI_COMM_WORLD);
                
                        } else if (my_id == 0) {

                            check += MPI_Send(my_grid+x_size, x_size, MPI_CHAR, prev, tag_send, MPI_COMM_WORLD);

                        } else if (my_id == n_procs-1) {

                            check += MPI_Recv(my_grid, x_size, MPI_CHAR, prev, tag_recv_p, MPI_COMM_WORLD, &status);
                            check += MPI_Recv(my_grid+x_size+my_n_cells, x_size, MPI_CHAR, succ, tag_recv_s, MPI_COMM_WORLD, &status);
                        }
                        
                        if (check != 0 && error_control_1 == 0) {
                            printf("\n--- AN ERROR OCCURRED WHILE COMMUNICATING NEIGHBOR CELLS' STATUS OF PROCESS %d ---\n\n", proc);
                            check = 0;
                            error_control_1 = 1;   // to avoid a large number of error messages
                        }

                    } else {

                        if (my_id == proc-1) {

                            check += MPI_Send(my_grid+my_n_cells, x_size, MPI_CHAR, succ, tag_send, MPI_COMM_WORLD);
                
                        } else if (my_id == proc+1) {

                            check += MPI_Send(my_grid+x_size, x_size, MPI_CHAR, prev, tag_send, MPI_COMM_WORLD);

                        } else if (my_id == proc) {

                            check += MPI_Recv(my_grid, x_size, MPI_CHAR, prev, tag_recv_p, MPI_COMM_WORLD, &status);
                            check += MPI_Recv(my_grid+x_size+my_n_cells, x_size, MPI_CHAR, succ, tag_recv_s, MPI_COMM_WORLD, &status);
                        }
                        
                        if (check != 0 && error_control_1 == 0) {
                            printf("\n--- AN ERROR OCCURRED WHILE COMMUNICATING NEIGHBOR CELLS' STATUS OF PROCESS %d ---\n\n", proc);
                            check = 0;
                            error_control_1 = 1;   // to avoid a large number of error messages
                        }

                    }


                    if (my_id == proc) {



                        /* updating the cells' status */

                        char count;   // counter of alive neighbor cells
                        int position;   // position of the cell to update

                        /* iteration on rows */
                        for (int i=1; i<my_y_size+1; i++) {

                            /* first element of the row */ 
                            count = 0;
                            position = i*x_size;
                            for (int b=position-x_size; b<position-x_size+2; b++) {
                                count += my_grid[b];
                            }
                            count += my_grid[position-1];
                            count += my_grid[position+1];
                            for (int b=position+x_size-1; b<position+x_size+2; b++) {
                                count += my_grid[b];
                            }
                            count += my_grid[position+2*x_size-1];

                            if (count == 2 || count == 3) {
                                my_grid[position] = 1;
                            } else {
                                my_grid[position] = 0;
                            }

                            /* iteration on internal columns */
                            for (int j=1; j<x_size-1; j++) {

                                count = 0;
                                position = i*x_size+j;
                                for (int b=position-x_size-1; b<position-x_size+2; b++) {
                                    count += my_grid[b];
                                }
                                count += my_grid[position-1];
                                count += my_grid[position+1];
                                for (int b=position+x_size-1; b<position+x_size+2; b++) {
                                    count += my_grid[b];
                                }

                                if (count == 2 || count == 3) {
                                    my_grid[position] = 1;
                                } else {
                                    my_grid[position] = 0;
                                }
                            }

                            /* last element of the row */
                            count = 0;
                            position = (i+1)*x_size-1;
                            count += my_grid[position-2*x_size+1];
                            for (int b=position-x_size-1; b<position-x_size+2; b++) {
                                count += my_grid[b];
                            }
                            count += my_grid[position-1];
                            count += my_grid[position+1];
                            for (int b=position+x_size-1; b<position+x_size+1; b++) {
                                count += my_grid[b];
                            }

                            if (count == 2 || count == 3) {
                                my_grid[position] = 1;
                            } else {
                                my_grid[position] = 0;
                            }
                        }

                    }

                    MPI_Barrier(MPI_COMM_WORLD);

                }
            
            }



#ifdef TIME
    MPI_Barrier(MPI_COMM_WORLD);
    if (my_id == 0) {
        double time = CPU_TIME - t_start;
        printf("elapsed time for ordered evolution: %f sec\n\n", time);
    }
#endif



        } else if (e == STATIC) {


#ifdef TIME
    MPI_Barrier(MPI_COMM_WORLD);
    if (my_id == 0) {
        t_start = CPU_TIME;
    }
#endif



            /* auxiliary grid to store cells' status */
            BOOL* my_grid_aux = (BOOL*) malloc((my_n_cells+2*x_size)*sizeof(BOOL));
            /* temporary pointer used for grid switching */
            void* temp=NULL;

            
            /* evolution */

            for (int gen=0; gen<n; gen++) {


		/* writing a dump of the system */ 

                if (s != 0) {

                    if (gen % s == 0) {


                        sprintf(snap_name, "../../images/snapshots/snapshot_%05d.pgm", gen);

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
                        access_mode = MPI_MODE_WRONLY;
                        check += MPI_File_open(MPI_COMM_WORLD, snap_name, access_mode, MPI_INFO_NULL, &f_handle);

                        /* computing offsets */
	                    offset = header_size;
	                    if (my_id < y_size_rmd) {
		                    offset += my_id*my_n_cells;
	                    } else {
		                    offset += y_size_rmd*x_size + my_id*my_n_cells;
	                    }


                        // test
                        printf("after opening:  %d\n", gen);


                        /* writing in parallel */
                        check += MPI_File_write_at_all(f_handle, offset, my_grid+x_size, my_n_cells, MPI_CHAR, &status);

                        check += MPI_Barrier(MPI_COMM_WORLD);

                        check += MPI_File_close(&f_handle);

                        if (check != 0 && error_control_3 == 0) {
                            printf("--- AN I/O ERROR OCCURRED ON PROCESS %d WHILE WRITING THE SYSTEM DUMP NUMBER %d ---\n", my_id, gen/s);
                            error_control_3 = 1;   // to avoid a large number of error messages
                            check = 0;
                        }


                        // test
                        printf("after I/O:  %d\n", gen);

                    }
                }

                
		/* communicating border cells' status to neighbor processes */ 
                
                if (my_id % 2 == 0) {

                    check += MPI_Send(my_grid+x_size, x_size, MPI_CHAR, prev, tag_send, MPI_COMM_WORLD);
                    check += MPI_Recv(my_grid+x_size+my_n_cells, x_size, MPI_CHAR, succ, tag_recv_s, MPI_COMM_WORLD, &status);
                    check += MPI_Send(my_grid+my_n_cells, x_size, MPI_CHAR, succ, tag_send, MPI_COMM_WORLD);
                    check += MPI_Recv(my_grid, x_size, MPI_CHAR, prev, tag_recv_p, MPI_COMM_WORLD, &status);

                } else {
                     check += MPI_Recv(my_grid+x_size+my_n_cells, x_size, MPI_CHAR, succ, tag_recv_s, MPI_COMM_WORLD, &status);
                     check += MPI_Send(my_grid+x_size, x_size, MPI_CHAR, prev, tag_send, MPI_COMM_WORLD);
                     check += MPI_Recv(my_grid, x_size, MPI_CHAR, prev, tag_recv_p, MPI_COMM_WORLD, &status);
                     check += MPI_Send(my_grid+my_n_cells, x_size, MPI_CHAR, succ, tag_send, MPI_COMM_WORLD);
                  
                }

                if (check != 0 && error_control_1 == 0) {
                    printf("--- AN ERROR ON COMMUNICATION TO OR FROM PROCESS %d OCCURRED ---\n", my_id);
                    error_control_1 = 1;   // to avoid a large number of error messages
                    check = 0;
                }


                /* updating the cells' status */

                char count;   // counter of alive neighbor cells
                int position;   // position of the cell to update

                /* iteration on rows */
                for (int i=1; i<my_y_size+1; i++) {

                    /* first element of the row */ 
                    count = 0;
                    position = i*x_size;
                    for (int b=position-x_size; b<position-x_size+2; b++) {
                        count += my_grid[b];
                    }
                    count += my_grid[position-1];
                    count += my_grid[position+1];
                    for (int b=position+x_size-1; b<position+x_size+2; b++) {
                        count += my_grid[b];
                    }
                    count += my_grid[position+2*x_size-1];

                    if (count == 2 || count == 3) {
                        my_grid_aux[position] = 1;
                    } else {
                        my_grid_aux[position] = 0;
                    }

                    /* iteration on internal columns */
                    for (int j=1; j<x_size-1; j++) {

                        count = 0;
                        position = i*x_size+j;
                        for (int b=position-x_size-1; b<position-x_size+2; b++) {
                            count += my_grid[b];
                        }
                        count += my_grid[position-1];
                        count += my_grid[position+1];
                        for (int b=position+x_size-1; b<position+x_size+2; b++) {
                            count += my_grid[b];
                        }

                        if (count == 2 || count == 3) {
                            my_grid_aux[position] = 1;
                        } else {
                            my_grid_aux[position] = 0;
                        }
                    }

                    /* last element of the row */
                    count = 0;
                    position = (i+1)*x_size-1;
                    count += my_grid[position-2*x_size+1];
                    for (int b=position-x_size-1; b<position-x_size+2; b++) {
                        count += my_grid[b];
                    }
                    count += my_grid[position-1];
                    count += my_grid[position+1];
                    for (int b=position+x_size-1; b<position+x_size+1; b++) {
                        count += my_grid[b];
                    }

                    if (count == 2 || count == 3) {
                        my_grid_aux[position] = 1;
                    } else {
                        my_grid_aux[position] = 0;
                    }
                }

                /* switching pointers to grid and grid_aux */
                
                temp = my_grid;
                my_grid = my_grid_aux;
                my_grid_aux = temp;
                temp = NULL;

                // test
                printf("\tgenerations:  %d\n", gen);

            }



#ifdef TIME
    MPI_Barrier(MPI_COMM_WORLD);
    if (my_id == 0) {
        double time = CPU_TIME - t_start;
        printf("elapsed time for static evolution: %f sec\n\n", time);
    }
#endif




        } else if (e == STATIC_SAVE_MEM) {
            


#ifdef TIME
    MPI_Barrier(MPI_COMM_WORLD);
    if (my_id == 0) {
        double t_start = CPU_TIME;
    }
#endif


          
            /* evolution */

            for (int gen=0; gen<n; gen++) {


		        /* writing a dump of the system */ 

                if (s != 0) {

                    if (gen % s == 0) {

 
                        /* selecting the state signaling bit */
                        if (bit_control % 2 == 1) {

                            for (int i=0; i<my_n_cells+2*x_size; i++)
                                my_grid[i] >>= 1;				                        

                            bit_control++;
                        
                        } else {

                            for (int i=0; i<my_n_cells+2*x_size; i++)
                                my_grid[i] &= 1;
				
			            }


                        sprintf(snap_name, "../../images/snapshots/snapshot_%05d.pgm", gen);

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
                                printf("\n--- AN ERROR OCCURRED WHILE WRITING THE HEADER OF THE SYSTEM DUMP NUMBER %d ---\n\n", gen/s);
                                error_control_2 = 1;   // to avoid a large number of error messages
                                check = 0; 
                            }
                        }


                        /* needed to make sure that all processes are actually 
                        * wrtiting on an already formatted PGM file */
                        MPI_Barrier(MPI_COMM_WORLD);

                
                        /* opening file in parallel */
                        access_mode = MPI_MODE_WRONLY | MPI_MODE_APPEND;
                        check += MPI_File_open(MPI_COMM_WORLD, snap_name, access_mode, MPI_INFO_NULL, &f_handle);

                        /* computing offsets */
	                    offset = header_size;
	                    if (my_id < y_size_rmd) {
		                    offset += my_id*my_n_cells;
	                    } else {
		                    offset += y_size_rmd*x_size + my_id*my_n_cells;
	                    }

                        /* writing in parallel */
                        check += MPI_File_write_at_all(f_handle, offset, my_grid+x_size, my_n_cells, MPI_CHAR, &status);

                        check += MPI_File_close(&f_handle);

                        if (check != 0 && error_control_3 == 0) {
                            printf("\n--- AN I/O ERROR OCCURRED ON PROCESS %d WHILE WRITING THE SYSTEM DUMP NUMBER %d ---\n\n", my_id, gen/s);
                            error_control_3 = 1;   // to avoid a large number of error messages
                            check = 0;
                        }

                    }
                }


        		/* communicating border cells' status to neighbor processes */ 
                
                if (my_id % 2 == 0) {

                    check += MPI_Send(my_grid+x_size, x_size, MPI_CHAR, prev, tag_send, MPI_COMM_WORLD);
                    check += MPI_Recv(my_grid+x_size+my_n_cells, x_size, MPI_CHAR, succ, tag_recv_s, MPI_COMM_WORLD, &status);
                    check += MPI_Send(my_grid+my_n_cells, x_size, MPI_CHAR, succ, tag_send, MPI_COMM_WORLD);
                    check += MPI_Recv(my_grid, x_size, MPI_CHAR, prev, tag_recv_p, MPI_COMM_WORLD, &status);

                } else {
                     check += MPI_Recv(my_grid+x_size+my_n_cells, x_size, MPI_CHAR, succ, tag_recv_s, MPI_COMM_WORLD, &status);
                     check += MPI_Send(my_grid+x_size, x_size, MPI_CHAR, prev, tag_send, MPI_COMM_WORLD);
                     check += MPI_Recv(my_grid, x_size, MPI_CHAR, prev, tag_recv_p, MPI_COMM_WORLD, &status);
                     check += MPI_Send(my_grid+my_n_cells, x_size, MPI_CHAR, succ, tag_send, MPI_COMM_WORLD);
                  
                }

                if (check != 0 && error_control_1 == 0) {
                    printf("\n--- AN ERROR ON COMMUNICATION TO OR FROM PROCESS %d OCCURRED ---\n\n", my_id);
                    error_control_1 = 1;   // to avoid a large number of error messages
                    check = 0;
                }


                /* updating the cells' status */

                /* needed for evolution without allocating auxiliary grid */
                char alive = 2 - bit_control % 2;
                char dead = 1 + bit_control % 2;
                char shift = bit_control % 2;

                char count;
                int position;

                /* iteration on rows */
                for (int i=1; i<my_y_size+1; i++) {

                    /* first element of the row */
                    count = 0;
                    position = i*x_size;
                    for (int b=position-x_size; b<position-x_size+2; b++) {
                        count += (my_grid[b] >> shift) & 1;
                    }
                    count += (my_grid[position-1] >> shift) & 1;
                    count += (my_grid[position+1] >> shift) & 1;
                    for (int b=position+x_size-1; b<position+x_size+2; b++) {
                        count += (my_grid[b] >> shift) & 1;
                    }
                    count += (my_grid[position+2*x_size-1] >> shift) & 1;

                    if (count == 2 || count == 3) {
                        my_grid[position] |= alive;
                    } else {
                        my_grid[position] &= dead;
                    }

                    /* iteration on internal columns */
                    for (int j=1; j<x_size+1; j++) {

                        count = 0;
                        position = i*x_size+j;
                        for (int b=position-x_size-1; b<position-x_size+2; b++) {
                            count += (my_grid[b] >> shift) & 1;
                        }
                        count += (my_grid[position-1] >> shift) & 1;
                        count += (my_grid[position+1] >> shift) & 1;
                        for (int b=position+x_size-1; b<position+x_size+2; b++) {
                            count += (my_grid[b] >> shift) & 1;
                        }

                        if (count == 2 || count == 3) {
                            my_grid[position] |= alive;
                        } else {
                            my_grid[position] &= dead;
                        }
                    }

                    /* last element of the row */
                    count = 0;
                    position = (i+1)*x_size-1;
                    count += (my_grid[position-2*x_size+1] >> shift) & 1;
                    for (int b=position-x_size-1; b<position-x_size+2; b++) {
                        count += (my_grid[b] >> shift) & 1;
                    }
                    count += (my_grid[position-1] >> shift) & 1;
                    count += (my_grid[position+1] >> shift) & 1;
                    for (int b=position+x_size-1; b<position+x_size+1; b++) {
                        count += (my_grid[b] >> shift) & 1;
                    }

                    if (count == 2 || count == 3) {
                        my_grid[position] |= alive;
                    } else {
                        my_grid[position] &= dead;
                    }
                }


		bit_control++;

            }

         }


	/* writing the final state */
	
	/* selecting the state signaling bit */
	if (e == STATIC_SAVE_MEM) {

            if (bit_control % 2 == 1) {

            for (int i=x_size; i<my_n_cells+x_size; i++)
                my_grid[i] >>= 1;
                      
            } else {

                for (int i=x_size; i<my_n_cells+x_size; i++)
                    my_grid[i] &= 1;
		    
	    }
	}


	    sprintf(snap_name, "../../images/snapshots/final_state.pgm");

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
                printf("\n--- AN ERROR OCCURRED WHILE WRITING THE HEADER OF THE FINAL STATE OF THE SYSTEM ---\n\n");
                error_control_2 = 1;   // to avoid a large number of error messages
                check = 0; 
            }
        }


        /* needed to make sure that all processes are actually 
        * wrtiting on an already formatted PGM file */
        MPI_Barrier(MPI_COMM_WORLD);
            

        /* opening file in parallel */
        access_mode = MPI_MODE_WRONLY;
        check += MPI_File_open(MPI_COMM_WORLD, snap_name, access_mode, MPI_INFO_NULL, &f_handle);

        /* computing offsets */
	    offset = header_size;
	    if (my_id < y_size_rmd) {
	        offset += my_id*my_n_cells;
	    } else {
	        offset += y_size_rmd*x_size + my_id*my_n_cells;
	    }

        /* writing in parallel */
        check += MPI_File_write_at_all(f_handle, offset, my_grid+x_size, my_n_cells, MPI_CHAR, &status);

        check += MPI_Barrier(MPI_COMM_WORLD);

	    check += MPI_File_close(&f_handle);

	    if (check != 0 && error_control_3 == 0) {
            printf("\n--- AN I/O ERROR OCCURRED ON PROCESS %d WHILE WRITING THE FINAL STATE OF THE SYSTEM ---\n\n", my_id);
            check = 0;
	    }



#ifdef TIME
    MPI_Barrier(MPI_COMM_WORLD);
    if (my_id == 0) {
        double time = CPU_TIME - t_start;
        printf("elapsed time for static evolution: %f sec\n\n", time);
    }
#endif


    
	    free(snap_name);
        free(my_grid);

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
    //fseek(image_file, 0L, SEEK_END);
    //long unsigned int total_size = ftell(image_file);
    //head[3] = total_size - head[1]*head[2];


    /* getting header size */
    int size = 0;
    for (int i=0; i<3; i++) {
	int cipher = 9;
	int power = 10;
	size++;
	while (head[i] > cipher) {
	    size++;
	    cipher += 9*power;
	    power *= 10;
	}
    }
    head[3] = 6 + size;


    fclose(image_file);
    free(line);
    return 0;
}
