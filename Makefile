
all: xtun

xtun: xtun.c
	gcc -Wall -std=gnu99 -lpthread $^ -o $@

clean:
	rm -fv xtun
