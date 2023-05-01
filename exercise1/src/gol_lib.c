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



/* function for ordered evolution */
int ordered_evo(BOOL* my_grid, const int my_n_cells, const int x_size, int my_thread_start, int first_edge, const int first_row, const int last_row, const int my_thread_stop, int n_procs, int my_thread_id, MPI_Status status, const int prev, const int succ, const int tag_send, const int tag_recv_s) {


    char count;   // counter of alive neighbor cells
    int position = my_thread_start;   // position of the cell to be updated 
    int check = 0;


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


    return check;
}



/* function for static evolution with auxiliary grid */
void static_evo(BOOL* my_grid, BOOL* my_grid_aux, const int x_size, int my_thread_start, int first_edge, const int first_row, const int last_row, const int my_thread_stop) {


    char count;   // counter of alive neighbor cells
    int position = my_thread_start;   // position of the cell to update


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
            my_grid_aux[position] = 1;
        } else {
            my_grid_aux[position] = 0;
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
            my_grid_aux[position] = 1;
        } else {
            my_grid_aux[position] = 0;
        }

    }

     
    /* iteration on 'complete' rows */

    for (int i=first_row; i<last_row; i++) {

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
            my_grid_aux[position] = 1;
        } else {
            my_grid_aux[position] = 0;
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
                my_grid_aux[position] = 1;
            } else {
                my_grid_aux[position] = 0;
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
            my_grid_aux[position] = 1;
        } else {
            my_grid_aux[position] = 0;
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
            my_grid_aux[position] = 1;
        } else {
            my_grid_aux[position] = 0;
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
                my_grid_aux[position] = 1;
            } else {
                my_grid_aux[position] = 0;
            }
        }
    }


    return;   
}



void static_evo_in_place(BOOL* my_grid, const int x_size, int my_thread_start, int first_edge, const int first_row, const int last_row, const int my_thread_stop, int bit_control) {


    char alive = 2 - bit_control % 2;
    char dead = 1 + bit_control % 2;
    char shift = bit_control % 2;

    char count;
    int position = my_thread_start;


    /* updating cells preceding the first edge */

    for ( ; position<first_edge; position++) {

        count = 0;
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


    /* updating first edge encountered */
    
    if (position == first_edge) {

        count = 0;
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

     
    /* iteration on 'complete' rows */

    for (int i=first_row; i<last_row; i++) {

        /* updating first element of the row */ 
        position = i*x_size;
        count = 0;
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

        /* iteration on internal columns (updating internal elements
        * of te row) */
        for (int j=1; j<x_size-1; j++) {

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

        /* updating last element of the row */
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

    position++;


    /* checking if all cells have been already updated */ 
    if (position != my_thread_stop) {

        /* updating first element after last row */ 

        count = 0;
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

        position++;


        /* updating remaining elements */ 

        for ( ; position<my_thread_stop; position++) {

            count = 0;
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
    }


    return;
}
