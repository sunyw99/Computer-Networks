CC = gcc

CFLAGS  = -g -Wall -std=c99 -D_GNU_SOURCE

# the build target executable:
RECEIVER = receiver
SENDER = sender

all: $(RECEIVER) $(SENDER)

# assume the C code for TARGET is TARGET.c
$(RECEIVER): $(RECEIVER).c
	$(CC) $(CFLAGS) -o $(RECEIVER) $(RECEIVER).c

$(SENDER): $(SENDER).c
	$(CC) $(CFLAGS) -o $(SENDER) $(SENDER).c

clean:
	$(RM) $(RECEIVER) $(SENDER)
