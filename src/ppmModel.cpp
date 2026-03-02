#include "../include/ppm_model.hpp"
#include <iostream>

PpmModel::PpmModel(int order) : equal_probs(std::vector<uint32_t>(257, 1)) {
  this->order = order;
  this->history = "";
}

void PpmModel::update(uint32_t symb) {
  // std::cout << "============================\n";
  // std::cout << "Simbolo:" << (char)symb << std::endl;
  // std::cout << "Ctx: " << this->history << std::endl;

  // ATUALIZA K = - 1
  if (equal_probs.get(symb)) {
    equal_probs.set(symb, 0);
    // std::cout << "Atualiza modelo k = -1\n";

    for (uint32_t symbol = 0; symbol < equal_probs.getSymbolLimit(); ++symbol) {
      // std::cout << "Símbolo " << (char)symbol << " = "
      // << equal_probs.get(symbol) << " ";
    }
    // std::cout << "\n";
  }

  // Atualiza k >= 0
  for (int _order = 0; _order <= this->history.size(); _order++) {
    const std::string subctx = this->history.substr(0, _order);

    // Verifica se subcontexto não existe
    if (this->model.find(subctx) == this->model.end()) {

      // Cria tabela de frequências
      this->model[subctx] = std::vector<uint32_t>(257, 0);
      this->model[subctx][symb] = 1;
      this->model[subctx][RO] = 1;
    }
    // Se existir para o contexto, é incrementado
    else if (this->model[subctx][symb]) {
      this->model[subctx][symb]++;
    }
    // Se existir e não tiver o símbolo
    else {
      this->model[subctx][symb]++;
      this->model[subctx][RO]++;
    }

    // std::cout << "Modelo k = " << _order << " atualizado\n";
    // for (int i = 0; i < this->model[subctx].size(); i++) {
    //   std::cout << "S: " << char(i) << " Freq: " << this->model[subctx][i]
    //             << " | ";
    // }

    // std::cout << std::endl;
  }

  // std::cout << "Modelos atualizados\n";

  // std::cout << "contexto: " << this->history << std::endl;

  // Atualiza contexto
  if (this->history.size() == this->order) {
    this->history.pop_back(); // remove último
  }

  this->history.insert(this->history.begin(), symb); // insere no início

  // std::cout << "Atualização completa finalizada\n";
}
