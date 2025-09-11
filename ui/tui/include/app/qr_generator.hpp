#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "qrcodegen.hpp"  // For error correction level enum

namespace app {

// Default error correction level for QR codes
// QUARTILE (25% recovery) provides excellent scannability for on-screen display
constexpr auto DEFAULT_ERROR_CORRECTION = qrcodegen::QrCode::Ecc::QUARTILE;

struct QRCode {
  std::vector<std::vector<bool>> modules;
  int size = 0;
  int part = 1;
  int total_parts = 1;
  mutable std::string cached_ascii; // Mutable for caching in const method
  
  // Renders a font-independent, scannable QR code using ASCII characters.
  // Each module is rendered as a 2x1 character block ("##" or "  ") to
  // approximate a square aspect ratio in standard terminals.
  // Includes a spec-compliant 4-module quiet zone.
  std::string toRobustAscii() const;
  std::string toCompactAscii() const;
  std::string toHalfBlockAscii() const;
};

// Utility function to convert hex string to bytes
// Expects hex string starting with "0x"
std::vector<uint8_t> hex_to_bytes(const std::string& hex);

// Generate QR code from binary data
// ecc parameter allows customizing error correction level (default: QUARTILE for best scannability)
QRCode GenerateQR(const std::vector<uint8_t>& data, 
                  qrcodegen::QrCode::Ecc ecc = DEFAULT_ERROR_CORRECTION);
std::vector<QRCode> GenerateQRs(const std::vector<uint8_t>& data, 
                                size_t max_length = 100,
                                qrcodegen::QrCode::Ecc ecc = DEFAULT_ERROR_CORRECTION);

// Legacy string-based interface (kept for compatibility)
QRCode GenerateQR(const std::string& data, 
                  qrcodegen::QrCode::Ecc ecc = DEFAULT_ERROR_CORRECTION);
std::vector<QRCode> GenerateQRs(const std::string& data, 
                                size_t max_length = 100,
                                qrcodegen::QrCode::Ecc ecc = DEFAULT_ERROR_CORRECTION);

} // namespace app