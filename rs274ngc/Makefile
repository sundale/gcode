SRC := #
SRC += interp_arc.cc
SRC += interp_array.cc
SRC += interp_check.cc
SRC += interp_convert.cc
SRC += interp_queue.cc
SRC += interp_cycles.cc
SRC += interp_execute.cc
SRC += interp_find.cc
SRC += interp_internal.cc
SRC += interp_inverse.cc
SRC += interp_read.cc
SRC += interp_write.cc
SRC += interp_o_word.cc
SRC += nurbs_additional_functions.cc
SRC += rs274ngc_pre.cc

OBJ = $(SRC:%.cc=%.o)

CC = g++

librs274.so:$(OBJ)
	@$(CC) -shared -fPIC -o $@ $^ -Os -Wall -g -Wl,-soname,$(notdir $@)
	@cp $@ ..

$(OBJ):%.o:%.cc
	@$(CC) -c -o $@ $< -fPIC -I.  -I../include

.PHONY:clean
clean:
	@rm *.o librs274.so -rf
