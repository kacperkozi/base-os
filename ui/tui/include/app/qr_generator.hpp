#pragma once
#include <string>
#include <vector>

namespace app {

struct QRCode {
  std::vector<std::vector<bool>> modules;
  int size = 0;
  
  // Generate ASCII art representation
  std::string toASCII(bool invert = false) const;
  
  // Generate compact representation using block characters
  std::string toBlocks() const;
};

// Generate QR code from data
QRCode GenerateQR(const std::string& data);

} // namespace app