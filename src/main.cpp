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


void print_context(
    std::vector<std::map<std::string, std::map<std::string, uint32_t>>> &contextual_tables
) {
    for(size_t i = 0; i < contextual_tables.size(); i++) {
        if(contextual_tables[i].size() == 0) {
            continue;
        }
        std::cout << "K = " << i << std::endl;
        
        std::map<std::string, std::map<std::string, uint32_t>>::iterator it;
        for(it = contextual_tables[i].begin(); it != contextual_tables[i].end(); it++) {
            std::cout << "Contexto " << it->first << std::endl;
            std::map<std::string, uint32_t>::iterator it2;
            for(it2 = it->second.begin(); it2 != it->second.end(); it2++) {
                std::cout << "Símbolo: " << it2->first << " | Contador: " << it2->second << std::endl;
            }
            std::cout << std::endl; 
        }
    }
    std::cout << "-------------------------------------------" << std::endl;
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
        if(contextual_tables[j][ctx_substr][std::string(1, byte)] == 1) {
            contextual_tables[j][ctx_substr][""] += 1; // se começar com 0, dá para deixar assim
        }
    }
    
    // print_context(contextual_tables);
}

int main(int argc, char* argv[]) {

    if (argc < 3) {
        std::cout << "Uso: ./compressor <arquivo> <k>\n";
        return 1;
    }

    std::string filename = argv[1];
    int k;
    if(argc > 2) {
        k = std::stoi(argv[2]);
    } else {
        k = 3;
    }

    std::ifstream inFile(filename, std::ios::binary);

    if (!inFile) {
        std::cout << "Erro ao abrir arquivo.\n";
        return 1;
    }

    std::string msg{
        std::istreambuf_iterator<char>(inFile),
        std::istreambuf_iterator<char>()};

    inFile.close();

    if (msg.empty()) {
        std::cout << "Arquivo vazio.\n";
        return 1;
    }

    std::cout << "Bytes lidos: " << msg.size() << "\n";
    getchar();

    std::vector<std::map<std::string, std::map<std::string, uint32_t>>> contextual_tables(k);
    std::map<std::string, std::map<std::string, uint32_t>>::iterator ctx_table;
    std::vector<uint32_t> unkown_symbols(256, 1);
    std::vector<uint32_t> freqs(257, 0);

    std::ofstream outFile("compressed.bin", std::ios::binary);

    if (!outFile) {
        std::cout << "Erro ao criar arquivo de saída.\n";
        return 1;
    }

    BitOutputStream bitOut(outFile);
    ArithmeticEncoder encoder(32, bitOut);
    
    for(int i = 0; i < static_cast<int>(msg.size()); i++) {
        std::vector<bool> excluded_symbols(256, false);
        std::map<std::string, uint32_t>::iterator it;
        unsigned char byte = msg[i];
        bool encoded = false;

        std::cout << "byte: " << byte << std::endl;

        for(int j = k-1; j >= 0; j--) {
            if(i < j) {
                continue;
            }

            // contexto
            std::string ctx_substr = msg.substr(i-j, j);

            // procurando contexto com k símbolos na tabela k
            ctx_table = contextual_tables[j].find(ctx_substr);

            if(ctx_table == contextual_tables[j].end()) {
                continue;
            }

            // procurando se existe o símbolo no contexto
            std::map<std::string, uint32_t>::iterator iter_symbol_counter = ctx_table->second.find(std::string(1, msg[i]));

            // caso não encontre, vê quais símbolos não devem ser considerados nas 
            // próximas tabelas (mecânismo de exclusão) e codifica um ro
            if(iter_symbol_counter == ctx_table->second.end()) {
                
                for(it = ctx_table->second.begin(); it != ctx_table->second.end(); it++) {
                    if(it->first.compare("") != 0) {
                        int pos = static_cast<int>(it->first[0]);
                        excluded_symbols[pos] = true; // adicionando na tabela para exclusão
                        freqs[pos] = it->second; // montando tabela para codificar o ro 
                    } else {
                        freqs[256] = it->second; // montando tabela para codificar o ro 
                    }
                }

                SimpleFrequencyTable freqTable(freqs);
                encoder.write(freqTable, 256);

                // limpa freqs para a próxima tabela
                for(it = ctx_table->second.begin(); it != ctx_table->second.end(); it++) {
                    if(it->first.compare("") != 0) {
                        freqs[static_cast<int>(it->first[0])] = 0; // montando tabela para codificar um ro 
                    } else {
                        freqs[256] = 0; // montando tabela para codificar um ro 
                    }
                }

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
            for(it = ctx_table->second.begin(); it != ctx_table->second.end(); it++) {
                if(it->first.compare("") == 0) {
                    freqs[256] = 0;
                    continue;
                }

                freqs[static_cast<int>(it->first[0])] = 0;
            }
            encoded = true;
            break;
        }

        if(!encoded) {
            SimpleFrequencyTable freqTable(unkown_symbols);
            encoder.write(freqTable, byte);
            unkown_symbols[static_cast<int>(msg[i])] = 0;
        }

        update_context(i, k, contextual_tables, byte, msg);
        
    }

    encoder.finish();
    bitOut.finish();
    outFile.close();

    return 0;
}