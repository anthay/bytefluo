# makefile for bytefluo_test
# (bytefluo is a header-only library - there is no separate lib to be built)

TARGET = bytefluo_test
LIBS = 
CC = clang++
CFLAGS = -std=c++11 -stdlib=libc++ -O3 -Wall
SRC_DIR  = ../..
TEST_DIR = ../..
INCLUDES = -I$(SRC_DIR)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@



all: $(TARGET)

test: $(TARGET)
	./$(TARGET)

clean:
	rm $(TARGET)


$(TARGET): $(SRC_DIR)/bytefluo_test.cpp $(SRC_DIR)/bytefluo.h
	$(CC) $(CFLAGS) $(INCLUDES) $(SRC_DIR)/bytefluo_test.cpp -o $@

