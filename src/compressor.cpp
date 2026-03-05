#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include <utility>
#include <cstdint>
#include <fstream>
#include <chrono>

#include "ArithmeticCoder.hpp"
#include "BitIoStream.hpp"
#include "FrequencyTable.hpp"

#define RO 256

typedef std::unordered_map<std::string, std::unordered_map<uint32_t, uint32_t>> ContextualTable;

static void compress(std::ifstream &in, ArithmeticEncoder encoder, int order);

void print_context(
    std::vector<std::unordered_map<std::string, std::unordered_map<uint32_t, uint32_t>>> &contextual_tables
) {
    for(size_t i = 0; i < contextual_tables.size(); i++) {
        if(contextual_tables[i].size() == 0) {
            continue;
        }
        std::cout << "K = " << i << std::endl;
        
        std::unordered_map<std::string, std::unordered_map<uint32_t, uint32_t>>::iterator it;
        for(it = contextual_tables[i].begin(); it != contextual_tables[i].end(); it++) {
            std::cout << "Contexto " << it->first << std::endl;
            std::unordered_map<uint32_t, uint32_t>::iterator it2;
            std::cout << "meu ro: " << it->second.size() << std::endl;
            for(it2 = it->second.begin(); it2 != it->second.end(); it2++) {
                std::cout << "Símbolo: " << static_cast<unsigned char>(it2->first) << " | Contador: " << it2->second << std::endl;
            }
            std::cout << std::endl; 
        }
    }
    std::cout << "-------------------------------------------" << std::endl;
}


void update_context(
    int i,
    std::vector<std::unordered_map<std::string, std::unordered_map<uint32_t, uint32_t>>> &contextual_tables,
    uint32_t byte,
    std::string &msg 
) {
    // atualização das tabelas
    for(int j = std::min(i, static_cast<int>(msg.size())); j >= 0; j--) {
        // contexto
        std::string ctx_substr = msg.substr(msg.size()-j, j);
        contextual_tables[j][ctx_substr][byte]++;
    }
}

int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cout << "Uso: ./compressor <arquivo> <k: Optional>\n";
        return 1;
    }

    std::string filename = argv[1];

    int k;
    if(argc > 2) k = std::stoi(argv[2]);
    else k = 2;

    std::ifstream inFile(filename, std::ios::binary);

    if (!inFile) {
        std::cout << "Erro ao abrir arquivo.\n";
        return 1;
    }

    const std::string ppm_ext = ".ppmc";
    const std::string output_file = filename + ppm_ext;
    std::ofstream outFile(output_file, std::ios::binary);

    if (!outFile) {
        std::cout << "Erro ao criar arquivo de saída.\n";
        return 1;
    }
    
    BitOutputStream bitOut(outFile);
    ArithmeticEncoder encoder(32, bitOut);

    auto start = std::chrono::high_resolution_clock::now();
    try {
        compress(inFile, encoder, k);
        bitOut.finish();

        // Fim de execução
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration<double>(end - start);
        std::cout << "Tempo de execução: " << duration.count() << " s\n";
        return EXIT_SUCCESS;
    }

    catch (const char *msg) {
        std::cerr << msg << std::endl;
        return EXIT_FAILURE;
    }
}


static void compress(std::ifstream &in, ArithmeticEncoder encoder, int k) {
    std::vector<ContextualTable> contextual_tables(k+1);
    ContextualTable::iterator tuple_ctx_table;
    std::vector<uint32_t> unkown_symbols(257, 1);
    std::vector<uint32_t> freqs(257, 0);
    std::unordered_map<uint32_t, uint32_t>::iterator it;
    int symbol;
    std::string msg = "";
    
    while(true) {
        std::vector<bool> excluded_symbols(257, false);
        uint32_t byte = in.get(); 
        if (static_cast<int>(byte) == std::char_traits<char>::eof()) {
            byte = 256;
        }

        std::cout << "byte: " << byte << std::endl;
        int j;
        int msg_size = static_cast<int>(msg.size());
        for(j = msg_size; j >= 0; j--) {
            std::string ctx_substr = msg.substr(msg_size-j, j);

            // procurando contexto com k símbolos na tabela k
            tuple_ctx_table = contextual_tables[j].find(ctx_substr);

            if(tuple_ctx_table == contextual_tables[j].end()) {
                contextual_tables[j][ctx_substr][byte]++; // atualizando a tabela de frequências
                continue;
            }

            auto &ctx_table = tuple_ctx_table->second;

            // procurando se existe o símbolo no contexto
            // caso não encontre, vê quais símbolos não devem ser considerados nas 
            // próximas tabelas (mecânismo de exclusão) e codifica um ro
            if(ctx_table.find(byte) == ctx_table.end()) {
                std::cout << "RO" << std::endl;
                for(it = ctx_table.begin(); it != ctx_table.end(); it++) {
                    symbol = static_cast<int>(it->first);
                    
                    if(excluded_symbols[symbol] && byte != 256) {
                        continue;
                    }

                    freqs[symbol] = it->second; // montando tabela para codificar o ro 
                    excluded_symbols[symbol] = true; // adicionando na tabela para exclusão
                }

                // exclui o ro se tiver achado todos os símbolos nesse contexto
                if(ctx_table.size() < 256) {
                    freqs[256] = ctx_table.size(); // frequência do ro
                } else {
                    freqs[256] = 0;
                }

                encoder.write(SimpleFrequencyTable(freqs), RO);

                // limpa freqs para a próxima tabela
                for(it = ctx_table.begin(); it != ctx_table.end(); it++) {
                    symbol = static_cast<int>(it->first);
                    freqs[symbol] = 0; // montando tabela para codificar um ro 
                }
                freqs[256] = 0;

                ctx_table[byte]++; // atualizando a tabela de frequências
                continue;
            }
        
            for(it = ctx_table.begin(); it != ctx_table.end(); it++) {
                int symbol = static_cast<int>(it->first);
                if( excluded_symbols[symbol] ) {
                    continue;
                }

                freqs[symbol] = it->second;
            }

            // exclui o ro se tiver achado todos os símbolos nesse contexto
            if(ctx_table.size() < 256) {
                freqs[256] = ctx_table.size(); // frequência do ro
            } else {
                freqs[256] = 0;
            }

            encoder.write(SimpleFrequencyTable(freqs), byte);

            // limpa freqs para o próximo símbolo 
            for(it = ctx_table.begin(); it != ctx_table.end(); it++) {
                symbol = static_cast<int>(it->first);
                freqs[symbol] = 0;
            }
            freqs[256] = 0;
            
            ctx_table[byte]++; // atualizando a tabela de frequências

            break;
        }

        if(j < 0) {
            std::cout << "ignorancia" << std::endl;
            if (byte == 256) {
                std::cout << "eof" << std::endl;
                encoder.write(SimpleFrequencyTable(unkown_symbols), RO);
                break;
            } else {
                encoder.write(SimpleFrequencyTable(unkown_symbols), byte);
                unkown_symbols[static_cast<int>(byte)] = 0;
            }
        }
        
        update_context(j-1, contextual_tables, byte, msg);
        if(msg_size < k) {
            msg.push_back(static_cast<unsigned char>(byte));
        } else {
            for(int r = 1; r < msg_size; r++) {
                msg[r-1] = msg[r];
            }
            msg[k-1] = static_cast<unsigned char>(byte);
        }
    }
    print_context(contextual_tables);
}