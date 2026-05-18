LIBS= -lpthread -lm #Libraries used if needed
SRCS= IOTClient.c 
BIN=IOTClient_bin
CFLAGS+= -g -O3
OBJS=$(SRCS:.c=.o)
all : $(BIN)
$(BIN): $(OBJS)	
	@echo [link] $@
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)
%.o: %.c
	@echo [Compile] $<
	$(CC) -c $(CFLAGS) $< -o $@
clean:
	@rm -f $(OBJS) $(BIN)
