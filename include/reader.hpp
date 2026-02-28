#pragma once
#include <fstream>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <functional>

class Reader {
public:
    explicit Reader(const std::string& tarPath)
        : input(tarPath, std::ios::binary)
    {
        if (!input)
            throw std::runtime_error("Erro ao abrir arquivo .tar");
    }

    void streamTo(const std::function<void(const uint8_t*, size_t)>& callback) {
        const size_t BUFFER_SIZE = 8192;
        std::vector<uint8_t> buffer(BUFFER_SIZE);

        while (input) {
            input.read(reinterpret_cast<char*>(buffer.data()), BUFFER_SIZE);
            std::streamsize bytesRead = input.gcount();

            if (bytesRead > 0) {
                callback(buffer.data(), static_cast<size_t>(bytesRead));
            }
        }
    }

private:
    std::ifstream input;
};