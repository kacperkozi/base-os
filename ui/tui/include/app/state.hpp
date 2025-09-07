#pragma once
#include <string>
#include <vector>
#include <optional>
#include <map>
#include <memory>
#include <mutex>
#include <atomic>
#include <stdexcept>
#include <functional>

namespace app {

// Forward declarations
class Config;
class Logger;

// Exception types for better error handling
class StateException : public std::runtime_error {
public:
    explicit StateException(const std::string& message) : std::runtime_error(message) {}
};

class ValidationException : public StateException {
public:
    explicit ValidationException(const std::string& message) : StateException("Validation error: " + message) {}
};

class ConfigurationException : public StateException {
public:
    explicit ConfigurationException(const std::string& message) : StateException("Configuration error: " + message) {}
};

// Contact types for different address categories
enum class ContactType {
  ENS,        // Ethereum Name Service
  Base,       // Base names
  Multisig,   // Multisig contracts
  Contract,   // Regular contracts
  EOA         // Externally Owned Account
};

// Known address entry for address book with validation
struct KnownAddress {
  std::string address;
  std::string name;
  std::string description;
  ContactType type = ContactType::EOA;
  
  // Validation
  bool isValid() const noexcept;
  
  // Safe construction
  static std::optional<KnownAddress> create(const std::string& address, 
                                          const std::string& name,
                                          const std::string& description = "",
                                          ContactType type = ContactType::EOA) noexcept;
  
  // Comparison for sorting/searching
  bool operator<(const KnownAddress& other) const noexcept {
    return address < other.address;
  }
  
  bool operator==(const KnownAddress& other) const noexcept {
    return address == other.address;
  }
};

// Transaction structure supporting both legacy and EIP-1559 with validation
struct UnsignedTx {
  std::string to;
  std::string value;  // in Wei
  std::string data;
  std::string nonce;
  std::string gas_limit;
  
  // Legacy transaction
  std::string gas_price;
  
  // EIP-1559 transaction
  std::string max_fee_per_gas;
  std::string max_priority_fee_per_gas;
  
  int chain_id = 8453;  // Base network
  int type = 2;  // 0=legacy, 2=EIP-1559
  
  // Helper methods
  bool isEIP1559() const noexcept { return type == 2; }
  bool isEmpty() const noexcept { return to.empty() && value.empty(); }
  
  // Validation and bounds checking
  bool isValid() const noexcept;
  std::vector<std::string> getValidationErrors() const noexcept;
  
  // Safe numeric operations with overflow protection
  bool setValueFromString(const std::string& value_str) noexcept;
  bool setNonceFromString(const std::string& nonce_str) noexcept;
  bool setGasLimitFromString(const std::string& gas_limit_str) noexcept;
  
  // Clear all fields safely
  void clear() noexcept {
    to.clear();
    value.clear();
    data = "0x";
    nonce.clear();
    gas_limit.clear();
    gas_price.clear();
    max_fee_per_gas.clear();
    max_priority_fee_per_gas.clear();
    chain_id = 8453;
    type = 2;
  }
  
  // Create from configuration defaults
  static UnsignedTx createFromDefaults(int chain_id = 8453, bool use_eip1559 = true) noexcept;
};

// Hardware wallet device info with validation
struct DeviceInfo {
  std::string model;
  std::string path;  // USB path
  bool connected = false;
  bool app_open = false;  // Ethereum app open
  std::string version;    // Firmware version
  std::string serial;     // Device serial number (anonymized)
  
  // Validation and safety
  bool isValid() const noexcept {
    return !model.empty() && !path.empty() && 
           model.size() <= 100 && path.size() <= 255;
  }
  
  // Safe comparison
  bool operator==(const DeviceInfo& other) const noexcept {
    return path == other.path && model == other.model;
  }
};

// Workflow screens matching PRD flow
enum class Route {
  ConnectWallet,     // Connect hardware wallet screen
  USBContacts,       // USB contacts screen
  TransactionInput,  // Enter transaction details
  Confirmation,      // Review before signing
  Signing,          // Signing in progress
  Result,           // Show signed transaction & QR
  
  // Additional screens
  Settings,
  Help,
  Error,
};

// Thread-safe Application state with improved error handling
class AppState {
public:
  // Public members for backward compatibility with views_new.cpp
  // These maintain the original struct interface while adding safety features
  
  // Core state
  Route route = Route::ConnectWallet;
  Route previous_route = Route::ConnectWallet;
  
  // Transaction data
  UnsignedTx unsigned_tx;
  std::atomic<bool> has_unsigned{false};
  std::string signed_hex;
  std::atomic<bool> has_signed{false};
  
  // UI state
  std::string status;
  std::string error;
  std::string info;
  std::string last_error;
  
  // Hardware wallet
  std::string account_path = "m/44'/60'/0'/0/0";
  std::atomic<int> selected_device{0};
  std::vector<DeviceInfo> devices;
  std::atomic<bool> wallet_connected{false};
  
  // Address book & USB contacts (with size limits for security)
  std::vector<KnownAddress> known_addresses;
  std::vector<KnownAddress> usb_contacts;
  std::string address_suggestion;
  std::atomic<int> selected_contact{0};
  std::atomic<bool> is_scanning_usb{false};
  std::atomic<bool> usb_scan_complete{false};
  
  // Animation state
  std::atomic<int> animation_frame{0};
  std::atomic<bool> is_signing{false};
  std::atomic<bool> is_detecting_wallet{false};
  
  // Settings
  std::atomic<bool> use_eip1559{true};
  std::atomic<bool> show_wei{false};
  std::string network_name = "Base";
  
  // Input validation state
  std::map<std::string, std::string> field_errors;

private:
  // Private members for internal state management
  mutable std::mutex mutex_;  // Thread safety
  std::atomic<bool> shutdown_requested_{false};
  
  // Error recovery state
  std::chrono::system_clock::time_point last_error_time_;
  int error_count_ = 0;
  
  // State change callbacks
  std::vector<std::function<void(Route, Route)>> route_change_callbacks_;
  std::vector<std::function<void(const std::string&)>> error_callbacks_;

public:
  AppState() = default;
  ~AppState() = default;
  
  // Copy prevention for thread safety
  AppState(const AppState&) = delete;
  AppState& operator=(const AppState&) = delete;
  
  // Safe getters with locking
  Route getRoute() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return route;
  }
  
  Route getPreviousRoute() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return previous_route;
  }
  
  UnsignedTx getUnsignedTx() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return unsigned_tx;
  }
  
  std::string getSignedHex() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return signed_hex;
  }
  
  std::string getStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return status;
  }
  
  std::string getError() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return error;
  }
  
  std::string getInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return info;
  }
  
  std::string getLastError() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_error;
  }
  
  std::vector<DeviceInfo> getDevices() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return devices;
  }
  
  std::vector<KnownAddress> getKnownAddresses() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return known_addresses;
  }
  
  std::vector<KnownAddress> getUsbContacts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return usb_contacts;
  }
  
  std::string getAddressSuggestion() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return address_suggestion;
  }
  
  std::map<std::string, std::string> getFieldErrors() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return field_errors;
  }
  
  std::string getNetworkName() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return network_name;
  }
  
  std::string getAccountPath() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return account_path;
  }
  
  // Atomic getters (no locking needed)
  bool hasUnsigned() const noexcept { return has_unsigned.load(); }
  bool hasSigned() const noexcept { return has_signed.load(); }
  int getSelectedDevice() const noexcept { return selected_device.load(); }
  bool isWalletConnected() const noexcept { return wallet_connected.load(); }
  int getSelectedContact() const noexcept { return selected_contact.load(); }
  bool isScanningUsb() const noexcept { return is_scanning_usb.load(); }
  bool isUsbScanComplete() const noexcept { return usb_scan_complete.load(); }
  int getAnimationFrame() const noexcept { return animation_frame.load(); }
  bool isSigning() const noexcept { return is_signing.load(); }
  bool isDetectingWallet() const noexcept { return is_detecting_wallet.load(); }
  bool useEip1559() const noexcept { return use_eip1559.load(); }
  bool showWei() const noexcept { return show_wei.load(); }
  bool isShutdownRequested() const noexcept { return shutdown_requested_.load(); }
  
  // Safe setters with validation and locking
  bool setRoute(Route new_route) noexcept;
  bool setUnsignedTx(const UnsignedTx& tx) noexcept;
  bool setSignedHex(const std::string& hex) noexcept;
  void setStatus(const std::string& status) noexcept;
  void setError(const std::string& error) noexcept;
  void setInfo(const std::string& info) noexcept;
  bool setDevices(const std::vector<DeviceInfo>& devices) noexcept;
  bool addKnownAddress(const KnownAddress& address) noexcept;
  bool setKnownAddresses(const std::vector<KnownAddress>& addresses) noexcept;
  bool setUsbContacts(const std::vector<KnownAddress>& contacts) noexcept;
  void setAddressSuggestion(const std::string& suggestion) noexcept;
  void setFieldErrors(const std::map<std::string, std::string>& errors) noexcept;
  void setNetworkName(const std::string& name) noexcept;
  bool setAccountPath(const std::string& path) noexcept;
  
  // Atomic setters
  void setHasUnsigned(bool value) noexcept { has_unsigned.store(value); }
  void setHasSigned(bool value) noexcept { has_signed.store(value); }
  void setSelectedDevice(int index) noexcept;
  void setWalletConnected(bool connected) noexcept { wallet_connected.store(connected); }
  void setSelectedContact(int index) noexcept;
  void setScanningUsb(bool scanning) noexcept { is_scanning_usb.store(scanning); }
  void setUsbScanComplete(bool complete) noexcept { usb_scan_complete.store(complete); }
  void setAnimationFrame(int frame) noexcept { animation_frame.store(frame); }
  void setSigning(bool signing) noexcept { is_signing.store(signing); }
  void setDetectingWallet(bool detecting) noexcept { is_detecting_wallet.store(detecting); }
  void setUseEip1559(bool value) noexcept { use_eip1559.store(value); }
  void setShowWei(bool value) noexcept { show_wei.store(value); }
  void requestShutdown() noexcept { shutdown_requested_.store(true); }
  
  // Validation and error handling
  bool isStateValid() const noexcept;
  std::vector<std::string> getStateValidationErrors() const noexcept;
  
  // Cleanup methods
  void clearTransaction() noexcept;
  void clearError() noexcept;
  void clearAll() noexcept;
  
  // Error recovery
  void resetErrorState() noexcept;
  bool isInErrorRecovery() const noexcept;
  
  // Callback management
  void addRouteChangeCallback(std::function<void(Route, Route)> callback);
  void addErrorCallback(std::function<void(const std::string&)> callback);
  void clearCallbacks() noexcept;
  
  // Utility methods
  size_t getKnownAddressCount() const noexcept;
  size_t getUsbContactCount() const noexcept;
  bool hasFieldError(const std::string& field) const noexcept;
  void incrementAnimationFrame() noexcept { animation_frame.fetch_add(1); }
  
  // Configuration integration
  bool loadFromConfig() noexcept;
  bool saveToConfig() const noexcept;
};

} // namespace app
