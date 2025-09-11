#pragma once
#include <string>
#include <vector>
#include <optional>
#include <map>
#include "qr_generator.hpp"

namespace app {

// Contact types for address book
enum class ContactType {
  ENS, EOA, Multisig, Contract, Base
};

// Known address entry for address book
struct KnownAddress {
  std::string address;
  std::string name;
  std::string description;
  ContactType type = ContactType::EOA;
  
  bool operator<(const KnownAddress& other) const noexcept { return address < other.address; }
  bool operator==(const KnownAddress& other) const noexcept { return address == other.address; }
};

// Hardware wallet device info
struct DeviceInfo {
  std::string model;
  std::string path;
  bool connected = false;
  bool app_open = false;
  std::string version;
  std::string serial;
  
  bool operator==(const DeviceInfo& other) const noexcept { return path == other.path && model == other.model; }
};

// Workflow screens matching PRD flow
enum class Route {
  ConnectWallet, USBContacts, TransactionInput, Confirmation, Signing, Result, Settings, Help, Error
};

// Transaction data structure
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
  
  static UnsignedTx createFromDefaults(int chain_id, bool use_eip1559) {
    UnsignedTx tx;
    tx.chain_id = chain_id;
    tx.type = use_eip1559 ? 2 : 0;
    tx.gas_limit = "21000";
    if (use_eip1559) {
      tx.max_fee_per_gas = "2000000000"; // 2 gwei
      tx.max_priority_fee_per_gas = "1000000000"; // 1 gwei
    } else {
      tx.gas_price = "2000000000"; // 2 gwei
    }
    return tx;
  }
  
  void clear() noexcept {
    to.clear();
    value.clear();
    data.clear();
    nonce.clear();
    gas_limit.clear();
    gas_price.clear();
    max_fee_per_gas.clear();
    max_priority_fee_per_gas.clear();
    chain_id = 8453;
    type = 2;
  }
};

// UI-related state
struct UIState {
  // Navigation
  Route route = Route::ConnectWallet;
  Route previous_route = Route::ConnectWallet;
  
  // Messages
  std::string status;
  std::string error;
  std::string info;
  std::string last_error;
  
  // UI feedback
  std::map<std::string, std::string> field_errors;
  std::string address_suggestion;
  
  // Animation and progress
  int animation_frame = 0;
  bool is_signing = false;
  bool is_detecting_wallet = false;
  bool is_scanning_usb = false;
  bool usb_scan_complete = false;
  
  // Selection indices
  int selected_device = 0;
  int selected_contact = 0;
  
  // Settings
  bool show_wei = false;
  
  void clear() noexcept {
    route = Route::ConnectWallet;
    previous_route = Route::ConnectWallet;
    status.clear();
    error.clear();
    info.clear();
    last_error.clear();
    field_errors.clear();
    address_suggestion.clear();
    animation_frame = 0;
    is_signing = false;
    is_detecting_wallet = false;
    is_scanning_usb = false;
    usb_scan_complete = false;
    selected_device = 0;
    selected_contact = 0;
    show_wei = false;
  }
};

// Transaction-related state
struct TransactionState {
  // Transaction data
  UnsignedTx unsigned_tx;
  bool has_unsigned = false;
  std::string signed_hex;
  bool has_signed = false;
  bool signing = false;
  std::vector<app::QRCode> qr_codes;
  
  // Transaction settings
  bool use_eip1559 = true;
  std::string network_name = "Base";
  
  void clear() noexcept {
    unsigned_tx.clear();
    has_unsigned = false;
    signed_hex.clear();
    has_signed = false;
    signing = false;
    qr_codes.clear();
  }
  
  void clearSigned() noexcept {
    signed_hex.clear();
    has_signed = false;
    qr_codes.clear();
  }
};

// Device and wallet-related state
struct DeviceState {
  // Hardware wallet
  std::vector<DeviceInfo> devices;
  bool wallet_connected = false;
  bool detecting_wallet = false;
  bool scanning_usb = false;
  std::string account_path = "m/44'/60'/0'/0/0";
  
  // Address book & USB contacts
  std::vector<KnownAddress> known_addresses;
  std::vector<KnownAddress> usb_contacts;
  
  void clear() noexcept {
    devices.clear();
    wallet_connected = false;
    detecting_wallet = false;
    scanning_usb = false;
    account_path = "m/44'/60'/0'/0/0";
    known_addresses.clear();
    usb_contacts.clear();
  }
  
  void clearContacts() noexcept {
    usb_contacts.clear();
  }
  
  size_t getDeviceCount() const noexcept {
    return devices.size();
  }
  
  size_t getKnownAddressCount() const noexcept {
    return known_addresses.size();
  }
  
  size_t getUsbContactCount() const noexcept {
    return usb_contacts.size();
  }
};

} // namespace app