CC = gcc
CFLAGS = `pkg-config --cflags gtk+-3.0`
LIBS = `pkg-config --libs gtk+-3.0`

all: blueteth

blueteeth: ./src/main.c
	$(CC) ./src/main.c -o ./bin/blueteeth $(CFLAGS) $(LIBS)

clean:
	rm -f ./bin/blueteeth

run: blueteeth
	./bin/blueteeth

design:
	xdg-open ./assets/main.glade
	gedit ./src/main.c &

.PHONY: all clean run design
