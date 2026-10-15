CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c11 -pedantic
CPPFLAGS ?= -Iinclude
LDLIBS ?= -lm
TARGET ?= blang
SOURCES := src/main.c
.PHONY: all clean test
all: $(TARGET)
$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(SOURCES) $(LDLIBS)
test: $(TARGET)
	./$(TARGET) tests/hello.bs
clean:
	$(RM) $(TARGET) $(TARGET).exe
