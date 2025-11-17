#pragma once

#include <fstream>
#include <vector>
namespace util {
    
inline std::vector<char> readSpvFile(const std::string& filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open SPV file: " + filename);
  }

  auto fileSize = static_cast<size_t>(file.tellg());
  auto buffer = std::vector<char>(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();
  return buffer;
}
}  // namespace util