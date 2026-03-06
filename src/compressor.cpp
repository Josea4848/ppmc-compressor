// #include "../include/ppm_compress.hpp"
#include "../include/ArithmeticCoder.hpp"
#include "../include/BitIoStream.hpp"
#include "../include/FrequencyTable.hpp"
#include "../include/ppm_model.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

#define WINDOW 1000

static void compress(std::ifstream &in, BitOutputStream &out, int order);
static void encodeModel(PpmModel &ppm_model, ArithmeticEncoder &encoder,
                        uint16_t symbol);
std::vector<bool> excluded_buffer(257, 1);
void hashToVector(const ankerl::unordered_dense::map<uint16_t, uint32_t> &freq,
                  std::vector<uint32_t> &buffer, const uint16_t &symbol,
                  const bool set_exclusion);
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
  const std::string ppm_ext = ".ppmc";
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
  std::uint64_t symbol_count = 0;
  std::uint64_t checkpoint_1 = 0;
  std::uint64_t checkpoint_2 = 0;

  // Enquanto houver símbolos
  while (true) {
    uint32_t symbol = in.get();

    // std::cout << "Codificação do símbolo: " << (char)symbol
    //           << " cód: " << symbol << std::endl;

    if (symbol == std::char_traits<char>::eof())
      break;

    // Codifica com modelo adequado
    encodeModel(ppm_model, encoder, symbol);

    symbol_count++;
    if (symbol_count % WINDOW == 0) {

        uint64_t currentBits = out.getBitCount();
        uint64_t current_window = currentBits - checkpoint_1;
        uint64_t old_window = checkpoint_1 - checkpoint_2;

        double current_avg = (double) current_window / WINDOW;
        double old_avg = (double) old_window / WINDOW;

        double perc_diff = 0;
        if(old_avg > 0) {
          perc_diff = (current_avg - old_avg) / old_avg;
        }
        std::cout << "Janela: " << current_avg << " bits/símbolo\n";
        
        if(perc_diff > 0.5) {
          ppm_model.reset();
          std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        checkpoint_2 = checkpoint_1;
        checkpoint_1 = currentBits;
    }

    // Atualiza modelo
    ppm_model.update(symbol);
  };

  // Adiciona EOF
  encodeModel(ppm_model, encoder, 256);
  encoder.finish();
}

static void encodeModel(PpmModel &ppm_model, ArithmeticEncoder &encoder,
                        uint16_t symbol) {
  std::fill(excluded_buffer.begin(), excluded_buffer.end(), 1);

  const std::string history = ppm_model.getHistory();
  // Percorre tabelas até k = 0
  for (int _order = static_cast<int>(history.size()); _order >= 0; _order--) {
    std::string_view subctx(history.data(), _order);

    auto model_frequences_it = ppm_model.findModelIt(std::string(subctx));

    // Verifica se o modelo k = _order existe
    if (model_frequences_it == ppm_model.getModel()->end()) {
      continue;
    }

    // Se símbolo estiver no modelo
    if (model_frequences_it->second.find(symbol) !=
            model_frequences_it->second.end() &&
        symbol != RO) {
      hashToVector(model_frequences_it->second, buffer, symbol, false);
      encoder.write(SimpleFrequencyTable(buffer), symbol);
      return;
    }
    // Codificar com rô
    else {
      hashToVector(model_frequences_it->second, buffer, RO, true);
      encoder.write(SimpleFrequencyTable(buffer), RO);
    }
  }

  //  Modelo de ignorância absoluta
  encoder.write(*ppm_model.getInitialModelIt(), symbol);
}

void hashToVector(const ankerl::unordered_dense::map<uint16_t, uint32_t> &freq,
                  std::vector<uint32_t> &buffer, const uint16_t &symbol,
                  const bool set_exclusion) {
  std::fill(buffer.begin(), buffer.end(), 0);

  for (const auto &[_symbol, freq] : freq) {
    buffer[_symbol] = freq * excluded_buffer[_symbol];

    // Se o símbolo (que não o a ser codificado) estiver na tabela e a exclusão
    // for habilitada
    if (_symbol != RO && set_exclusion) {
      excluded_buffer[_symbol] = 0;
    }
  }
}
