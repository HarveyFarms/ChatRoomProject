CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic
QT_CFLAGS = $(shell pkg-config --cflags Qt6Widgets Qt6Network)
QT_LIBS = $(shell pkg-config --libs Qt6Widgets Qt6Network)
ARGON2_LIBS = -largon2

AUTH_SOURCES = Authentication.cpp User.cpp PasswordHasher.cpp

.PHONY: all clean run-server run-client test

all: chat_server chat_client

chat_server: server.cpp $(AUTH_SOURCES)
	$(CXX) $(CXXFLAGS) $(QT_CFLAGS) $^ $(QT_LIBS) $(ARGON2_LIBS) -o $@

chat_client: client.cpp
	$(CXX) $(CXXFLAGS) $(QT_CFLAGS) $< $(QT_LIBS) -o $@

clean:
	rm -f chat_server chat_client authentication_test
