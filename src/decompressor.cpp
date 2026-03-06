// #include "../include/ppm_compress.hpp"
#include "../include/ArithmeticCoder.hpp"
#include "../include/BitIoStream.hpp"
#include "../include/FrequencyTable.hpp"
#include "../include/ppm_model.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>

#define WINDOW 1000

static void decompress(BitInputStream &in, std::ostream &out, int order);
uint32_t decodeModel(PpmModel &ppm_model, ArithmeticDecoder &decoder);
void hashToVector(const ankerl::unordered_dense::map<uint16_t, uint32_t> &freq,
                  std::vector<uint32_t> &buffer);
void setExclusion(const ankerl::unordered_dense::map<uint16_t, uint32_t> &freq,
                  std::vector<bool> &excluded_buffer);
std::vector<uint32_t> buffer(257, 0);
std::vector<bool> excluded_buffer(257, 1);

int main(int argc, char *argv[]) {
  // Início de medição
  auto start = std::chrono::high_resolution_clock::now();

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

static void decompress(BitInputStream &in, std::ostream &out, int order) {
  ArithmeticDecoder decoder(32, in);
  PpmModel ppm_model(order);
  std::uint64_t last_checkpoint = 0;
  int symbol_counter = 0;

  // Enquanto houver símbolos
  while (true) {
    uint16_t symbol = decodeModel(ppm_model, decoder);

    if (symbol == 256)
      break;

    symbol_counter++;
    if (symbol_counter % WINDOW == 0) {

        uint64_t currentBits = in.getBitCount();
        uint64_t windowBits = currentBits - last_checkpoint;

        double avg = (double) windowBits / WINDOW;

        std::cout << "Janela decode: "
                  << avg << " bits/símbolo\n";

        last_checkpoint = currentBits;
    }

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
  std::fill(excluded_buffer.begin(), excluded_buffer.end(), 1);

  // Percorre tabelas até k = 0
  for (int _order = history.size(); _order >= 0; _order--) {
    std::string_view subctx(history.data(), _order);

    auto model_frequences_it = ppm_model.findModelIt(std::string(subctx));

    // Verifica se o modelo k = _order existe
    if (model_frequences_it == ppm_model.getModel()->end()) {
      continue;
    }

    hashToVector(model_frequences_it->second, buffer);
    const uint32_t symbol = decoder.read(SimpleFrequencyTable(buffer));

    // Se símbolo estiver no modelo
    if (symbol < 256) {
      return symbol;
    }

    // Rô emitido, e iniciando exclusão
    setExclusion(model_frequences_it->second, excluded_buffer);
  }

  // Modelo de ignorância absoluta
  return decoder.read(*ppm_model.getInitialModelIt());
}

void hashToVector(const ankerl::unordered_dense::map<uint16_t, uint32_t> &freq,
                  std::vector<uint32_t> &buffer) {
  std::fill(buffer.begin(), buffer.end(), 0);

  for (const auto &[symbol, freq] : freq) {
    buffer[symbol] = freq * excluded_buffer[symbol];
  }
}

void setExclusion(const ankerl::unordered_dense::map<uint16_t, uint32_t> &freq,
                  std::vector<bool> &excluded_buffer) {

  for (const auto &[_symbol, freq] : freq) {
    if (_symbol != RO) {
      excluded_buffer[_symbol] = 0;
    }
  }
}
