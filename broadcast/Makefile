# Variables
CXX = g++
CXXFLAGS = -Wall -std=c++17
SRC_DIR = src
EXEC_DIR = executables
SRCS = $(SRC_DIR)/server.cpp $(SRC_DIR)/client.cpp
OBJS = $(SRCS:.cpp=.o)
EXECUTABLES = $(EXEC_DIR)/server.exe $(EXEC_DIR)/client.exe

# Regla principal
all: $(EXECUTABLES)

# Regla para crear el ejecutable server.exe
$(EXEC_DIR)/server.exe: $(SRC_DIR)/server.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

# Regla para crear el ejecutable client.exe
$(EXEC_DIR)/client.exe: $(SRC_DIR)/client.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

# Regla para limpiar los archivos generados
clean:
	rm -f $(EXECUTABLES)

# Regla para ejecutar todo
run: all
	@echo "Ejecutables generados en la carpeta $(EXEC_DIR)"
