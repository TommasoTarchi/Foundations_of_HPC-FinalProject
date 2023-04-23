gol: serial_gol parallel_gol

serial_gol: ${target_path}/serial_gol.x

${target_path}/serial_gol.x: src/serial_gol.c
	gcc -DTIME $^ -o $@

parallel_gol: ${target_path}/parallel_gol.x

${target_path}/parallel_gol.x: src/parallel_gol.c
	


clean: clean_exe clean_images

clean_exe:
	rm ${target_path}/*.x

clean_images:
	rm images/snapshots/*.pgm
	rm images/*.pgm
