#include "../include/ppm_model.hpp"
#include <iostream>

PpmModel::PpmModel(int &order) {
  this->order = order;
  this->history = "";
  this->equal_probs = new SimpleFrequencyTable(std::vector<uint32_t>(257, 1));
}

void PpmModel::update(const uint32_t &symb) {
  std::cout << "==== Atualização do modelo ====" << std::endl;

  // ATUALIZA K = - 1
  if (equal_probs->get(symb)) {
    equal_probs->set(symb, 0);
    std::cout << "(k = - 1) modelo k = -1 atualizado\n";
  }

  // Atualiza k >= 0
  for (int _order = 0; _order <= this->history.size(); _order++) {
    const std::string subctx = this->history.substr(0, _order);
    const auto model_it = this->model.find(subctx);

    std::cout << "k = " << _order << " Subcontexto: " << subctx
              << " Atualizando modelo k = " << _order << std::endl;

    // Verifica se subcontexto não existe
    if (model_it == this->model.end()) {
      std::cout << "k = " << _order << "Modelo não existe, criando novo\n";
      // Cria tabela de frequências
      this->model[subctx] = {{symb, 1}, {RO, 1}};
      std::cout << "k = " << _order << "Feito.\n";
      continue;
    }
    const auto model_symb_it = std::find_if(
        model_it->second.begin(), model_it->second.end(),
        [&](const contextItem &item) { return item.symb == symb; });

    // Se existir para o contexto, é incrementado
    if (model_symb_it != model_it->second.end()) {
      std::cout << "k = " << _order << "Modelo existe, apenas incrementado.\n";
      model_symb_it->freq++;
    }
    // Se existir e não tiver o símbolo
    else {
      std::cout << "k = " << _order
                << "Modelo existe, mas o símbolo não está.\n";
      model_it->second.push_front({symb, 1});
      model_it->second.back().freq++;
    }
  }

  // Atualiza contexto
  if (this->history.size() == this->order) {
    this->history.pop_back();
  }

  this->history.insert(this->history.begin(), symb);
}

std::vector<uint32_t> createFrequencyTable(std::list<contextItem> *freqs) {
  std::vector<uint32_t> _freqs(257, 0);

  for (auto &item : *freqs) {
    _freqs[item.symb] = item.freq;
  }

  for (int i = 0; i < _freqs.size(); i++) {
    std::cout << (char)i << " : " << _freqs[i] << "  ";
  }

  std::cout << "\n";

  return _freqs;
}