#include "../include/ppm_compress.hpp"

void compress(std::iostream in, BitOutputStream &out) {
  // Tabela de frequências
  std::vector<uint32_t> freqs(257, 0);

  freqs['a'] = 3;
  freqs['s'] = 4;
  freqs['i'] = 1;
  freqs['n'] = 1;
  freqs['r'] = 1;
  freqs[256] = 1;

  SimpleFrequencyTable table(freqs);
  ArithmeticEncoder encoder(32, out);

  while (true) {
    int byte = in.get();
    if (byte == EOF)
      break;
    encoder.write(table, byte);
  };

  // Adiciona EOF
  encoder.write(table, 256);
  encoder.finish();
}