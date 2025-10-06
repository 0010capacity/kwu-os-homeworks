TARGET = hw2

CC = gcc
CFLAGS = -Wall -O2

OBJS = testcase.o buf.o disk.o buf_extension.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)

testcase.o: testcase.c buf.h disk.h
	$(CC) $(CFLAGS) -c testcase.c

buf.o: buf.c buf.h queue.h disk.h buf_extension.h
	$(CC) $(CFLAGS) -c buf.c

buf_extension.o: buf_extension.c buf.h buf_extension.h
	$(CC) $(CFLAGS) -c buf_extension.c

disk.o: disk.c disk.h
	$(CC) $(CFLAGS) -c disk.c

clean:
	rm -f $(TARGET) $(OBJS)
