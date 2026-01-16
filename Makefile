CC = gcc
CFLAGS = `pkg-config --cflags gtk+-3.0`
LIBS = `pkg-config --libs gtk+-3.0`

all: blueteeth

blueteeth: ./src/main.c
	$(CC) ./src/main.c -o ./bin/blueteeth $(CFLAGS) $(LIBS)

gtk: ./src/gtk.c
	$(CC) ./src/gtk.c -o ./bin/gtk $(CFLAGS) $(LIBS)

clean:
	rm -f ./bin/blueteeth

run: blueteeth
	./bin/blueteeth

rungtk: gtk
	./bin/gtk

design:
	xdg-open ./assets/main.glade
	gedit ./src/main.c &

.PHONY: all gtk clean run rungtk design
