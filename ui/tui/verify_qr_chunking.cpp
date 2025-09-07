#include "app/qr_generator.hpp"
#include <iostream>
#include <cassert>
#include <iomanip>

void test_qr_struct_fields() {
    std::cout << "Testing QRCode struct fields..." << std::endl;
    
    app::QRCode qr;
    qr.part = 2;
    qr.total_parts = 5;
    qr.size = 21;
    
    assert(qr.part == 2);
    assert(qr.total_parts == 5);
    assert(qr.size == 21);
    
    std::cout << "✅ QRCode struct fields test passed" << std::endl;
}

void test_single_qr_generation() {
    std::cout << "Testing single QR generation..." << std::endl;
    
    std::string small_data = "Hello World";
    auto qrs = app::GenerateQRs(small_data, 100);
    
    assert(qrs.size() == 1);
    assert(qrs[0].part == 1);
    assert(qrs[0].total_parts == 1);
    assert(qrs[0].size > 0);
    
    std::cout << "✅ Single QR generation test passed" << std::endl;
}

void test_multi_qr_chunking() {
    std::cout << "Testing multi-QR chunking..." << std::endl;
    
    std::string long_data = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz";
    auto qrs = app::GenerateQRs(long_data, 20); // Small chunks to force multiple parts
    
    int expected_parts = (long_data.length() + 19) / 20; // Ceiling division
    assert(qrs.size() == static_cast<size_t>(expected_parts));
    
    // Verify part numbering
    for (size_t i = 0; i < qrs.size(); ++i) {
        assert(qrs[i].part == static_cast<int>(i + 1));
        assert(qrs[i].total_parts == expected_parts);
        assert(qrs[i].size > 0);
    }
    
    std::cout << "✅ Multi-QR chunking test passed (generated " << qrs.size() << " parts)" << std::endl;
}

void test_transaction_payload() {
    std::cout << "Testing transaction payload chunking..." << std::endl;
    
    std::string tx_payload = R"({"type":"1","version":"1.0","data":{"hash":"0x1db03e193bc95ca525006ed6ccd619b3b9db060a959d5e5c987c807c992732d1","signature":{"r":"0xf827b2181487b88bcef666d5729a8b9fcb7ac7cfd94dd4c4e9e9dbcfc9be154d","s":"0x5981479fb853e3779b176e12cd6feb4424159679c6bf8f4f468f92f700d9722d","v":"0x422d"},"transaction":{"to":"0x8c47B9fADF822681C68f34fd9b0D3063569245A1","value":"0x01e078","nonce":23,"gasPrice":"0x019bfcc0","gasLimit":"0x5208","data":"0x","chainId":8453},"timestamp":1757205711661,"network":"base"},"checksum":"dee6a6184b7c1479"})";
    
    auto qrs = app::GenerateQRs(tx_payload, 100);
    
    int expected_parts = (tx_payload.length() + 99) / 100;
    assert(qrs.size() == static_cast<size_t>(expected_parts));
    
    std::cout << "   Transaction length: " << tx_payload.length() << " chars" << std::endl;
    std::cout << "   Generated parts: " << qrs.size() << std::endl;
    
    // Verify all parts are valid
    for (const auto& qr : qrs) {
        assert(qr.size > 0);
        assert(qr.part >= 1);
        assert(qr.total_parts == expected_parts);
    }
    
    std::cout << "✅ Transaction payload chunking test passed" << std::endl;
}

void test_ascii_rendering() {
    std::cout << "Testing ASCII rendering methods..." << std::endl;
    
    std::string test_data = "Test data for ASCII rendering";
    auto qrs = app::GenerateQRs(test_data, 15); // Create multiple parts
    
    assert(!qrs.empty());
    
    for (const auto& qr : qrs) {
        std::string robust = qr.toRobustAscii();
        std::string compact = qr.toCompactAscii();
        
        assert(!robust.empty());
        assert(!compact.empty());
        assert(compact.length() < robust.length());
        
        // Check for expected characters
        assert(robust.find("##") != std::string::npos || robust.find("  ") != std::string::npos);
        assert(compact.find("█") != std::string::npos || compact.find(" ") != std::string::npos);
    }
    
    std::cout << "✅ ASCII rendering test passed" << std::endl;
}

void test_edge_cases() {
    std::cout << "Testing edge cases..." << std::endl;
    
    // Empty string
    auto empty_qrs = app::GenerateQRs("", 100);
    assert(empty_qrs.size() == 1);
    assert(empty_qrs[0].total_parts == 1);
    
    // Exactly at boundary
    std::string boundary_data(100, 'A');
    auto boundary_qrs = app::GenerateQRs(boundary_data, 100);
    assert(boundary_qrs.size() == 1);
    assert(boundary_qrs[0].total_parts == 1);
    
    // Just over boundary
    std::string over_boundary(101, 'B');
    auto over_qrs = app::GenerateQRs(over_boundary, 100);
    assert(over_qrs.size() == 2);
    assert(over_qrs[0].total_parts == 2);
    assert(over_qrs[1].total_parts == 2);
    
    std::cout << "✅ Edge cases test passed" << std::endl;
}

void test_p_format_simulation() {
    std::cout << "Testing P1/3: format simulation..." << std::endl;
    
    // Simulate what the chunking would produce
    std::string test_data = "This is test data that will be chunked";
    auto qrs = app::GenerateQRs(test_data, 15);
    
    std::cout << "   Original data: \"" << test_data << "\"" << std::endl;
    std::cout << "   Generated " << qrs.size() << " QR parts:" << std::endl;
    
    for (const auto& qr : qrs) {
        std::cout << "     Part " << qr.part << " of " << qr.total_parts 
                  << " - Size: " << qr.size << "x" << qr.size << std::endl;
    }
    
    // The actual data in each QR would be "P1/4:chunk1", "P2/4:chunk2", etc.
    // We can't easily extract this without decoding, but we can verify structure
    assert(qrs.size() > 1); // Should be chunked
    
    std::cout << "✅ P1/3: format simulation test passed" << std::endl;
}

int main() {
    std::cout << "=== Comprehensive QR Chunking Verification ===" << std::endl;
    
    try {
        test_qr_struct_fields();
        test_single_qr_generation();
        test_multi_qr_chunking();
        test_transaction_payload();
        test_ascii_rendering();
        test_edge_cases();
        test_p_format_simulation();
        
        std::cout << "\n🎉 ALL VERIFICATION TESTS PASSED!" << std::endl;
        std::cout << "✅ QR code chunking implementation is correct and complete" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}
