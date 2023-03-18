### MKL libraries
###
###
MKL= -L${MKLROOT}/lib/intel64  -lmkl_intel_lp64 -lmkl_gnu_thread -lmkl_core -lgomp -lpthread -lm -ldl

### OpenBLAS libraries 
OPENBLASROOT=${OPENBLAS_ROOT}
### BLIS library
BLISROOT=/u/dssc/ttarch00/myblis


float: ${data}/gemm_mkl_f.x ${data}/gemm_oblas_f.x ${data}/gemm_blis_f.x

${data}/gemm_mkl_f.x: gemm.c
	gcc -DUSE_FLOAT -DMKL $^ -m64 -I${MKLROOT}/include $(MKL)  -o $@ -DSAVE_RESULTS

${data}/gemm_oblas_f.x: gemm.c
	gcc -DUSE_FLOAT -DOPENBLAS $^ -m64 -I${OPENBLASROOT}/include -L/${OPENBLASROOT}/lib -lopenblas -lpthread -o $@ -fopenmp -DSAVE_RESULTS

${data}/gemm_blis_f.x: gemm.c
	gcc -DUSE_FLOAT  -DBLIS $^ -m64 -I${BLISROOT}/include/blis -L/${BLISROOT}/lib -o $@ -lpthread  -lblis -fopenmp -lm -DSAVE_RESULTS


double: ${data}/gemm_mkl_d.x ${data}/gemm_oblas_d.x ${data}/gemm_blis_d.x 

${data}/gemm_mkl_d.x: gemm.c
	gcc -DUSE_DOUBLE -DMKL $^ -m64 -I${MKLROOT}/include $(MKL)  -o $@ -DSAVE_RESULTS

${data}/gemm_oblas_d.x: gemm.c
	gcc -DUSE_DOUBLE -DOPENBLAS $^ -m64 -I${OPENBLASROOT}/include -L/${OPENBLASROOT}/lib -lopenblas -lpthread -o $@ -fopenmp -DSAVE_RESULTS

${data}/gemm_blis_d.x: gemm.c
	gcc -DUSE_DOUBLE -DBLIS $^ -m64 -I${BLISROOT}/include/blis -L/${BLISROOT}/lib -o $@ -lpthread  -lblis -fopenmp -lm -DSAVE_RESULTS


clean:
	rm -rf ${data}/*.x
