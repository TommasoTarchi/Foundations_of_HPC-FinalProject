### MKL libraries
###
###
MKL= -L${MKLROOT}/lib/intel64  -lmkl_intel_lp64 -lmkl_gnu_thread -lmkl_core -lgomp -lpthread -lm -ldl

### OpenBLAS libraries 
OPENBLASROOT=${OPENBLAS_ROOT}
### BLIS library
BLISROOT=/u/dssc/ttarch00/myblis


setup: setup.c
	gcc setup.c -o setup.x -DUSE_FLOAT -DUSE_DOUBLE -DMKL -DOPENBLAS -DBLIS


float: gemm_mkl_f.x gemm_oblas_f.x gemm_blis_f.x

gemm_mkl_f.x: gemm.c
	gcc -DUSE_FLOAT -DMKL $^ -m64 -I${MKLROOT}/include $(MKL)  -o $@ -DMAKE_STAT

gemm_oblas_f.x: gemm.c
	gcc -DUSE_FLOAT -DOPENBLAS $^ -m64 -I${OPENBLASROOT}/include -L/${OPENBLASROOT}/lib -lopenblas -lpthread -o $@ -fopenmp -DMAKE_STAT

gemm_blis_f.x: gemm.c
	gcc -DUSE_FLOAT  -DBLIS $^ -m64 -I${BLISROOT}/include/blis -L/${BLISROOT}/lib -o $@ -lpthread  -lblis -fopenmp -lm -DMAKE_STAT


double: gemm_mkl_d.x gemm_oblas_d.x gemm_blis_d.x 

gemm_mkl_d.x: gemm.c
	gcc -DUSE_DOUBLE -DMKL $^ -m64 -I${MKLROOT}/include $(MKL)  -o $@ -DMAKE_STAT

gemm_oblas_d.x: gemm.c
	gcc -DUSE_DOUBLE -DOPENBLAS $^ -m64 -I${OPENBLASROOT}/include -L/${OPENBLASROOT}/lib -lopenblas -lpthread -o $@ -fopenmp -DMAKE_STAT

gemm_blis_d.x: gemm.c
	gcc -DUSE_DOUBLE -DBLIS $^ -m64 -I${BLISROOT}/include/blis -L/${BLISROOT}/lib -o $@ -lpthread  -lblis -fopenmp -lm -DMAKE_STAT


clean:
	rm -rf *.x
