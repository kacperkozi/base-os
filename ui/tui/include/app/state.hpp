
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
#include <chrono>
#include "qr_generator.hpp"
#include "qrcodegen.hpp" // Include for Ecc enum

namespace app {

// Forward declarations
class Config;
class Logger;

// --- Domain Objects ---

// Exception types for better error handling
class StateException : public std::runtime_error {
public:
    explicit StateException(const std::string& message) : std::runtime_error(message) {}
};

// Contact types for different address categories
enum class ContactType {
  ENS, EOA, Multisig, Contract, Base
};

// Known address entry for address book with validation
struct KnownAddress {
  std::string address;
  std::string name;
  std::string description;
  ContactType type = ContactType::EOA;
  
  bool isValid() const noexcept;
  static std::optional<KnownAddress> create(const std::string& address, 
                                          const std::string& name,
                                          const std::string& description = "",
                                          ContactType type = ContactType::EOA) noexcept;
  bool operator<(const KnownAddress& other) const noexcept { return address < other.address; }
  bool operator==(const KnownAddress& other) const noexcept { return address == other.address; }
};

// Transaction structure supporting both legacy and EIP-1559 with validation
struct UnsignedTx {
  std::string to;
  std::string value;  // in Wei
  std::string data;
  std::string nonce;
  std::string gas_limit;
  std::string gas_price; // Legacy
  std::string max_fee_per_gas; // EIP-1559
  std::string max_priority_fee_per_gas; // EIP-1559
  int chain_id = 8453;
  int type = 2; // 0=legacy, 2=EIP-1559
  
  bool isEIP1559() const noexcept { return type == 2; }
  bool isEmpty() const noexcept { return to.empty() && value.empty(); }
  bool isValid() const noexcept;
  std::vector<std::string> getValidationErrors() const noexcept;
  void clear() noexcept;
  static UnsignedTx createFromDefaults(int chain_id = 8453, bool use_eip1559 = true) noexcept;
};

// Hardware wallet device info with validation
struct DeviceInfo {
  std::string model;
  std::string path;
  bool connected = false;
  bool app_open = false;
  std::string version;
  std::string serial;
  
  bool isValid() const noexcept;
  bool operator==(const DeviceInfo& other) const noexcept { return path == other.path && model == other.model; }
};

// --- State Management ---

// Workflow screens matching PRD flow
enum class Route {
  ConnectWallet, USBContacts, TransactionInput, Confirmation, Signing, Result, Settings, Help, Error
};

// Focused state structures for better separation of concerns
struct UIState {
  Route route = Route::ConnectWallet;
  Route previous_route = Route::ConnectWallet;
  std::string status;
  std::string error;
  std::string info;
  std::map<std::string, std::string> field_errors;
  std::string address_suggestion;
  int animation_frame = 0;
  int selected_device = 0;
  int selected_contact = 0;
  bool show_wei = false;
  bool is_signing = false;
  bool is_detecting_wallet = false;
  bool is_scanning_usb = false;
  bool usb_scan_complete = false;
  bool edit_mode = false;
  bool dev_mode = false; // Mock wallet mode
};

struct TransactionState {
  UnsignedTx unsigned_tx;
  std::string signed_hex;
  std::vector<app::QRCode> qr_codes;
  bool use_eip1559 = true;
  std::string network_name = "Base";
};

struct DeviceState {
  std::vector<DeviceInfo> devices;
  bool wallet_connected = false;
  std::string account_path = "m/44'/60'/0'/0/0";
  std::vector<KnownAddress> known_addresses;
  std::vector<KnownAddress> usb_contacts;
};

// Thread-safe Application state composing focused state objects
class AppState {
private:
  mutable std::mutex mutex_;
  UIState ui_;
  TransactionState tx_;
  DeviceState device_;
  std::atomic<bool> shutdown_requested_{false};

public:
  AppState();
  ~AppState() = default;
  AppState(const AppState&) = delete;
  AppState& operator=(const AppState&) = delete;

  // --- Thread-Safe State Accessors ---

  // Getters for entire sub-states (for read-only operations)
  UIState getUIState() const;
  TransactionState getTransactionState() const;
  DeviceState getDeviceState() const;

  // Getters for individual properties
  Route getRoute() const;
  UnsignedTx getUnsignedTx() const;
  bool hasUnsignedTx() const;
  std::string getSignedHex() const;
  bool hasSignedTx() const;
  std::vector<app::QRCode> getQRCodes() const;

  // Setters for state modification
  void setRoute(Route new_route);
  void setUnsignedTx(const UnsignedTx& tx);
  void setSignedHex(const std::string& hex, qrcodegen::QrCode::Ecc ecl = qrcodegen::QrCode::Ecc::QUARTILE);
  void setDevices(const std::vector<DeviceInfo>& devices);
  void setWalletConnected(bool connected);
  void setUsbContacts(const std::vector<KnownAddress>& contacts);
  void addKnownAddress(const KnownAddress& address);
  void setStatus(const std::string& status);
  void setInfo(const std::string& info);
  void setError(const std::string& error, const std::map<std::string, std::string>& field_errors = {});
  void setScanningUsb(bool scanning);
  void setUsbScanComplete(bool complete);
  void setSigning(bool signing);
  void setSelectedContact(int index);
  void setDetectingWallet(bool detecting);
  void setEditMode(bool edit);
  void toggleEditMode();
  void setUseEIP1559(bool use_it);
  void setShowWei(bool show_it);
  void setDevMode(bool enabled);

  // --- UI & Animation ---
  void incrementAnimationFrame();
  int getAnimationFrame() const;

  // --- Cleanup & Lifecycle ---
  void clearTransaction();
  void clearError();
  void clearAll();
  void requestShutdown();
  bool isShutdownRequested() const;

  // --- Configuration ---
  bool loadFromConfig();
  bool saveToConfig() const;
};

}
