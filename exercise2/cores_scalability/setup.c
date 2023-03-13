#include <stdio.h>

int main() {

	FILE* f;

#ifdef USE_FLOAT
#ifdef MKL
	f = fopen("mkl_f.out", "w");
	fprintf(f, "node:        EPYC\nlibrary:     MKL\nprecision:   float\n\n");
	fprintf(f, "mat_size\ttime\t   GFLOPS\n");
	fclose(f);
#endif
#ifdef OPENBLAS
	f = fopen("oblas_f.out", "w");
	fprintf(f, "node:        EPYC\nlibrary:     MKL\nprecision:   float\n\n");
	fprintf(f, "mat_size\ttime\t   GFLOPS\n");
	fclose(f);
#endif
#ifdef BLIS
	f = fopen("blis_f.out", "w");
	fprintf(f, "node:        EPYC\nlibrary:     MKL\nprecision:   float\n\n");
	fprintf(f, "mat_size\ttime\t   GFLOPS\n");
	fclose(f);
#endif
#endif

#ifdef USE_DOUBLE
#ifdef MKL
	f = fopen("mkl_d.out", "w");
	fprintf(f, "node:        EPYC\nlibrary:     MKL\nprecision:   float\n\n");
	fprintf(f, "mat_size\ttime\t   GFLOPS\n");
	fclose(f);
#endif
#ifdef OPENBLAS
	f = fopen("oblas_d.out", "w");
	fprintf(f, "node:        EPYC\nlibrary:     MKL\nprecision:   float\n\n");
	fprintf(f, "mat_size\ttime\t   GFLOPS\n");
	fclose(f);
#endif
#ifdef BLIS
        f = fopen("blis_d.out", "w");
	fprintf(f, "node:        EPYC\nlibrary:     MKL\nprecision:   float\n\n");
	fprintf(f, "mat_size\ttime\t   GFLOPS\n");
	fclose(f);
#endif
#endif

	return 0;
}
