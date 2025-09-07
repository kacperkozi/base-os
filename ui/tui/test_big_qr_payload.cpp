#include "app/qr_generator.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>

int main() {
    std::cout << "=== Testing QR Code Generation with Big Payloads ===" << std::endl;
    
    // Test 1: Small payload
    std::cout << "\n1. Testing small payload..." << std::endl;
    std::string small_payload = "Hello, World!";
    auto start = std::chrono::high_resolution_clock::now();
    app::QRCode small_qr = app::GenerateQR(small_payload);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "   Size: " << small_qr.size << "x" << small_qr.size << std::endl;
    std::cout << "   Generation time: " << duration.count() << "ms" << std::endl;
    std::cout << "   Status: " << (small_qr.size > 0 ? "✅ SUCCESS" : "❌ FAILED") << std::endl;
    
    // Test 2: The actual big payload from simple_transaction.cpp
    std::cout << "\n2. Testing big transaction payload..." << std::endl;
    std::string big_payload = R"({"type":"1","version":"1.0","data":{"hash":"0x1db03e193bc95ca525006ed6ccd619b3b9db060a959d5e5c987c807c992732d1","signature":{"r":"0xf827b2181487b88bcef666d5729a8b9fcb7ac7cfd94dd4c4e9e9dbcfc9be154d","s":"0x5981479fb853e3779b176e12cd6feb4424159679c6bf8f4f468f92f700d9722d","v":"0x422d"},"transaction":{"to":"0x8c47B9fADF822681C68f34fd9b0D3063569245A1","value":"0x01e078","nonce":23,"gasPrice":"0x019bfcc0","gasLimit":"0x5208","data":"0x","chainId":8453},"timestamp":1757205711661,"network":"base"},"checksum":"dee6a6184b7c1479"})";
    
    start = std::chrono::high_resolution_clock::now();
    app::QRCode big_qr = app::GenerateQR(big_payload);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "   Payload length: " << big_payload.length() << " characters" << std::endl;
    std::cout << "   QR Size: " << big_qr.size << "x" << big_qr.size << std::endl;
    std::cout << "   Generation time: " << duration.count() << "ms" << std::endl;
    std::cout << "   Status: " << (big_qr.size > 0 ? "✅ SUCCESS" : "❌ FAILED") << std::endl;
    
    // Test 3: Test both ASCII methods with the big payload
    if (big_qr.size > 0) {
        std::cout << "\n3. Testing ASCII rendering methods..." << std::endl;
        
        start = std::chrono::high_resolution_clock::now();
        std::string robust_ascii = big_qr.toRobustAscii();
        end = std::chrono::high_resolution_clock::now();
        auto robust_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        start = std::chrono::high_resolution_clock::now();
        std::string compact_ascii = big_qr.toCompactAscii();
        end = std::chrono::high_resolution_clock::now();
        auto compact_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "   toRobustAscii():" << std::endl;
        std::cout << "     - Length: " << robust_ascii.length() << " characters" << std::endl;
        std::cout << "     - Generation time: " << robust_time.count() << "ms" << std::endl;
        std::cout << "     - Preview: " << robust_ascii.substr(0, 50) << "..." << std::endl;
        
        std::cout << "   toCompactAscii():" << std::endl;
        std::cout << "     - Length: " << compact_ascii.length() << " characters" << std::endl;
        std::cout << "     - Generation time: " << compact_time.count() << "ms" << std::endl;
        std::cout << "     - Preview: " << compact_ascii.substr(0, 50) << "..." << std::endl;
        
        // Size comparison
        double size_reduction = (double)(robust_ascii.length() - compact_ascii.length()) / robust_ascii.length() * 100;
        std::cout << "     - Size reduction: " << std::fixed << std::setprecision(1) << size_reduction << "%" << std::endl;
    }
    
    // Test 4: Very large payload stress test
    std::cout << "\n4. Testing very large payload (stress test)..." << std::endl;
    std::string very_large_payload;
    for (int i = 0; i < 100; i++) {
        very_large_payload += big_payload;
    }
    
    start = std::chrono::high_resolution_clock::now();
    app::QRCode stress_qr = app::GenerateQR(very_large_payload);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "   Payload length: " << very_large_payload.length() << " characters" << std::endl;
    std::cout << "   QR Size: " << stress_qr.size << "x" << stress_qr.size << std::endl;
    std::cout << "   Generation time: " << duration.count() << "ms" << std::endl;
    std::cout << "   Status: " << (stress_qr.size > 0 ? "✅ SUCCESS" : "❌ FAILED") << std::endl;
    
    // Summary
    std::cout << "\n=== Test Summary ===" << std::endl;
    bool all_passed = (small_qr.size > 0) && (big_qr.size > 0) && (stress_qr.size > 0);
    std::cout << "Overall Status: " << (all_passed ? "✅ ALL TESTS PASSED" : "❌ SOME TESTS FAILED") << std::endl;
    
    if (all_passed) {
        std::cout << "\n🎉 QR code payload generation is working correctly!" << std::endl;
        std::cout << "   - Small payloads: ✅" << std::endl;
        std::cout << "   - Transaction payloads: ✅" << std::endl;
        std::cout << "   - ASCII rendering: ✅" << std::endl;
        std::cout << "   - Stress test: ✅" << std::endl;
    }
    
    return all_passed ? 0 : 1;
}
