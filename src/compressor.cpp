// #include "../include/ppm_compress.hpp"
#include "../include/ArithmeticCoder.hpp"
#include "../include/BitIoStream.hpp"
#include "../include/FrequencyTable.hpp"
#include "../include/ppm_model.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <unordered_map>

static void compress(std::ifstream &in, BitOutputStream &out, int order);
static void encodeModel(PpmModel &ppm_model, ArithmeticEncoder &encoder,
                        uint16_t symbol);
void hashToVector(const ankerl::unordered_dense::map<uint16_t, uint32_t> &freq,
                  std::vector<uint32_t> &buffer);
;
std::vector<uint32_t> buffer(257, 0);
// SimpleFrequencyTable frequencyTable;

int main(int argc, char *argv[]) {
  // Início de medição
  auto start = std::chrono::high_resolution_clock::now();

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
    compress(in, bout, order);
    bout.finish();

    // Fim de execução
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration<double>(end - start);
    std::cout << "Tempo de execução: " << duration.count() << " s\n";
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
    encodeModel(ppm_model, encoder, symbol);

    // Atualiza modelo
    ppm_model.update(symbol);
  };

  // Adiciona EOF
  encodeModel(ppm_model, encoder, 256);
  encoder.finish();
}

static void encodeModel(PpmModel &ppm_model, ArithmeticEncoder &encoder,
                        uint16_t symbol) {

  const std::string history = ppm_model.getHistory();
  // Percorre tabelas até k = 0
  for (int _order = history.size(); _order >= 0; _order--) {
    std::string_view subctx(history.data(), _order);

    auto model_frequences_it = ppm_model.findModelIt(std::string(subctx));

    // Verifica se o modelo k = _order existe
    if (model_frequences_it == ppm_model.getModel()->end()) {
      continue;
    }

    hashToVector(model_frequences_it.value(), buffer);

    // Se símbolo estiver no modelo
    if (model_frequences_it.value().find(symbol) !=
            model_frequences_it.value().end() &&
        symbol != 256) {
      encoder.write(SimpleFrequencyTable(buffer), symbol);
      return;
    }
    // Codificar com rô
    else {
      encoder.write(SimpleFrequencyTable(buffer), RO);
    }
  }

  // Modelo de ignorância absoluta
  encoder.write(*ppm_model.getInitialModelIt(), symbol);
}

void hashToVector(const ankerl::unordered_dense::map<uint16_t, uint32_t> &freq,
                  std::vector<uint32_t> &buffer) {
  std::fill(buffer.begin(), buffer.end(), 0);

  for (const auto &[symbol, freq] : freq) {
    buffer[symbol] = freq;
  }
}
