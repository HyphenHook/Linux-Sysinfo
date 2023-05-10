CC = gcc
CK = -Werror -Wall
.PHONY: ALL
ALL: stats_functions.o  main
.PHONY: clean
clean: 
	rm -rf *.o
	rm -rf main
stats_functions.o: stats_functions.c stats_functions.h
	$(CC) $(CK) -c $< -o $@
main: driver.c stats_functions.o
	$(CC) $(CK) -o $@ $^