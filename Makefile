
all: tun-client

tun-client: tun-client.c
	gcc -std=c99 -lpthread $^ -o $@

clean:
	rm -fv tun-client
