# Makefile para compilar el cliente y el servidor UDP

# Variables
SRC_DIR = src
BUILD_DIR = build
CLIENT_SRC = $(SRC_DIR)/client.cpp
SERVER_SRC = $(SRC_DIR)/server.cpp
CLIENT_BIN = client
SERVER_BIN = server

# Compilador
CXX = g++
CXXFLAGS = -std=c++11 -Wall

# Objetivos por defecto
all: $(CLIENT_BIN) $(SERVER_BIN)

# Regla para compilar el cliente
$(CLIENT_BIN): $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Regla para compilar el servidor
$(SERVER_BIN): $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Limpiar archivos generados
clean:
	rm -f $(CLIENT_BIN) $(SERVER_BIN)

.PHONY: all clean
