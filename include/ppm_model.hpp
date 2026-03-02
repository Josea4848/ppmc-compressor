#pragma once
#include "../include/FrequencyTable.hpp"
#include <map>
#include <string>
#include <vector>
#define RO 256

class PpmModel {
private:
  std::map<std::string, std::vector<u_int32_t>> model;
  SimpleFrequencyTable equal_probs;
  std::string history;
  int order;

public:
  PpmModel(int order);
  void update(uint32_t symb);
  inline int getOrder() { return order; }
  inline std::string getHistory() { return history; }
  inline SimpleFrequencyTable *getInitialModelIt() { return &equal_probs; };
  inline std::vector<u_int32_t> *findModelIt(std::string subctx) {
    if (model.find(subctx) == model.end()) {
      return nullptr;
    }
    return &(model[subctx]);
  }
  inline std::map<std::string, std::vector<u_int32_t>> getModel() {
    return this->model;
  };
};