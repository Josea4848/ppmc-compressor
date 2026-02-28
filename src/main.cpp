#include "../include/reader.hpp"
#include <iostream>
#include <iomanip>

int main(int argc, char *argv[]) {
  if (argc < 2) {
      std::cout << "Invalid argument (file)\n";
      return -1;
    }  
  
    Reader reader(argv[1]);  
    reader.streamTo([&](const uint8_t* data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        std::cout << static_cast<char>(data[i]);
    }
});

    std::cout << "\n\nFim da leitura.\n";
    return 0;
}