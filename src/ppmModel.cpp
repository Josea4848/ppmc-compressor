#include "../include/ppm_model.hpp"
#include <iostream>

PpmModel::PpmModel(int &order) {
  this->order = order;
  this->history = "";
  this->equal_probs = new SimpleFrequencyTable(std::vector<uint32_t>(257, 1));
  this->model = new Model();
}

void PpmModel::update(const uint16_t &symb) {
  // ATUALIZA K = - 1
  if (equal_probs->get(symb)) {
    equal_probs->set(symb, 0);
  }

  // Atualiza k >= 0
  for (int _order = 0; _order <= this->history.size(); _order++) {
    const std::string subctx = this->history.substr(0, _order);

    // Verifica se subcontexto não existe
    if (this->model->find(subctx) == this->model->end()) {
      // Cria tabela de frequências
      (*this->model)[subctx][symb] = 1;
      (*this->model)[subctx][RO] = 1;

    }
    // Se existir para o contexto, é incrementado
    else if ((*this->model)[subctx].find(symb) !=
             (*this->model)[subctx].end()) {
      (*this->model)[subctx][symb]++;
    }
    // Se existir e não tiver o símbolo
    else {
      (*this->model)[subctx][symb] = 1;
      (*this->model)[subctx][RO]++;
    }
  }

  // Atualiza contexto
  if (static_cast<int>(this->history.size()) == this->order && this->order) {
    this->history.pop_back();
  }
  if (this->order) {
    this->history.insert(this->history.begin(), symb);
  }
}
void PpmModel::printModel(const std::string &subctx) {

  for (const auto &[symbol, freq] : (*this->model)[subctx]) {
    std::cout << "  Símbolo: " << symbol << " Frequência: " << freq << "\n";
  }
}

void PpmModel::reset() {
  this->model->clear();
}