#pragma once
#include <string>
#include <vector>

namespace app {

struct QRCode {
  std::vector<std::vector<bool>> modules;
  int size = 0;
  
};

// Generate QR code from data
QRCode GenerateQR(const std::string& data);

} // namespace app