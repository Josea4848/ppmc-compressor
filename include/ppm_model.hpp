#pragma once
#include "../external/unordered_dense/include/ankerl/unordered_dense.h"
#include "../include/FrequencyTable.hpp"
#include "trie.h"
#include <string>
#define RO 256

typedef trie::trie_map<char, ankerl::unordered_dense::map<uint16_t, uint32_t>>
    Model;

class PpmModel {
private:
  Model *model;
  SimpleFrequencyTable *equal_probs;
  std::string history;
  int order;

public:
  PpmModel(int &order);
  void update(const uint16_t &symb);
  inline int getOrder() { return order; }
  inline std::string getHistory() { return history; }
  inline SimpleFrequencyTable *getInitialModelIt() { return equal_probs; };
  inline Model *getModel() { return model; };
  inline Model::iterator findModelIt(std::string subctx) {
    return this->model->find(subctx);
  }
  void printModel(const std::string &subctx);
};