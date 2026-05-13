LIBS= -lpthread -lm #Libraries used if needed
SRCS= IOTClient.c 
BIN=IOTClient_bin
CFLAGS+= -g -O3
OBJS=$(subst .cpp,.o,$(SRCS))
all : $(BIN)
$(BIN): $(OBJS)	
	@echo [link] $@
	$(CXX) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)
%.o: %.cpp
	@echo [Compile] $<
	$(CXX) -c $(CFLAGS) $< -o $@
clean:
	@rm -f $(OBJS) $(BIN)
