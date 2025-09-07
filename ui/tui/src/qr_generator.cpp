#include "app/qr_generator.hpp"

#ifndef NO_ZXING
#include <ZXing/BitMatrix.h>
#include <ZXing/MultiFormatWriter.h>
#include <ZXing/TextUtfEncoding.h>
#endif

namespace app {

std::string QRCode::toASCII(bool invert) const {
  if (modules.empty()) return "(No QR data)";
  
  std::string result;
  const char* black = invert ? "  " : "██";
  const char* white = invert ? "██" : "  ";
  
  // Add quiet zone (border)
  for (int i = 0; i < size + 4; ++i) result += white;
  result += "\n";
  for (int i = 0; i < size + 4; ++i) result += white;
  result += "\n";
  
  for (int y = 0; y < size; ++y) {
    result += white; // Left quiet zone
    result += white;
    for (int x = 0; x < size; ++x) {
      result += modules[y][x] ? black : white;
    }
    result += white; // Right quiet zone
    result += white;
    result += "\n";
  }
  
  // Bottom quiet zone
  for (int i = 0; i < size + 4; ++i) result += white;
  result += "\n";
  for (int i = 0; i < size + 4; ++i) result += white;
  result += "\n";
  
  return result;
}

std::string QRCode::toBlocks() const {
  if (modules.empty()) return "(No QR data)";
  
  std::string result;
  // Using Unicode block elements for more compact display
  const char* blocks[] = {" ", "▀", "▄", "█"};
  
  // Process two rows at a time for compact representation
  for (int y = 0; y < size; y += 2) {
    for (int x = 0; x < size; ++x) {
      bool top = (y < size) ? modules[y][x] : false;
      bool bottom = (y + 1 < size) ? modules[y + 1][x] : false;
      
      int idx = (top ? 1 : 0) + (bottom ? 2 : 0);
      result += blocks[idx];
    }
    result += "\n";
  }
  
  return result;
}

QRCode GenerateQR(const std::string& data) {
  QRCode qr;
  
#ifndef NO_ZXING
  try {
    using namespace ZXing;
    
    auto writer = MultiFormatWriter(BarcodeFormat::QRCode);
    writer.setEncoding(CharacterSet::UTF8);
    writer.setMargin(0); // We'll add margin in rendering
    
    auto matrix = writer.encode(data, 0, 0);
    
    qr.size = matrix.width();
    qr.modules.resize(qr.size);
    
    for (int y = 0; y < qr.size; ++y) {
      qr.modules[y].resize(qr.size);
      for (int x = 0; x < qr.size; ++x) {
        qr.modules[y][x] = matrix.get(x, y);
      }
    }
  } catch (...) {
    // Fallback to placeholder
    qr.size = 21; // Minimal QR size
    qr.modules.resize(qr.size, std::vector<bool>(qr.size, false));
    
    // Create a simple pattern as placeholder
    for (int i = 0; i < 7; ++i) {
      for (int j = 0; j < 7; ++j) {
        // Top-left finder pattern
        if (i == 0 || i == 6 || j == 0 || j == 6 || (i >= 2 && i <= 4 && j >= 2 && j <= 4)) {
          qr.modules[i][j] = true;
        }
        // Top-right finder pattern
        if (i == 0 || i == 6 || j == 0 || j == 6 || (i >= 2 && i <= 4 && j >= 2 && j <= 4)) {
          qr.modules[i][qr.size - 7 + j] = true;
        }
        // Bottom-left finder pattern
        if (i == 0 || i == 6 || j == 0 || j == 6 || (i >= 2 && i <= 4 && j >= 2 && j <= 4)) {
          qr.modules[qr.size - 7 + i][j] = true;
        }
      }
    }
  }
#else
  // Placeholder implementation when ZXing is not available
  qr.size = 21;
  qr.modules.resize(qr.size, std::vector<bool>(qr.size, false));
  
  // Create a simple pattern
  for (int i = 0; i < qr.size; ++i) {
    qr.modules[i][0] = true;
    qr.modules[0][i] = true;
    qr.modules[i][qr.size-1] = true;
    qr.modules[qr.size-1][i] = true;
    if (i % 2 == 0) {
      qr.modules[qr.size/2][i] = true;
      qr.modules[i][qr.size/2] = true;
    }
  }
#endif
  
  return qr;
}

} // namespace app