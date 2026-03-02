#pragma once
#include "../include/FrequencyTable.hpp"
#include <algorithm>
#include <list>
#include <map>
#include <string>
#define RO 256

struct contextItem {
  uint32_t symb;
  uint32_t freq;
};

class PpmModel {
private:
  std::map<std::string, std::list<contextItem>> model;
  SimpleFrequencyTable *equal_probs;
  std::string history;
  int order;

public:
  PpmModel(int &order);
  void update(const uint32_t &symb);
  inline int getOrder() { return order; }
  inline std::string getHistory() { return history; }
  inline SimpleFrequencyTable *getInitialModelIt() { return equal_probs; };
  inline std::map<std::string, std::list<contextItem>> *getModel() {
    return &model;
  };
  inline std::list<contextItem> *findModelIt(std::string subctx) {
    if (this->model.find(subctx) == this->model.end())
      return nullptr;
    return &model[subctx];
  }
};

std::vector<uint32_t> createFrequencyTable(std::list<contextItem> *freqs);