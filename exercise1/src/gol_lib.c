#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <mpi.h>
#include <omp.h>



#define BOOL char



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



void ordered_evo(int n, int s, BOOL* my_grid, int my_id, const int my_n_cells, const int header_size, const int x_size, const int y_size, const int y_size_rmd, const int color_maxval, int n_threads, int my_thread_start, int first_edge, const int first_row, const int last_row, const int my_thread_stop, int n_procs, int my_thread_id, MPI_Status status, const int prev, const int succ, const int tag_send, const int tag_recv_p, const int tag_recv_s)  {


    char* snap_name = (char*) malloc(50*sizeof(char));   // string to store name of snapshot files 

    MPI_File f_handle;   // pointer to file for MPI I/O
    int access_mode;   // variable to store access mode in MPI I/O
    int check = 0;   // error checker for MPI I/O and communications



    for (int gen=0; gen<n; gen++) {


       #pragma omp barrier
       #pragma omp master
        {


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
            
                        if (check != 0) {
                            printf("\n--- AN ERROR OCCURRED WHILE WRITING THE HEADER OF THE SYSTEM DUMP NUMBER %d ---\n\n", gen/s); 
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

                    if (check != 0) {
                        printf("\n--- AN I/O ERROR OCCURRED ON PROCESS %d WHILE WRITING THE SYSTEM DUMP NUMBER %d ---\n\n", my_id, gen/s);
                        check = 0;
                    }

                }
            }


        }


        /* iteration on processes */
        for (int proc=0; proc<n_procs; proc++) {


           #pragma omp barrier
           #pragma omp master
            {

                /* getting neighbor cells' status */ 


                if (n_procs > 1) {

                    if (proc == 0) {

                        if (my_id == n_procs-1)
                            check += MPI_Send(my_grid+my_n_cells, x_size, MPI_CHAR, succ, tag_send, MPI_COMM_WORLD);
        
                        if (my_id == 1)    
                            check += MPI_Send(my_grid+x_size, x_size, MPI_CHAR, prev, tag_send, MPI_COMM_WORLD);

                        if (my_id == 0) {
                            check += MPI_Recv(my_grid, x_size, MPI_CHAR, prev, tag_recv_p, MPI_COMM_WORLD, &status);
                            check += MPI_Recv(my_grid+x_size+my_n_cells, x_size, MPI_CHAR, succ, tag_recv_s, MPI_COMM_WORLD, &status);
                        }
                    
                        if (check != 0) {
                            printf("\n--- AN ERROR OCCURRED WHILE COMMUNICATING NEIGHBOR CELLS' STATUS OF PROCESS %d ---\n\n", proc);
                            check = 0;
                        }

                    } else if (proc == n_procs-1) {

                        if (my_id == n_procs-2)
                            check += MPI_Send(my_grid+my_n_cells, x_size, MPI_CHAR, succ, tag_send, MPI_COMM_WORLD);
            
                        if (my_id == 0)
                            check += MPI_Send(my_grid+x_size, x_size, MPI_CHAR, prev, tag_send, MPI_COMM_WORLD);

                        if (my_id == n_procs-1) {
                            check += MPI_Recv(my_grid, x_size, MPI_CHAR, prev, tag_recv_p, MPI_COMM_WORLD, &status);
                            check += MPI_Recv(my_grid+x_size+my_n_cells, x_size, MPI_CHAR, succ, tag_recv_s, MPI_COMM_WORLD, &status);

                        }
                
                        if (check != 0) {
                            printf("\n--- AN ERROR OCCURRED WHILE COMMUNICATING NEIGHBOR CELLS' STATUS OF PROCESS %d ---\n\n", proc);
                            check = 0;
                        }

                    } else if (n_procs > 2) {

                        if (my_id == proc-1) {
                            check += MPI_Send(my_grid+my_n_cells, x_size, MPI_CHAR, succ, tag_send, MPI_COMM_WORLD);
        
                        } else if (my_id == proc+1) { 
                            check += MPI_Send(my_grid+x_size, x_size, MPI_CHAR, prev, tag_send, MPI_COMM_WORLD);

                        } else if (my_id == proc) {
                            check += MPI_Recv(my_grid, x_size, MPI_CHAR, prev, tag_recv_p, MPI_COMM_WORLD, &status);
                            check += MPI_Recv(my_grid+x_size+my_n_cells, x_size, MPI_CHAR, succ, tag_recv_s, MPI_COMM_WORLD, &status);
                        }
                
                        if (check != 0) {
                            printf("\n--- AN ERROR OCCURRED WHILE COMMUNICATING NEIGHBOR CELLS' STATUS OF PROCESS %d ---\n\n", proc);
                            check = 0;
                        }

                    }

                
                /* case of single MPI process */
                } else {

                    check += MPI_Send(my_grid+my_n_cells, x_size, MPI_CHAR, succ, tag_send, MPI_COMM_WORLD);
                    check += MPI_Recv(my_grid, x_size, MPI_CHAR, prev, tag_recv_p, MPI_COMM_WORLD, &status);

                    if (check != 0) {
                        printf("\n--- AN ERROR OCCURRED WHILE COMMUNICATING NEIGHBOR CELLS' STATUS OF PROCESS %d ---\n\n", proc);
                        check = 0;
                    }

                }


            }
           #pragma omp barrier


            if (my_id == proc) {


                /* updating cells' status */

               #pragma omp for ordered
                for (int th=0; th<n_threads; th++) {


                    char count;   // counter of alive neighbor cells
                    int position = my_thread_start;   // position of the cell to be updated 


                    /* updating cells preceding the first edge */

                    for ( ; position<first_edge; position++) {

                        count = 0;
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


                    /* updating first edge encountered */
                    
                    if (position == first_edge) {

                        count = 0;
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


                    /* iteration on first complete row */ 
                    
                    /* updating first element of the row */
                    position = first_row*x_size;
                    count = 0;

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

                    /* iteration on internal columns (updating internal elements
                    * of te row) */
                    for (int j=1; j<x_size-1; j++) {

                        count = 0;
                        position = first_row*x_size+j;
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

                    /* updating last element of the row */
                    count = 0;
                    position = (first_row+1)*x_size-1;
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


                    /* communicating updated first row in case of single MPI process */ 
                    if (n_procs == 1) {
                       
                        if (my_thread_id == 0) {

                            check += MPI_Send(my_grid+x_size, x_size, MPI_CHAR, prev, tag_send, MPI_COMM_WORLD);
                            check += MPI_Recv(my_grid+x_size+my_n_cells, x_size, MPI_CHAR, succ, tag_recv_s, MPI_COMM_WORLD, &status);
                        }
                    }

                     
                    /* iteration on other 'complete' rows */

                    for (int i=first_row+1; i<last_row; i++) {

                        /* updating first element of the row */ 
                        position = i*x_size;
                        count = 0;
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

                        /* iteration on internal columns (updating internal elements
                        * of te row) */
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

                        /* updating last element of the row */
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

                    position++;


                    /* checking if all cells have been already updated */ 
                    if (position < my_thread_stop) {

                        /* updating first element after last row */ 

                        count = 0;
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

                        position++;


                        /* updating remaining elements */ 

                        for ( ; position<my_thread_stop; position++) {

                            count = 0;
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
                    }

                }

                if (check != 0) {
                    printf("\n--- AN I/O ERROR OCCURRED WHILE EVOLVING PLAYGROUND ON PROCESS %d ---\n\n", proc);
                    check = 0;
                }


            }

        }


       #pragma omp barrier
       #pragma omp master
        {

            MPI_Barrier(MPI_COMM_WORLD);
        }


    }


    return;
}
