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
    (*this->model)[subctx][symb]++;
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
  this->model->rehash(0);
  
  delete this->equal_probs;
  this->equal_probs = new SimpleFrequencyTable(std::vector<uint32_t>(257, 1));
}