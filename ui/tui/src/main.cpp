#include <cstdlib>
#include <iostream>
#include <string>

int RunHelloWorld();         // Ultra-minimal test (hello_world.cpp)
int RunSimpleTransaction();  // Comprehensive transaction app (simple_transaction.cpp)
int RunWalletDetectionApp(); // Wallet detection with polling (main_with_wallet_detection.cpp)

int main(int argc, char* argv[]){ 
  // Handle command-line arguments
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--version" || arg == "-v") {
      std::cout << "Base OS TUI v1.0.0" << std::endl;
      std::cout << "Simple Ethereum transaction interface" << std::endl;
      return 0;
    } else if (arg == "--help" || arg == "-h") {
      std::cout << "Base OS TUI - Simple Ethereum Transaction Interface" << std::endl;
      std::cout << std::endl;
      std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
      std::cout << std::endl;
      std::cout << "Options:" << std::endl;
      std::cout << "  --version, -v    Show version information" << std::endl;
      std::cout << "  --help, -h       Show this help message" << std::endl;
      std::cout << std::endl;
      std::cout << "Controls:" << std::endl;
      std::cout << "  Tab              Navigate between fields" << std::endl;
      std::cout << "  Enter            Submit/Continue" << std::endl;
      std::cout << "  Escape           Go back" << std::endl;
      std::cout << "  Ctrl+C           Quit" << std::endl;
      return 0;
    }
  }
  
  // Use the original comprehensive interface with integrated wallet detection
  return RunSimpleTransaction(); 
}
