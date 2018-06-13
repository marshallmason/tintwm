OPTIONS = -O3 -Wall -Werror -pedantic

tintwm: tintwm.o keys.o layouts.o
	$(CC) $(OPTIONS) -o $@ $^ -lxcb -lxcb-keysyms

tintwm.o: tintwm.c tintwm.h keys.h layouts.h
	$(CC) $(OPTIONS) -c $<

keys.o: keys.c tintwm.h keys.h layouts.h
	$(CC) $(OPTIONS) -c $<

layouts.o: layouts.c tintwm.h layouts.h
	$(CC) $(OPTIONS) -c $<

clean:
	rm -f tintwm *.o

.PHONY: clean
