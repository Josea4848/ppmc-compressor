// #include "../include/ppm_compress.hpp"
#include "../include/ArithmeticCoder.hpp"
#include "../include/BitIoStream.hpp"
#include "../include/FrequencyTable.hpp"
#include "../include/ppm_model.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <thread>

static void decompress(BitInputStream &in, std::ostream &out, int order);
uint32_t decodeModel(PpmModel &ppm_model, ArithmeticDecoder &decoder);

int main(int argc, char *argv[]) {

  if (argc < 2) {
    std::cerr << "Usage: ./decompress file.ppm"
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
    return EXIT_FAILURE;
    std::cerr << "Erro ao abrir arquivo\n";
  }
  BitInputStream bin(in);

  // Configuração de stream de saída
  std::string output_file = input_file;
  size_t pos = output_file.find_last_of('.');
  if (pos != std::string::npos) {
    output_file.erase(pos);
  }

  std::ofstream out(output_file, std::ios::binary);
  if (!out) {
    std::cerr << "Erro ao criar arquivo de saída\n";
    return 1;
  }

  try {
    decompress(bin, out, order);
    return EXIT_SUCCESS;
  }

  catch (const char *msg) {
    std::cerr << msg << std::endl;
    return EXIT_FAILURE;
  }
}

static void decompress(BitInputStream &in, std::ostream &out, int order) {
  ArithmeticDecoder decoder(32, in);
  PpmModel ppm_model(order);

  // Enquanto houver símbolos
  while (true) {
    uint32_t symbol = decodeModel(ppm_model, decoder);
    std::cout << "Símbolo decodificado: " << (char)symbol
              << " código: " << symbol << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    if (symbol == 256)
      break;

    // Atualiza modelo
    ppm_model.update(symbol);
    int b = static_cast<int>(symbol);
    if (std::numeric_limits<char>::is_signed)
      b -= (b >> 7) << 8;
    out.put(static_cast<char>(b));
  };
}

uint32_t decodeModel(PpmModel &ppm_model, ArithmeticDecoder &decoder) {
  const std::string history = ppm_model.getHistory();

  // Percorre tabelas até k = 0
  for (int _order = history.size(); _order >= 0; _order--) {
    const std::string subctx = history.substr(0, _order);

    const auto model_frequencies = ppm_model.findModelIt(subctx);

    // Verifica se o modelo k = _order existe
    if (!model_frequencies) {
      continue;
    }

    const uint32_t symbol = decoder.read(
        SimpleFrequencyTable(createFrequencyTable(model_frequencies)));

    // Se símbolo estiver no modelo
    if (symbol < 256) {
      return symbol;
    }
  }

  // Modelo de ignorância absoluta
  return decoder.read(*ppm_model.getInitialModelIt());
}
