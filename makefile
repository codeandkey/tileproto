CC = gcc
CFLAGS = -std=c99 -Wall -g -I/usr/include/libxml2
LDFLAGS = -ldl -lm -lglfw -lGL

OUTPUT = tileproto

SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)

all: $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $(OUTPUT)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS)
