# Nome do executável (padrão)
TARGET ?= compressor

# Compilador
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -Iinclude
CXXFLAGS += -O3


# Diretórios
SRC_DIR = src
OBJ_DIR = build

# Arquivo principal (main)
MAIN_SRC = $(SRC_DIR)/$(TARGET).cpp

# Todos os outros .cpp (sem o main)
SRCS = $(filter-out $(SRC_DIR)/compressor.cpp $(SRC_DIR)/decompressor.cpp, $(wildcard $(SRC_DIR)/*.cpp))

# Objetos
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
MAIN_OBJ = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(MAIN_SRC))

# Regra principal
all: $(TARGET)

# Link
$(TARGET): $(OBJS) $(MAIN_OBJ)
	$(CXX) $^ -o $@ $(LDFLAGS)

# Compilação
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rodar
run: $(TARGET)
	./$(TARGET)

# Limpar
clean:
	rm -rf $(OBJ_DIR) compressor decompressor

.PHONY: all clean run