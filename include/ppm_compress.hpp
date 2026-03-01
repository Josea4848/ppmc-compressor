#pragma once

#include "ArithmeticCoder.hpp"
#include "BitIoStream.hpp"
#include "FrequencyTable.hpp"

void compress(std::iostream in, BitOutputStream &out);
