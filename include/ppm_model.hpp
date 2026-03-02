#pragma once
#include "../include/FrequencyTable.hpp"
#include <map>
#include <string>
#include <vector>
#define RO 256

class PpmModel {
private:
  std::map<std::string, std::vector<u_int32_t>> model;
  SimpleFrequencyTable *equal_probs;
  std::string history;
  int order;

public:
  PpmModel(int &order);
  void update(const uint16_t &symb);
  inline int getOrder() { return order; }
  inline std::string getHistory() { return history; }
  inline SimpleFrequencyTable *getInitialModelIt() { return equal_probs; };
  inline std::map<std::string, std::vector<u_int32_t>> *getModel() {
    return &model;
  };
  inline std::map<std::string, std::vector<u_int32_t>>::iterator
  findModelIt(std::string subctx) {
    return this->model.find(subctx);
  }
};