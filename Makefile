SRC := #
SRC += driver.cc
SRC += inifile.cc
SRC += saicanon.cc
SRC += tool_parse.cc

OBJ = $(SRC:%.cc=%.o)

CC = g++

chong: $(OBJ) ./librs274.so
	@$(CC)  -o $@ $^ -Os -Wall -g -I. -L./ -Wl,-rpath,./ -lrs274

librs274.so:
	@make -C rs274ngc

$(OBJ):%.o:%.cc
	@$(CC) -c -o $@ $< -I./include -I. -I./rs274ngc

.PHONY:clean
clean:
	@rm *.o librs274.so chong -rf
	@make clean -C rs274ngc
