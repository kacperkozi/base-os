#include "app/qr_generator.hpp"
#include <iostream>

int main() {
    // Test with your hardcoded transaction payload
    std::string signed_tx_payload = R"({"type":"1","version":"1.0","data":{"hash":"0x1db03e193bc95ca525006ed6ccd619b3b9db060a959d5e5c987c807c992732d1","signature":{"r":"0xf827b2181487b88bcef666d5729a8b9fcb7ac7cfd94dd4c4e9e9dbcfc9be154d","s":"0x5981479fb853e3779b176e12cd6feb4424159679c6bf8f4f468f92f700d9722d","v":"0x422d"},"transaction":{"to":"0x8c47B9fADF822681C68f34fd9b0D3063569245A1","value":"0x01e078","nonce":23,"gasPrice":"0x019bfcc0","gasLimit":"0x5208","data":"0x","chainId":8453},"timestamp":1757205711661,"network":"base"},"checksum":"dee6a6184b7c1479"})";
    
    std::cout << "Generating QR code for transaction payload..." << std::endl;
    std::cout << "Payload length: " << signed_tx_payload.length() << " characters" << std::endl;
    std::cout << std::endl;
    
    app::QRCode qr = app::GenerateQR(signed_tx_payload);
    
    std::cout << "QR Code (Robust ASCII representation):" << std::endl;
    std::cout << qr.toRobustAscii() << std::endl;
    
    std::cout << "QR Code (Compact ASCII representation):" << std::endl;
    std::cout << qr.toCompactAscii() << std::endl;
    
    return 0;
}
