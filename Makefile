# Executable Name
TARGET = MartinezK_clienteFTP

# Source Files
SRC = MartinezK_clienteFTP.c connectTCP.c connectsock.c errexit.c

# Compiler and Flags
CC = gcc

# Default Rule
all: $(TARGET)

# How to compilate the executable
$(TARGET): $(SRC)
	$(CC) -o $(TARGET) $(SRC)

# Generated files cleaning
clean:
	rm -f $(TARGET)

