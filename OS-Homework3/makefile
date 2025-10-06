TARGET = hw3

CC = gcc
CFLAGS = -Wall -O2

OBJS = testcase.o buf.o disk.o buf_extension.o fs.o fs_internal.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)

testcase.o: testcase.c buf.h disk.h
	$(CC) $(CFLAGS) -c testcase.c

buf.o: buf.c buf.h queue.h disk.h buf_extension.h
	$(CC) $(CFLAGS) -c buf.c

buf_extension.o: buf_extension.c buf.h buf_extension.h
	$(CC) $(CFLAGS) -c buf_extension.c

fs.o: fs.c buf.h disk.h fs.h
	$(CC) $(CFLAGS) -c fs.c

fs_internal.o: fs_internal.c buf.h disk.h fs.h
	$(CC) $(CFLAGS) -c fs_internal.c

disk.o: disk.c disk.h
	$(CC) $(CFLAGS) -c disk.c

clean:
	rm -f $(TARGET) $(OBJS)
