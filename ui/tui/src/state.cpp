
#include "app/state.hpp"
#include "app/config.hpp"
#include "app/logger.hpp"
#include "app/validation.hpp"
#include <algorithm>
#include <regex>
#include <limits>
#include <chrono>
#include <iostream>

namespace app {

// --- Domain Object Implementations ---

bool KnownAddress::isValid() const noexcept {
    return Validator::isValidKnownAddress(*this);
}

std::optional<KnownAddress> KnownAddress::create(const std::string& address, 
                                                const std::string& name,
                                                const std::string& description,
                                                ContactType type) noexcept {
    KnownAddress addr{address, name, description, type};
    if (addr.isValid()) {
        return addr;
    }
    return std::nullopt;
}

bool UnsignedTx::isValid() const noexcept {
    return Validator::validateTransaction(*this).empty();
}

std::vector<std::string> UnsignedTx::getValidationErrors() const noexcept {
    return Validator::validateTransaction(*this);
}

void UnsignedTx::clear() noexcept {
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

UnsignedTx UnsignedTx::createFromDefaults(int chain_id, bool use_eip1559) noexcept {
    UnsignedTx tx;
    tx.chain_id = chain_id;
    tx.type = use_eip1559 ? 2 : 0;
    tx.gas_limit = "21000";
    tx.data = "0x";
    if (use_eip1559) {
        tx.max_fee_per_gas = "50";
        tx.max_priority_fee_per_gas = "2";
    } else {
        tx.gas_price = "20";
    }
    return tx;
}

bool DeviceInfo::isValid() const noexcept {
    return !model.empty() && !path.empty() && 
           model.size() <= 100 && path.size() <= 255;
}

// --- AppState Implementation ---

AppState::AppState() {
    loadFromConfig();
}

// Getters for entire sub-states
UIState AppState::getUIState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ui_;
}

TransactionState AppState::getTransactionState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tx_;
}

DeviceState AppState::getDeviceState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return device_;
}

// Getters for individual properties
Route AppState::getRoute() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ui_.route;
}

UnsignedTx AppState::getUnsignedTx() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tx_.unsigned_tx;
}

bool AppState::hasUnsignedTx() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !tx_.unsigned_tx.isEmpty();
}

std::string AppState::getSignedHex() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tx_.signed_hex;
}

bool AppState::hasSignedTx() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !tx_.signed_hex.empty();
}

std::vector<app::QRCode> AppState::getQRCodes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tx_.qr_codes;
}

// Setters for state modification
void AppState::setRoute(Route new_route) {
    std::lock_guard<std::mutex> lock(mutex_);
    ui_.previous_route = ui_.route;
    ui_.route = new_route;
    LOG_DEBUG("Route changed to " + std::to_string(static_cast<int>(new_route)));
}

void AppState::setUnsignedTx(const UnsignedTx& tx) {
    std::lock_guard<std::mutex> lock(mutex_);
    tx_.unsigned_tx = tx;
}

void AppState::setSignedHex(const std::string& hex, qrcodegen::QrCode::Ecc ecl) {
    std::lock_guard<std::mutex> lock(mutex_);
    tx_.signed_hex = hex;
    if (hex.empty()) {
        tx_.qr_codes.clear();
        return;
    }

    try {
        // Convert hex string to raw bytes for efficient encoding
        std::vector<uint8_t> binary_data = hex_to_bytes(hex);
        // Generate QR codes with the specified error correction level
        tx_.qr_codes = GenerateQRs(binary_data, 100, ecl);
        LOG_INFO("Generated " + std::to_string(tx_.qr_codes.size()) + " QR code parts.");
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to generate QR codes: " + std::string(e.what()));
        tx_.qr_codes.clear();
        ui_.error = "Failed to generate QR codes.";
    }
}

void AppState::setDevices(const std::vector<DeviceInfo>& devices) {
    std::lock_guard<std::mutex> lock(mutex_);
    device_.devices = devices;
}

void AppState::setWalletConnected(bool connected) {
    std::lock_guard<std::mutex> lock(mutex_);
    device_.wallet_connected = connected;
}

void AppState::setUsbContacts(const std::vector<KnownAddress>& contacts) {
    std::lock_guard<std::mutex> lock(mutex_);
    device_.usb_contacts = contacts;
}

void AppState::addKnownAddress(const KnownAddress& address) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Prevent duplicates
    auto it = std::find(device_.known_addresses.begin(), device_.known_addresses.end(), address);
    if (it == device_.known_addresses.end()) {
        device_.known_addresses.push_back(address);
    }
}

void AppState::setStatus(const std::string& status) {
    std::lock_guard<std::mutex> lock(mutex_);
    ui_.status = status;
}

void AppState::setInfo(const std::string& info) {
    std::lock_guard<std::mutex> lock(mutex_);
    ui_.info = info;
}

void AppState::setError(const std::string& error, const std::map<std::string, std::string>& field_errors) {
    std::lock_guard<std::mutex> lock(mutex_);
    ui_.error = error;
    ui_.field_errors = field_errors;
    LOG_ERROR("Application error: " + error);
}

void AppState::setScanningUsb(bool scanning) {
    std::lock_guard<std::mutex> lock(mutex_);
    ui_.is_scanning_usb = scanning;
}

void AppState::setUsbScanComplete(bool complete) {
    std::lock_guard<std::mutex> lock(mutex_);
    ui_.usb_scan_complete = complete;
}

void AppState::setSigning(bool signing) {
    std::lock_guard<std::mutex> lock(mutex_);
    ui_.is_signing = signing;
}

void AppState::setSelectedContact(int index) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (index >= 0 && index < static_cast<int>(device_.usb_contacts.size())) {
        ui_.selected_contact = index;
    }
}

void AppState::setDetectingWallet(bool detecting) {
    std::lock_guard<std::mutex> lock(mutex_);
    ui_.is_detecting_wallet = detecting;
}

void AppState::setEditMode(bool edit) {
    std::lock_guard<std::mutex> lock(mutex_);
    ui_.edit_mode = edit;
}

void AppState::toggleEditMode() {
    std::lock_guard<std::mutex> lock(mutex_);
    ui_.edit_mode = !ui_.edit_mode;
}

void AppState::setUseEIP1559(bool use_it) {
    std::lock_guard<std::mutex> lock(mutex_);
    tx_.use_eip1559 = use_it;
}

void AppState::setShowWei(bool show_it) {
    std::lock_guard<std::mutex> lock(mutex_);
    ui_.show_wei = show_it;
}

void AppState::setDevMode(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    ui_.dev_mode = enabled;
    
    // In dev mode, start directly on USBContacts screen to avoid auto-navigation timing issues
    if (enabled) {
        ui_.previous_route = ui_.route;
        ui_.route = Route::USBContacts;
        LOG_DEBUG("Dev mode enabled - starting on USBContacts screen");
    }
}

// UI & Animation
void AppState::incrementAnimationFrame() {
    std::lock_guard<std::mutex> lock(mutex_);
    ui_.animation_frame++;
}

int AppState::getAnimationFrame() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ui_.animation_frame;
}

// Cleanup & Lifecycle
void AppState::clearTransaction() {
    std::lock_guard<std::mutex> lock(mutex_);
    tx_.unsigned_tx.clear();
    tx_.signed_hex.clear();
    tx_.qr_codes.clear();
    ui_.field_errors.clear();
}

void AppState::clearError() {
    std::lock_guard<std::mutex> lock(mutex_);
    ui_.error.clear();
    ui_.field_errors.clear();
}

void AppState::clearAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    ui_ = {};
    tx_ = {};
    device_ = {};
    loadFromConfig(); // Reload defaults
}

void AppState::requestShutdown() {
    shutdown_requested_.store(true);
}

bool AppState::isShutdownRequested() const {
    return shutdown_requested_.load();
}

// Configuration
bool AppState::loadFromConfig() {
    try {
        const auto& config = Config::getInstance();
        const auto& app_config = config.getAppConfig();
        const auto& network_config = config.getNetworkConfig();
        
        std::lock_guard<std::mutex> lock(mutex_);
        tx_.network_name = network_config.name;
        tx_.use_eip1559 = network_config.use_eip1559;
        tx_.unsigned_tx = UnsignedTx::createFromDefaults(network_config.chain_id, network_config.use_eip1559);
        ui_.show_wei = app_config.show_amounts_in_wei;
        device_.account_path = app_config.preferred_wallet_path;
        
        LOG_INFO("State loaded from configuration");
        return true;
    } catch (...) {
        LOG_ERROR("Failed to load state from configuration");
        return false;
    }
}

bool AppState::saveToConfig() const {
    try {
        auto& config = Config::getInstance();
        auto app_config = config.getAppConfig();
        
        std::lock_guard<std::mutex> lock(mutex_);
        app_config.show_amounts_in_wei = ui_.show_wei;
        app_config.preferred_wallet_path = device_.account_path;
        config.setAppConfig(app_config);
        
        return config.save();
    } catch (...) {
        LOG_ERROR("Failed to save state to configuration");
        return false;
    }
}

} // namespace app
