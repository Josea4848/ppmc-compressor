#pragma once
#include "../include/FrequencyTable.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#define RO 256

class PpmModel {
private:
  std::unordered_map<std::string, std::vector<u_int32_t>> model;
  SimpleFrequencyTable *equal_probs;
  std::string history;
  int order;

public:
  PpmModel(int &order);
  void update(const uint16_t &symb);
  inline int getOrder() { return order; }
  inline std::string getHistory() { return history; }
  inline SimpleFrequencyTable *getInitialModelIt() { return equal_probs; };
  inline std::unordered_map<std::string, std::vector<u_int32_t>> *getModel() {
    return &model;
  };
  inline std::unordered_map<std::string, std::vector<u_int32_t>>::iterator
  findModelIt(std::string subctx) {
    return this->model.find(subctx);
  }
};