#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include <utility>
#include <cstdint>
#include <fstream>
#include <chrono>
#include <string_view>

#include "ArithmeticCoder.hpp"
#include "BitIoStream.hpp"
#include "FrequencyTable.hpp"

typedef std::unordered_map<std::string_view, std::unordered_map<unsigned char, uint32_t>> ContextualTable;

void print_context(
    std::vector<std::unordered_map<std::string_view, std::unordered_map<unsigned char, uint32_t>>> &contextual_tables
) {
    for(size_t i = 0; i < contextual_tables.size(); i++) {
        if(contextual_tables[i].size() == 0) {
            continue;
        }
        std::cout << "K = " << i << std::endl;
        
        std::unordered_map<std::string_view, std::unordered_map<unsigned char, uint32_t>>::iterator it;
        for(it = contextual_tables[i].begin(); it != contextual_tables[i].end(); it++) {
            std::cout << "Contexto " << it->first << std::endl;
            std::unordered_map<unsigned char, uint32_t>::iterator it2;
            std::cout << "meu ro: " << it->second.size() << std::endl;
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
    std::vector<std::unordered_map<std::string_view, std::unordered_map<unsigned char, uint32_t>>> &contextual_tables,
    unsigned char byte,
    std::string &msg 
) {
    // atualização das tabelas
    for(int j = std::min(i, k); j >= 0; j--) {
        // contexto
        std::string_view ctx_substr(msg.data() + (i - j), j);
        contextual_tables[j][ctx_substr][byte] += 1;
    }

    // print_context(contextual_tables);
}

int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cout << "Uso: ./compressor <arquivo> <k: Optional>\n";
        return 1;
    }

    std::string filename = argv[1];
    int k;
    if(argc > 2) {
        k = std::stoi(argv[2]);
    } else {
        k = 2;
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

    std::vector<ContextualTable> contextual_tables(k+1);
    ContextualTable::iterator tuple_ctx_table;
    std::vector<uint32_t> unkown_symbols(256, 1);
    std::vector<uint32_t> freqs(257, 0);
    std::unordered_map<unsigned char, uint32_t>::iterator it;
    int symbol;


    const std::string ppm_ext = ".ppm";
    const std::string output_file = filename + ppm_ext;
    std::ofstream outFile(output_file, std::ios::binary);

    if (!outFile) {
        std::cout << "Erro ao criar arquivo de saída.\n";
        return 1;
    }
    
    BitOutputStream bitOut(outFile);
    ArithmeticEncoder encoder(32, bitOut);
    auto start = std::chrono::steady_clock::now();
    // msg = "abracadabra";
    for(int i = 0; i < static_cast<int>(msg.size()); i++) {
        std::vector<bool> excluded_symbols(256, false);
        unsigned char byte = msg[i];

        int j;
        for(j = std::min(k, i); j >= 0; j--) {
            std::string_view ctx_substr(msg.data() + (i - j), j);

            // procurando contexto com k símbolos na tabela k
            tuple_ctx_table = contextual_tables[j].find(ctx_substr);

            if(tuple_ctx_table == contextual_tables[j].end()) {
                contextual_tables[j][ctx_substr][byte] += 1; // atualizando a tabela de frequências
                // print_context(contextual_tables);
                continue;
            }

            auto &ctx_table = tuple_ctx_table->second;

            // procurando se existe o símbolo no contexto
            // caso não encontre, vê quais símbolos não devem ser considerados nas 
            // próximas tabelas (mecânismo de exclusão) e codifica um ro
            if(ctx_table.find(byte) == ctx_table.end()) {
                for(it = ctx_table.begin(); it != ctx_table.end(); it++) {
                    symbol = static_cast<int>(it->first);
                    excluded_symbols[symbol] = true; // adicionando na tabela para exclusão
                    freqs[symbol] = it->second; // montando tabela para codificar o ro 
                }

                // exclui o ro se tiver achado todos os símbolos nesse contexto
                if(ctx_table.size() == 256) {
                    freqs[256] = 0;
                } else {
                    freqs[256] = ctx_table.size(); // frequência do ro
                }

                encoder.write(SimpleFrequencyTable(freqs), 256);

                // limpa freqs para a próxima tabela
                for(it = ctx_table.begin(); it != ctx_table.end(); it++) {
                    symbol = static_cast<int>(it->first);
                    freqs[symbol] = 0; // montando tabela para codificar um ro 
                }
                freqs[256] = 0;

                ctx_table[byte] += 1; // atualizando a tabela de frequências
                // print_context(contextual_tables);
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
            if(ctx_table.size() == 256) {
                freqs[256] = 0;
            } else {
                freqs[256] = ctx_table.size(); // frequência do ro
            }

            encoder.write(SimpleFrequencyTable(freqs), byte);

            // limpa freqs para o próximo símbolo 
            for(it = ctx_table.begin(); it != ctx_table.end(); it++) {
                symbol = static_cast<int>(it->first);
                freqs[symbol] = 0;
            }

            freqs[256] = 0;
            
            ctx_table[byte] += 1; // atualizando a tabela de frequências

            break;
        }

        if(j == -1) {
            encoder.write(SimpleFrequencyTable(unkown_symbols), byte);
            unkown_symbols[static_cast<int>(byte)] = 0;
        }
        
        update_context(i, j-1, contextual_tables, byte, msg);
        // print_context(contextual_tables);
    }
    print_context(contextual_tables);

    auto end = std::chrono::steady_clock::now();
    auto duracao = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Tempo: " << duracao.count() << " s\n";

    encoder.finish();
    bitOut.finish();
    outFile.close();

    return 0;
}