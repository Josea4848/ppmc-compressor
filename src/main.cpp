#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <utility>
#include <cstdint>
#include <fstream>

#include "ArithmeticCoder.hpp"
#include "BitIoStream.hpp"
#include "FrequencyTable.hpp"


void clear_freqs(std::vector<uint32_t> &freqs) {
    for(size_t i = 0; i < freqs.size(); i++) {
        freqs[i] = 0;         
    }
}


void update_context(
    int i, 
    int k,
    std::vector<std::map<std::string, std::map<std::string, uint32_t>>> &contextual_tables,
    unsigned char byte,
    std::string &msg 
) {
    // atualização das tabelas
    for(int j = 0; j < k; j++) {
        if(i < j) {
            continue;
        }

        // contexto
        std::string ctx_substr = msg.substr(i-j, j);
        contextual_tables[j][ctx_substr][std::string(1, byte)] += 1;
        contextual_tables[j][ctx_substr][""] += 1; // se começar com 0, dá para deixar assim
    }
}

int main() {
    int k = 3;
    std::string msg = "abracadabra";

    std::vector<std::map<std::string, std::map<std::string, uint32_t>>> contextual_tables(k);
    std::vector<uint32_t> unkown_symbols(256, 1);
    std::vector<uint32_t> freqs(257, 0);

    std::ofstream outFile("compressed.bin", std::ios::binary);

    if (!outFile) {
        std::cout << "Erro ao criar arquivo de saída.\n";
        return 1;
    }

    BitOutputStream bitOut(outFile);
    ArithmeticEncoder encoder(32, bitOut);
    
    contextual_tables[0][""][""] = 0;
    for(int i = 0; i < static_cast<int>(msg.size()); i++) {
        std::vector<bool> excluded_symbols(256, false);
        std::map<std::string, uint32_t>::iterator it;
        unsigned char byte = msg[i];
        std::cout << "byte: " << byte << std::endl;

        if ( unkown_symbols[ static_cast<int>(msg[i]) ] ) {
            SimpleFrequencyTable freqTable(unkown_symbols);

            encoder.write(freqTable, byte);

            unkown_symbols[static_cast<int>(msg[i])] = 0; 
        } else {
            for(int j = k-1; j >= 0; j--) {
                if(i < j) {
                    continue;
                }

                // contexto
                std::string ctx_substr = msg.substr(i-j, j);

                // procurando contexto com k símbolos na tabela k
                std::map<std::string, std::map<std::string, uint32_t>>::iterator ctx_table;
                ctx_table = contextual_tables[j].find(ctx_substr);

                if(ctx_table == contextual_tables[j].end()) {
                    continue;
                }

                // procurando se existe o símbolo no contexto
                std::map<std::string, uint32_t>::iterator iter_symbol_counter = ctx_table->second.find(std::string(1, msg[i]));

                // caso não encontre, vê quais símbolos não devem ser considerados nas 
                // próximas tabelas (mecânismo de exclusão) e codifica um ro
                if(iter_symbol_counter == ctx_table->second.end()){
                    for(it = ctx_table->second.begin(); it != ctx_table->second.end(); it++) {
                        if(it->first.compare("") != 0) {
                            excluded_symbols[static_cast<int>(it->second)] = true; // adicionando na tabela para exclusão
                            freqs[static_cast<uint32_t>(it->first[0])] = it->second; // montando tabela para codificar um ro 
                        } else {
                            freqs[256] = it->second; // montando tabela para codificar um ro 
                        }
                    }

                    SimpleFrequencyTable freqTable(freqs);
                    encoder.write(freqTable, byte);

                    // limpa freqs para a próxima tabela
                    clear_freqs(freqs);
                    continue;
                }
                
                for(it = ctx_table->second.begin(); it != ctx_table->second.end(); it++) {
                    if(it->first.compare("") == 0) {
                        freqs[256] = it->second;
                        continue;
                    }

                    int symbol = static_cast<uint32_t>(it->first[0]);
                    if( excluded_symbols[symbol] ) {
                        continue;
                    }

                    freqs[symbol] = it->second;
                }

                SimpleFrequencyTable freqTable(freqs);
                encoder.write(freqTable, byte);

                // limpa freqs para o próximo símbolo 
                clear_freqs(freqs);
            }
        }

        update_context(i, k, contextual_tables, byte, msg);
        
    }

    encoder.finish();
    bitOut.finish();
    outFile.close();

    return 0;
}