# Nome do executável
TARGET = compressor

# Compilador
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude

# Diretórios
SRC_DIR = src
OBJ_DIR = build

# Arquivos fonte
SRCS = $(wildcard $(SRC_DIR)/*.cpp)

# Arquivos objeto
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

# Regra principal
all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Compilação
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rodar
run: $(TARGET)
	./$(TARGET)

# Limpar
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean run