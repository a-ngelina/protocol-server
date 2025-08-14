CXX = g++
CXXFLAGS = -std=c++20

SOURCES = string_utilities.cpp request_parsing_response_preparation.cpp network.cpp file_manipulation.cpp server_main.cpp

OBJECTS = $(SOURCES:.cpp=.o)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

server: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o server

run: server
	./server

tests/client_functions.o: tests/client_functions.cpp
	$(CXX) $(CXXFLAGS) -c tests/client_functions.cpp -o tests/client_functions.o

tests/client_status.o: tests/client_status.cpp
	$(CXX) $(CXXFLAGS) -c tests/client_status.cpp -o tests/client_status.o

test-client: tests/client_functions.o tests/client_status.o
	$(CXX) $(CXXFLAGS) tests/client_functions.o tests/client_status.o -o test-client

test: server test-client
	@./server & SERVER_PID=$$!; \
	sleep 2; \
	./test-client; \
	kill $$SERVER_PID 2>/dev/null || true; \
	wait $$SERVER_PID 2>/dev/null || true

clean:
	rm -f server test-client *.o tests/*.o

.PHONY: run clean test
