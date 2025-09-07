#include "app/qr_generator.hpp"
#include "qrcodegen.hpp"
#include <sstream>
#include <cmath>

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

std::vector<QRCode> GenerateQRs(const std::string& data, size_t max_length) {
  std::vector<QRCode> qrs;
  
  // If data fits in a single QR code, return it as a single-part QR
  if (data.length() <= max_length) {
    QRCode qr = GenerateQR(data);
    if (qr.size > 0) {
      qr.part = 1;
      qr.total_parts = 1;
      qrs.push_back(qr);
    }
    return qrs;
  }

  // Calculate number of parts needed
  int num_parts = static_cast<int>(std::ceil(static_cast<double>(data.length()) / max_length));
  
  for (int i = 0; i < num_parts; ++i) {
    // Create part header like "P1/3:"
    std::string part_header = "P" + std::to_string(i + 1) + "/" + std::to_string(num_parts) + ":";
    
    // Extract chunk of data
    size_t start = i * max_length;
    size_t length = std::min(max_length, data.length() - start);
    std::string chunk = part_header + data.substr(start, length);

    try {
      // Generate QR code for this chunk
      std::vector<qrcodegen::QrSegment> segs = qrcodegen::QrSegment::makeSegments(chunk.c_str());
      qrcodegen::QrCode qrCode = qrcodegen::QrCode::encodeSegments(
        segs,
        qrcodegen::QrCode::Ecc::LOW,
        1, 40, -1, true
      );
      
      QRCode qr;
      qr.size = qrCode.getSize();
      qr.part = i + 1;
      qr.total_parts = num_parts;
      qr.modules.resize(qr.size, std::vector<bool>(qr.size));
      
      for (int y = 0; y < qr.size; ++y) {
        for (int x = 0; x < qr.size; ++x) {
          qr.modules[y][x] = qrCode.getModule(x, y);
        }
      }
      qrs.push_back(qr);
    } catch (const std::exception& e) {
      // If this chunk fails, create an empty QR to maintain part numbering
      QRCode qr;
      qr.size = 0;
      qr.part = i + 1;
      qr.total_parts = num_parts;
      qrs.push_back(qr);
    }
  }

  return qrs;
}

} // namespace app