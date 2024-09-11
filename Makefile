CC = gcc
CFLAGS = -g
TARGET = chatting
OBJS = main.o Client.o Server.o
SRCS = $(SRCS:.o=.c)
HEADERS = Client.h Server.h Macro.h

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

