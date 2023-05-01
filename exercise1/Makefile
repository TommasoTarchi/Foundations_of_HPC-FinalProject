### variables passed from command line
target_path=${data_folder}



gol_all: serial_gol parallel_gol

serial_gol: $(target_path)/serial_gol.x

$(target_path)/serial_gol.x: src/serial_gol.c
	gcc -DTIME $^ -o $@

parallel_gol: $(target_path)/parallel_gol.x

$(target_path)/parallel_gol.x: src/parallel_gol.c
	#mpicc -fopenmp -DTIME src/parallel_gol.c src/gol_lib.c -c
	#ar -rc libgol.a *.o
	#mpicc -fopenmp -DTIME src/parallel_gol.c -L. -lgol -o ${target_path}/parallel_gol.x
	mpicc -fopenmp -DTIME src/parallel_gol_unique.c -o ${target_path}/parallel_gol_unique.x


clean_all: clean_exe clean_images

clean_exe:
	rm $(target_path)/*.x *.o *.a

clean_images:
	rm images/snapshots/*.pgm
	rm images/*.pgm
