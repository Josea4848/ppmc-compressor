// #include "../include/ppm_compress.hpp"
#include "../include/ArithmeticCoder.hpp"
#include "../include/BitIoStream.hpp"
#include "../include/FrequencyTable.hpp"
#include "../include/ppm_model.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>

static void compress(std::ifstream &in, BitOutputStream &out, int order);
static void encodeModel(PpmModel &ppm_model, ArithmeticEncoder &encoder,
                        uint32_t symbol);

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
  const std::string ppm_ext = ".ppmc";
  const std::string output_file = argv[1] + ppm_ext;
  std::ofstream out(output_file, std::ios::binary);
  BitOutputStream bout(out);

  try {
    compress(in, bout, order);
    bout.finish();
    return EXIT_SUCCESS;
  }

  catch (const char *msg) {
    std::cerr << msg << std::endl;
    return EXIT_FAILURE;
  }
}

static void compress(std::ifstream &in, BitOutputStream &out, int order) {
  ArithmeticEncoder encoder(32, out);
  PpmModel ppm_model(order);

  // Enquanto houver símbolos
  while (true) {
    uint32_t symbol = in.get();
    if (symbol == std::char_traits<char>::eof())
      break;

    // Codifica com modelo adequado
    std::cout << "-------------------------------\n";
    std::cout << "| Início de codificação\n";
    encodeModel(ppm_model, encoder, symbol);
    std::cout << "| Símbolo codificado\n";

    // Atualiza modelo
    ppm_model.update(symbol);
    std::cout << "-------------------------------\n";
  };

  // Adiciona EOF
  encodeModel(ppm_model, encoder, 256);
  encoder.finish();
}

static void encodeModel(PpmModel &ppm_model, ArithmeticEncoder &encoder,
                        uint32_t symbol) {
  const std::string history = ppm_model.getHistory();
  std::cout << "Codificando símbolo: " << symbol << " (char): " << char(symbol)
            << std::endl;

  // Percorre tabelas até k = 0
  for (int _order = history.size(); _order >= 0; _order--) {

    const std::string subctx = history.substr(0, _order);

    std::cout << "Tentando modelo k = " << _order
              << " - Subcontexto: " << subctx << std::endl;

    auto model_it = ppm_model.findModelIt(subctx);

    // Verifica se o modelo k = _order existe
    if (!model_it) {
      std::cout << "Modelo k = " << _order << std::endl;
      continue;
    }

    // Se símbolo estiver no modelo
    if (std::find_if(model_it->begin(), model_it->end(),
                     [&](const contextItem &item) {
                       return item.symb == symbol;
                     }) != model_it->end() &&
        symbol != RO) {
      std::cout << "Codificado com o modelo k = " << _order << std::endl;
      encoder.write(SimpleFrequencyTable(createFrequencyTable(model_it)),
                    symbol);
      return;
    }
    // Codificar com rô
    else {
      std::cout << "Rô emitido no modelo k =" << _order << std::endl;
      encoder.write(SimpleFrequencyTable(createFrequencyTable(model_it)), RO);
    }
  }

  // Modelo de ignorância absoluta
  std::cout << "Codificado com modelo k = -1\n";
  encoder.write(*ppm_model.getInitialModelIt(), symbol);
}
