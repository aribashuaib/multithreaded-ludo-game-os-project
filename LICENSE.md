
Educational Use - Operating Systems Project

### **2. Makefile**
```makefile
# Makefile for Ludo Game OS Project

CC = g++
CFLAGS = -Wall -pthread -std=c++11
TARGET = ludo_game
SOURCES = main.cpp
OBJS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS += -g
debug: clean $(TARGET)

.PHONY: all clean run debug
