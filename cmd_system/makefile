CC := g++
SRCS := *.cpp ../network/tcp_socket.cpp ../util/logger.cpp
LIBS :=  -lpthread 
TARGET := -o cmd
object: $(SRCS)
	$(CC) $(TARGET) $(SRCS) $(LIBS)
	
.PHONY: clean
clean:
	-rm cmd 