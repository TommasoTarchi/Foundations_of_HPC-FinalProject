#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>


#define K_DEFAULT 100


int main(int argc, char** argv) {

    // this is the string to which getopt will compare
    // the command line arguments; getopt returns the
    // arguments that were passed from command line 
    // every time I call it in sequence, and converted
    // to integer; when it finishes the arguments it
    // returns -1
    char* optstring="a:bcd";

    printf("%d\n", (char)getopt(argc, argv, optstring));

    printf("%d\n", (int)'a');
    printf("%d\n", (int)'b');
    printf("%d\n", (int)'c');
    
    return 0;
}
