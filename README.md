# PPMC-compressor

A lossless data compressor and decompressor written in **C++17**, combining a custom **PPM (Prediction by Partial Matching) contextual model** with **Nayuki's arithmetic coding** engine for efficient entropy coding.

---

## Overview

PPMC-compressor implements the classic **PPM** compression scheme, where the compressor builds a probabilistic context model by tracking symbol frequencies across multiple context orders. The resulting probability distributions are fed into an arithmetic coder, achieving compression ratios close to the empirical entropy of the input data.


---

## Requirements

- C++17-compatible compiler (e.g., `g++ >= 7`, `clang++ >= 5`)
- GNU Make

---

## Building

Use the `Makefile` with the `TARGET` variable to select which binary to build.

**Compressor:**
```bash
make TARGET=compressor
```

**Decompressor:**
```bash
make TARGET=decompressor
```

To clean build artifacts:
```bash
make clean
```

---

## Usage

### Compress a file

```bash
./compressor <file> <order>
```

| Argument | Description |
|----------|-------------|
| `file`   | Path to the input file to compress |
| `order`  | PPM context order (e.g., `3`, `5`) — higher values may improve compression at the cost of memory and speed |

**Example:**
```bash
./compressor document.txt 5
```

---

### Decompress a file

```bash
./decompressor <file> <order>
```

| Argument | Description |
|----------|-------------|
| `file`   | Path to the compressed file to decompress |
| `order`  | PPM context order used during compression — **must match the value used when compressing** |

**Example:**
```bash
./decompressor document.txt.compressed 5
```

> **Important:** The `order` parameter must be identical between compression and decompression. Using a different order will produce incorrect output.

## References

- [Nayuki's Arithmetic Coding](https://www.nayuki.io/page/reference-arithmetic-coding) — Reference implementation used as the entropy coder

---

## License

See [LICENSE](LICENSE) for details.
