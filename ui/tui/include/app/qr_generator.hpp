#pragma once
#include <string>
#include <vector>

namespace app {

struct QRCode {
  std::vector<std::vector<bool>> modules;
  int size = 0;
  int part = 1;
  int total_parts = 1;
  
  // Renders a font-independent, scannable QR code using ASCII characters.
  // Each module is rendered as a 2x1 character block ("##" or "  ") to
  // approximate a square aspect ratio in standard terminals.
  // Includes a spec-compliant 4-module quiet zone.
  std::string toRobustAscii() const;
  std::string toCompactAscii() const;
};

// Generate QR code from data
QRCode GenerateQR(const std::string& data);
std::vector<QRCode> GenerateQRs(const std::string& data, size_t max_length = 100);

} // namespace app