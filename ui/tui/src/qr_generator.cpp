#include "app/qr_generator.hpp"
#include "qrcodegen.hpp"
#include <sstream>

namespace app {

std::string QRCode::toRobustAscii() const {
  if (modules.empty()) return "(No QR data)";
  
  std::string result;
  const std::string black = "##";
  const std::string white = "  ";
  const int quiet_zone = 4;

  auto draw_horizontal_border = [&]() {
    for (int i = 0; i < quiet_zone; ++i) {
      for (int j = 0; j < size + 2 * quiet_zone; ++j) {
        result += white;
      }
      result += '\n';
    }
  };

  draw_horizontal_border();

  for (int y = 0; y < size; ++y) {
    for (int i = 0; i < quiet_zone; ++i) result += white;
    for (int x = 0; x < size; ++x) {
      result += modules[y][x] ? black : white;
    }
    for (int i = 0; i < quiet_zone; ++i) result += white;
    result += '\n';
  }

  draw_horizontal_border();
  
  return result;
}

std::string QRCode::toCompactAscii() const {
  if (modules.empty()) return "(No QR data)";
  
  std::string result;
  const std::string black = "█";
  const std::string white = " ";
  const int quiet_zone = 2;

  auto draw_horizontal_border = [&]() {
    for (int i = 0; i < quiet_zone; ++i) {
      for (int j = 0; j < size + 2 * quiet_zone; ++j) {
        result += white;
      }
      result += '\n';
    }
  };

  draw_horizontal_border();

  for (int y = 0; y < size; ++y) {
    for (int i = 0; i < quiet_zone; ++i) result += white;
    for (int x = 0; x < size; ++x) {
      result += modules[y][x] ? black : white;
    }
    for (int i = 0; i < quiet_zone; ++i) result += white;
    result += '\n';
  }

  draw_horizontal_border();
  
  return result;
}
  
QRCode GenerateQR(const std::string& data) {
  QRCode qr;
  
  try {
    // Manually create segments to gain more control over generation
    std::vector<qrcodegen::QrSegment> segs = qrcodegen::QrSegment::makeSegments(data.c_str());
    
    // Use encodeSegments to specify advanced parameters
    qrcodegen::QrCode qrCode = qrcodegen::QrCode::encodeSegments(
      segs,
      qrcodegen::QrCode::Ecc::LOW,
      1, 40, -1, true
    );
    
    qr.size = qrCode.getSize();
    qr.modules.resize(qr.size, std::vector<bool>(qr.size));
    
    for (int y = 0; y < qr.size; ++y) {
      for (int x = 0; x < qr.size; ++x) {
        qr.modules[y][x] = qrCode.getModule(x, y);
      }
    }
  } catch (const std::exception& e) {
    // Fallback for safety, though should be rare.
    qr.size = 0;
  }
  
  return qr;
}

} // namespace app