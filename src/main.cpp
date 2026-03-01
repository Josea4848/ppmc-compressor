#include "../include/ppm_compress.hpp"
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: ./compress model_order"
                 "model_order\n";
    return 1;
  }

  // Ordem do modelo
  int order = 3;
  if (argc > 2) {
    order = std::stoi(argv[2]);
  }

  // Leitura de arquivo de entrada
  const std::string input_file = argv[1];
  std::ifstream in(input_file, std::ios::binary);
  if (!in) {
    std::cerr << "Erro ao abrir arquivo\n";
  }

  // Configuração de stream de saída
  const std::string ppm_ext = ".ppm";
  const std::string output_file = argv[1] + ppm_ext;
  std::ofstream out(output_file, std::ios::binary);
  BitOutputStream bout(out);

  try {

    bout.finish();
    return EXIT_SUCCESS;
  }

  catch (const char *msg) {
    std::cerr << msg << std::endl;
    return EXIT_FAILURE;
  }
}
