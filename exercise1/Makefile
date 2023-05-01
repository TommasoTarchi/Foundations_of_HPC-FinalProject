### variables passed from command line
target_path=${data_folder}



gol: serial_gol parallel_gol

serial_gol: $(target_path)/serial_gol.x

$(target_path)/serial_gol.x: src/serial_gol.c
	gcc -DTIME $^ -o $@

parallel_gol: $(target_path)/parallel_gol.x

$(target_path)/parallel_gol.x: src/parallel_gol.c
	mpicc -fopenmp src/parallel_gol.c src/gol_lib.c -c src/parallel_gol.o src/gol_lib.o
	ar -rc src/libgol.a src/*.o
	mpicc -fopenmp src/parallel_gol.c -Lsrc -lsrc/gol -o ${target_path}/parallel_gol.x



clean: clean_exe clean_images

clean_exe:
	rm $(target_path)/*.x

clean_images:
	rm images/snapshots/*.pgm
	rm images/*.pgm
