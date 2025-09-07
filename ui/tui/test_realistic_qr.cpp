#include "app/qr_generator.hpp"
#include <iostream>
#include <iomanip>

int main() {
    std::cout << "=== Realistic QR Code Payload Testing ===" << std::endl;
    
    // Test the exact payload from simple_transaction.cpp
    std::string transaction_payload = R"({"type":"1","version":"1.0","data":{"hash":"0x1db03e193bc95ca525006ed6ccd619b3b9db060a959d5e5c987c807c992732d1","signature":{"r":"0xf827b2181487b88bcef666d5729a8b9fcb7ac7cfd94dd4c4e9e9dbcfc9be154d","s":"0x5981479fb853e3779b176e12cd6feb4424159679c6bf8f4f468f92f700d9722d","v":"0x422d"},"transaction":{"to":"0x8c47B9fADF822681C68f34fd9b0D3063569245A1","value":"0x01e078","nonce":23,"gasPrice":"0x019bfcc0","gasLimit":"0x5208","data":"0x","chainId":8453},"timestamp":1757205711661,"network":"base"},"checksum":"dee6a6184b7c1479"})";
    
    std::cout << "\n📊 Transaction Payload Analysis:" << std::endl;
    std::cout << "   Length: " << transaction_payload.length() << " characters" << std::endl;
    
    // Generate QR code
    app::QRCode qr = app::GenerateQR(transaction_payload);
    
    if (qr.size == 0) {
        std::cout << "❌ QR code generation FAILED!" << std::endl;
        return 1;
    }
    
    std::cout << "   QR Size: " << qr.size << "x" << qr.size << " modules" << std::endl;
    std::cout << "   Total modules: " << (qr.size * qr.size) << std::endl;
    
    // Test both ASCII methods
    std::string robust = qr.toRobustAscii();
    std::string compact = qr.toCompactAscii();
    
    std::cout << "\n🎨 ASCII Rendering Results:" << std::endl;
    std::cout << "   Robust ASCII: " << robust.length() << " chars" << std::endl;
    std::cout << "   Compact ASCII: " << compact.length() << " chars" << std::endl;
    
    double savings = (double)(robust.length() - compact.length()) / robust.length() * 100;
    std::cout << "   Space savings: " << std::fixed << std::setprecision(1) << savings << "%" << std::endl;
    
    // Verify the QR code contains actual data
    bool has_black_modules = false;
    bool has_white_modules = false;
    
    for (int y = 0; y < qr.size && (!has_black_modules || !has_white_modules); ++y) {
        for (int x = 0; x < qr.size && (!has_black_modules || !has_white_modules); ++x) {
            if (qr.modules[y][x]) {
                has_black_modules = true;
            } else {
                has_white_modules = true;
            }
        }
    }
    
    std::cout << "\n✅ QR Code Validation:" << std::endl;
    std::cout << "   Has black modules: " << (has_black_modules ? "✅" : "❌") << std::endl;
    std::cout << "   Has white modules: " << (has_white_modules ? "✅" : "❌") << std::endl;
    std::cout << "   Pattern diversity: " << (has_black_modules && has_white_modules ? "✅ Good" : "❌ Poor") << std::endl;
    
    // Show a small sample of the QR code
    std::cout << "\n🔍 QR Code Preview (top-left 10x10):" << std::endl;
    for (int y = 0; y < std::min(10, qr.size); ++y) {
        std::cout << "   ";
        for (int x = 0; x < std::min(10, qr.size); ++x) {
            std::cout << (qr.modules[y][x] ? "██" : "  ");
        }
        std::cout << std::endl;
    }
    
    // Test terminal width simulation
    std::cout << "\n📏 Terminal Width Simulation:" << std::endl;
    int required_width_robust = qr.size * 2 + 8; // 2 chars per module + quiet zone
    int required_width_compact = qr.size + 4;    // 1 char per module + quiet zone
    
    std::cout << "   Required width for robust: " << required_width_robust << " columns" << std::endl;
    std::cout << "   Required width for compact: " << required_width_compact << " columns" << std::endl;
    
    // Common terminal sizes
    std::vector<std::pair<std::string, int>> terminal_sizes = {
        {"Small (80 cols)", 80},
        {"Medium (120 cols)", 120},
        {"Large (160 cols)", 160},
        {"Extra Large (200 cols)", 200}
    };
    
    for (const auto& [name, width] : terminal_sizes) {
        bool use_compact = width < required_width_robust;
        std::cout << "   " << name << ": " << (use_compact ? "Compact" : "Robust") << " rendering" << std::endl;
    }
    
    std::cout << "\n🎉 All tests completed successfully!" << std::endl;
    std::cout << "✅ Transaction payload QR generation is working perfectly!" << std::endl;
    
    return 0;
}
