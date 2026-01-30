CC ?= $(TARGET_CC)
CFLAGS += -Wall -Wextra -O2

TARGET = demo_app

all: $(TARGET)

$(TARGET): demo_app.c
	$(CC) $(CFLAGS) demo_app.c -o $(TARGET)

clean:
	rm -f $(TARGET)
