#define BOOL char


int read_pgm_header(unsigned int*, const char*);
int ordered_evo(BOOL*, const int, const int, int, int, const int, const int, const int, int, int, MPI_Status, const int, const int, const int, const int);
void static_evo(BOOL*, BOOL*, const int, int, int, const int, const int, const int);
void static_evo_in_place(BOOL*, const int, int, int, const int, const int, const int, int);
