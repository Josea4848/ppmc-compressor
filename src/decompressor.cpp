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
                std::cout << "Símbolo: " << static_cast<unsigned char>(it2->first) << " | Código: " << it2->first << " | Contador: " << it2->second << std::endl;
            }
            std::cout << std::endl; 
        }
    }
    std::cout << "-------------------------------------------" << std::endl;
}


void update_context(
    std::vector<std::unordered_map<std::string, std::unordered_map<uint32_t, uint32_t>>> &contextual_tables,
    uint32_t byte,
    std::string &msg 
) {
    // atualização das tabelas
    for(int j = static_cast<int>(msg.size()); j >= 0; j--) {
        // contexto
        std::string ctx_substr = msg.substr(msg.size()-j, j);
        contextual_tables[j][ctx_substr][byte] += 1;
    }
}

static void decompress(std::ostream &out, ArithmeticDecoder &decoder, int k);

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

    BitInputStream bitIn(inFile);
    ArithmeticDecoder decoder(32, bitIn);

    std::string output_file = filename;
    size_t pos = output_file.find_last_of('.');
    if (pos != std::string::npos) {
        output_file.erase(pos);
    }

    std::ofstream outFile(output_file, std::ios::binary);
    if (!outFile) {
        std::cout << "Erro ao criar arquivo de saída.\n";
        return 1;
    }

    try {

        auto start = std::chrono::high_resolution_clock::now();
        decompress(outFile, decoder, k);

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

    return EXIT_SUCCESS;
}


static void decompress(std::ostream &out, ArithmeticDecoder &decoder, int k) {
    std::vector<ContextualTable> contextual_tables(k+1);
    ContextualTable::iterator tuple_ctx_table;
    std::vector<uint32_t> unkown_symbols(257, 1);
    std::vector<uint32_t> freqs(257, 0);
    std::unordered_map<uint32_t, uint32_t>::iterator it;
    uint32_t decoded_symbol;
    int b = 0;
    int table_symbol = 0;
    std::string msg = "";

    while(true) {
        std::vector<bool> excluded_symbols(256, false);
        int msg_size = static_cast<int>(msg.size());
        std::cout << "msg:" << msg << std::endl;
        int j;
        for(j = msg_size; j >= 0; j--) {
            // contexto
            std::string ctx_substr = msg.substr(msg_size-j, j);

            tuple_ctx_table = contextual_tables[j].find(ctx_substr);
            if(tuple_ctx_table == contextual_tables[j].end()) {
                continue;
            }

            auto &ctx_table = tuple_ctx_table->second;

            for(it = ctx_table.begin(); it != ctx_table.end(); it++) {
                table_symbol = static_cast<int>(it->first);
                
                if(excluded_symbols[table_symbol]) {
                    continue;
                }
                
                freqs[table_symbol] = it->second; // montando tabela para codificar o ro 
                excluded_symbols[table_symbol] = true; // adicionando na tabela para exclusão
            }

            // exclui o ro se tiver achado todos os símbolos nesse contexto
            if(ctx_table.size() < 256) {
                freqs[256] = ctx_table.size(); // frequência do ro
            } else {
                freqs[256] = 0;
            }

            decoded_symbol = decoder.read(SimpleFrequencyTable(freqs));

            // limpa freqs para a próxima tabela
            for(it = ctx_table.begin(); it != ctx_table.end(); it++) {
                table_symbol = static_cast<int>(it->first);
                freqs[table_symbol] = 0; // montando tabela para codificar um ro 
            }
            freqs[256] = 0;

            if (decoded_symbol == 256)  { // ro
                std::cout << "ro " << ctx_substr << std::endl;
				continue;
            }

            b = static_cast<int>(decoded_symbol);
			if (std::numeric_limits<char>::is_signed) {
				b -= (b >> 7) << 8;
            }

			out.put(static_cast<char>(b));

            break;
        }

        if(j < 0) {
            decoded_symbol = decoder.read(SimpleFrequencyTable(unkown_symbols));
            for(auto g : unkown_symbols) {
                std::cout << g << " ";
            }
            std::cout << std::endl;
            std::cout << "decoded_symbol: " << decoded_symbol << std::endl;
            // EOF symbol
            if (decoded_symbol == 256)  { 
                break;
            }

            unkown_symbols[decoded_symbol] = 0;

            b = static_cast<int>(decoded_symbol);
			if (std::numeric_limits<char>::is_signed) {
				b -= (b >> 7) << 8;
            }

			out.put(static_cast<char>(b));
        }

        update_context(contextual_tables, b, msg);
        if(msg_size < k) {
            msg.push_back(static_cast<unsigned char>(b));
        } else {
            for(int i = 1; i < msg_size; i++) {
                msg[i-1] = msg[i];
            }
            msg[k-1] = static_cast<unsigned char>(b);
        }

    }

    std::cout << "antes" << std::endl;
    print_context(contextual_tables);
    std::cout << "depois" << std::endl;
}