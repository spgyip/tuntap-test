
all: main

main: main.c
	gcc $^ -o $@

clean:
	rm -fv main
