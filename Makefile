CC = gcc
CFLAGS = -Wall -pthread -g
LDFLAGS = -lglut -lGLU -lGL -lm -pthread

TARGET = ape_simulation
SOURCES = main.c config.c maze.c apes.c graphics.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
