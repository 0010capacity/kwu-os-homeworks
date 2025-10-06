TARGET = hw1

CC = gcc

OBJS = testcase.o hw1.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o ${TARGET} ${OBJS}

testcase.o: testcase.c hw1.h
	$(CC) $(CFLAGS) -c testcase.c

hw1.o: hw1.c hw1.h queue.h
	$(CC) $(CFLAGS) -c hw1.c

clean:
	rm -f $(TARGET) $(OBJS)
