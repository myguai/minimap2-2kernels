# Remember to recompile the lib everytime you changed the rtl
# aocl -march=emulator library hdl-comp-pkg compute_sc.xml -o rtl_lib.aoco 
# aocl library create -name rtl_lib rtl_lib.aoco
# aoc -march=emulator compute_sc.xml -o rtl_lib.aoco 
# aocl library create -name rtl_lib rtl_lib.aoco
lib:
	aocl library hdl-comp-pkg compute_sc.xml -o rtl_lib.aoco 
	aocl library create -name rtl_lib rtl_lib.aoco
clean:
	rm -rf *.aoco *.aoclib
