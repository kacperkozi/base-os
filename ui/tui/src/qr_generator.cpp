#include "app/qr_generator.hpp"
#include "app/qrcodegen.hpp"
#include <sstream>
#include <cmath>
#include <stdexcept>

namespace app {

std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    
    if (hex.empty()) {
        return bytes;
    }
    
    std::string hex_str = hex;
    if (hex_str.size() >= 2 && hex_str.substr(0, 2) == "0x") {
        hex_str = hex_str.substr(2);
    }
    
    if (hex_str.length() % 2 != 0) {
        throw std::invalid_argument("Hex string must have even length");
    }
    
    for (size_t i = 0; i < hex_str.length(); i += 2) {
        std::string byte_string = hex_str.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoul(byte_string, nullptr, 16));
        bytes.push_back(byte);
    }
    
    return bytes;
}

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

std::string QRCode::toHalfBlockAscii() const {
  if (!cached_ascii.empty()) {
    return cached_ascii;
  }
  if (modules.empty()) return "(No QR data)";

  std::stringstream result;
  const std::string top_half_black = "▀";
  const std::string bottom_half_black = "▄";
  const std::string both_halves_black = "█";
  const std::string both_halves_white = " ";
  const int quiet_zone = 1;

  // Top quiet zone
  for (int i = 0; i < quiet_zone; ++i) {
      result << std::string(size + quiet_zone * 2, ' ') << '\n';
  }

  for (int y = 0; y < size; y += 2) {
    result << std::string(quiet_zone, ' ');
    for (int x = 0; x < size; ++x) {
      bool top_module = modules[y][x];
      bool bottom_module = (y + 1 < size) ? modules[y + 1][x] : false;

      if (top_module && bottom_module) {
        result << both_halves_black;
      } else if (top_module) {
        result << top_half_black;
      } else if (bottom_module) {
        result << bottom_half_black;
      } else {
        result << both_halves_white;
      }
    }
    result << std::string(quiet_zone, ' ') << '\n';
  }

  // Bottom quiet zone
  for (int i = 0; i < quiet_zone; ++i) {
      result << std::string(size + quiet_zone * 2, ' ') << '\n';
  }

  cached_ascii = result.str();
  return cached_ascii;
}

QRCode GenerateQR(const std::vector<uint8_t>& data, qrcodegen::QrCode::Ecc ecc) {
  QRCode qr;
  
  try {
    std::vector<qrcodegen::QrSegment> segs;
    segs.push_back(qrcodegen::QrSegment::makeBytes(data));
    
    qrcodegen::QrCode qrCode = qrcodegen::QrCode::encodeSegments(
      segs,
      ecc,
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
    qr.size = 0;
  }
  
  return qr;
}
  
QRCode GenerateQR(const std::string& data, qrcodegen::QrCode::Ecc ecc) {
  std::vector<uint8_t> bytes(data.begin(), data.end());
  return GenerateQR(bytes, ecc);
}

std::vector<QRCode> GenerateQRs(const std::vector<uint8_t>& data, size_t max_length, qrcodegen::QrCode::Ecc ecc) {
  std::vector<QRCode> qrs;
  
  const size_t MAX_TOTAL_SIZE = 100000;
  const size_t MAX_QR_PARTS = 1000;
  const size_t MIN_CHUNK_SIZE = 10;
  
  if (data.empty() || data.size() > MAX_TOTAL_SIZE) {
    return qrs;
  }
  
  max_length = std::max(max_length, MIN_CHUNK_SIZE);
  
  if (data.size() <= max_length) {
    QRCode qr = GenerateQR(data, ecc);
    if (qr.size > 0) {
      qr.part = 1;
      qr.total_parts = 1;
      qrs.push_back(qr);
    }
    return qrs;
  }

  size_t num_parts = (data.size() + max_length - 1) / max_length;
  
  if (num_parts > MAX_QR_PARTS) {
    return qrs;
  }
  
  int parts_count = static_cast<int>(num_parts);
  
  for (int i = 0; i < parts_count; ++i) {
    std::string part_header = "P" + std::to_string(i + 1) + "/" + std::to_string(num_parts) + ":";
    std::vector<uint8_t> header_bytes(part_header.begin(), part_header.end());
    
    size_t start = static_cast<size_t>(i) * max_length;
    
    if (start >= data.size()) {
      break;
    }
    
    size_t length = std::min(max_length, data.size() - start);
    
    std::vector<uint8_t> chunk;
    chunk.insert(chunk.end(), header_bytes.begin(), header_bytes.end());
    chunk.insert(chunk.end(), data.begin() + start, data.begin() + start + length);

    try {
      std::vector<qrcodegen::QrSegment> segs;
      segs.push_back(qrcodegen::QrSegment::makeBytes(chunk));
      
      qrcodegen::QrCode qrCode = qrcodegen::QrCode::encodeSegments(
        segs,
        ecc,
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
      QRCode qr;
      qr.size = 0;
      qr.part = i + 1;
      qr.total_parts = num_parts;
      qrs.push_back(qr);
    }
  }

  return qrs;
}

std::vector<QRCode> GenerateQRs(const std::string& data, size_t max_length, qrcodegen::QrCode::Ecc ecc) {
  std::vector<uint8_t> bytes(data.begin(), data.end());
  return GenerateQRs(bytes, max_length, ecc);
}

} // namespace app