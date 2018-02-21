CFLAGS:= -Wall -Werror -lpthread -pthread -g

all: chat client

client:
	gcc $(CFLAGS) $@.c -o $@

chat:
	gcc $(CFLAGS) $@.c -o $@

clean:
	rm -f client
	rm -f chat
