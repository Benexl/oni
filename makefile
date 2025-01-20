CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = -lcurl -lxml2 -lcjson

SRC = src/oni.c src/scrappers/allanime.c src/utils/networking.c
OBJ = $(SRC:.c=.o)
TARGET = bin/oni

$(TARGET): | bin

bin:
	mkdir -p bin

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
