serial: ${target_path}/serial_gol.x

${target_path}/serial_gol.x: src/serial_gol.c
	gcc -DTIME $^ -o $@


parallel: ${target_path}/parallel_gol.x

${target_path}/parallel_gol.x: src/parallel_gol.c
	gcc -DTIME $^ -o $@


parallel_single_grid: ${target_path}/parallel_gol_single_grid.x

${target_path}/parallel_gol_single_grid.x: src/parallel_gol_single_grid.c
	gcc -DTIME $^ -o $@


clean:
	rm -rf ${target_path}/*.x
	rm snapshots/*.pgm
