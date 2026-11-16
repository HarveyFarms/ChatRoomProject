
CXX = g++

CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic

QT_CFLAGS = $(shell pkg-config --cflags Qt6Widgets Qt6Network)
QT_LIBS = $(shell pkg-config --libs Qt6Widgets Qt6Network)

ARGON2_LIBS = -largon2

AUTH_SOURCES = Authentication.cpp User.cpp PasswordHasher.cpp

SERVER_SOURCES = server.cpp Channel.cpp $(AUTH_SOURCES)

CLIENT_SOURCES = client.cpp

.PHONY: all clean run-server run-client test

all: chat_server chat_client

chat_server: $(SERVER_SOURCES)
	$(CXX) $(CXXFLAGS) $(QT_CFLAGS) $^ $(QT_LIBS) $(ARGON2_LIBS) -o $@

chat_client: $(CLIENT_SOURCES)
	$(CXX) $(CXXFLAGS) $(QT_CFLAGS) $^ $(QT_LIBS) -o $@

run-server: chat_server
	./chat_server

run-client: chat_client
	./chat_client

clean:
	rm -f chat_server chat_client authentication_test
