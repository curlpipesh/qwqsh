CC = clang++
CFLAGS = -Wall -pipe -lreadline --std=c++0x
TARGET = qwqsh

all: $(TARGET)

$(TARGET): clean
	# Curse you __FILE__
	cd src && $(CC) $(CFLAGS) -o ../$(TARGET) *.cpp

clean:
	rm -f *.a *.o *.la *.lo *.so *.so.* *.out $(TARGET)
