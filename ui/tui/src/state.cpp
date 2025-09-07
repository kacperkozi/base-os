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

// KnownAddress implementation
bool KnownAddress::isValid() const noexcept {
    try {
        return !address.empty() && 
               !name.empty() && 
               address.size() <= 42 &&  // Ethereum address max length
               name.size() <= 100 &&
               description.size() <= 500 &&
               IsAddress(address);
    } catch (...) {
        return false;
    }
}

std::optional<KnownAddress> KnownAddress::create(const std::string& address, 
                                                const std::string& name,
                                                const std::string& description,
                                                ContactType type) noexcept {
    try {
        KnownAddress addr{address, name, description, type};
        if (addr.isValid()) {
            return addr;
        }
        return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}

// UnsignedTx implementation
bool UnsignedTx::isValid() const noexcept {
    try {
        return !to.empty() && 
               IsAddress(to) &&
               !value.empty() && 
               IsNumeric(value) &&
               !nonce.empty() && 
               IsNumeric(nonce) &&
               !gas_limit.empty() && 
               IsNumeric(gas_limit) &&
               chain_id > 0 &&
               (type == 0 || type == 2) &&
               data.size() <= 1000000; // Reasonable data size limit
    } catch (...) {
        return false;
    }
}

std::vector<std::string> UnsignedTx::getValidationErrors() const noexcept {
    std::vector<std::string> errors;
    
    try {
        if (to.empty()) {
            errors.push_back("Recipient address is required");
        } else if (!IsAddress(to)) {
            errors.push_back("Invalid recipient address format");
        }
        
        if (value.empty()) {
            errors.push_back("Transaction value is required");
        } else if (!IsNumeric(value)) {
            errors.push_back("Transaction value must be numeric");
        } else {
            // Check for reasonable value limits
            try {
                auto val = std::stoull(value);
                if (val > std::numeric_limits<uint64_t>::max() / 2) {
                    errors.push_back("Transaction value is too large");
                }
            } catch (...) {
                errors.push_back("Transaction value is invalid");
            }
        }
        
        if (nonce.empty()) {
            errors.push_back("Transaction nonce is required");
        } else if (!IsNumeric(nonce)) {
            errors.push_back("Nonce must be numeric");
        }
        
        if (gas_limit.empty()) {
            errors.push_back("Gas limit is required");
        } else if (!IsNumeric(gas_limit)) {
            errors.push_back("Gas limit must be numeric");
        } else {
            try {
                auto gas = std::stoull(gas_limit);
                if (gas < 21000) {
                    errors.push_back("Gas limit too low (minimum 21000)");
                } else if (gas > 30000000) {
                    errors.push_back("Gas limit too high (maximum 30M)");
                }
            } catch (...) {
                errors.push_back("Gas limit value is invalid");
            }
        }
        
        if (chain_id <= 0) {
            errors.push_back("Invalid chain ID");
        }
        
        if (type != 0 && type != 2) {
            errors.push_back("Transaction type must be 0 (legacy) or 2 (EIP-1559)");
        }
        
        if (isEIP1559()) {
            if (max_fee_per_gas.empty() || !IsNumeric(max_fee_per_gas)) {
                errors.push_back("Max fee per gas is required for EIP-1559");
            }
            if (max_priority_fee_per_gas.empty() || !IsNumeric(max_priority_fee_per_gas)) {
                errors.push_back("Priority fee is required for EIP-1559");
            }
        } else {
            if (gas_price.empty() || !IsNumeric(gas_price)) {
                errors.push_back("Gas price is required for legacy transactions");
            }
        }
        
        if (!data.empty() && !IsHex(data)) {
            errors.push_back("Transaction data must be valid hex");
        }
        
        if (data.size() > 1000000) {
            errors.push_back("Transaction data too large (>1MB)");
        }
        
    } catch (...) {
        errors.push_back("Validation error occurred");
    }
    
    return errors;
}

bool UnsignedTx::setValueFromString(const std::string& value_str) noexcept {
    try {
        if (value_str.empty() || !IsNumeric(value_str)) {
            return false;
        }
        
        // Check for overflow
        auto val = std::stoull(value_str);
        if (val > std::numeric_limits<uint64_t>::max() / 2) {
            return false;
        }
        
        value = value_str;
        return true;
    } catch (...) {
        return false;
    }
}

bool UnsignedTx::setNonceFromString(const std::string& nonce_str) noexcept {
    try {
        if (nonce_str.empty() || !IsNumeric(nonce_str)) {
            return false;
        }
        
        auto nonce_val = std::stoull(nonce_str);
        if (nonce_val > std::numeric_limits<uint32_t>::max()) {
            return false;
        }
        
        nonce = nonce_str;
        return true;
    } catch (...) {
        return false;
    }
}

bool UnsignedTx::setGasLimitFromString(const std::string& gas_limit_str) noexcept {
    try {
        if (gas_limit_str.empty() || !IsNumeric(gas_limit_str)) {
            return false;
        }
        
        auto gas = std::stoull(gas_limit_str);
        if (gas < 21000 || gas > 30000000) {
            return false;
        }
        
        gas_limit = gas_limit_str;
        return true;
    } catch (...) {
        return false;
    }
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

// AppState implementation
bool AppState::setRoute(Route new_route) noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        
        Route old_route = route;
        previous_route = route;
        route = new_route;
        
        // Notify callbacks
        for (const auto& callback : route_change_callbacks_) {
            try {
                callback(old_route, new_route);
            } catch (...) {
                // Ignore callback errors
            }
        }
        
        LOG_DEBUG("Route changed from " + std::to_string(static_cast<int>(old_route)) + 
                 " to " + std::to_string(static_cast<int>(new_route)));
        
        return true;
    } catch (...) {
        LOG_ERROR("Failed to set route");
        return false;
    }
}

bool AppState::setUnsignedTx(const UnsignedTx& tx) noexcept {
    try {
        if (!tx.isValid()) {
            LOG_WARN("Attempted to set invalid transaction");
            return false;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        unsigned_tx = tx;
        has_unsigned.store(true);
        
        LOG_DEBUG("Transaction set successfully");
        return true;
    } catch (...) {
        LOG_ERROR("Failed to set unsigned transaction");
        return false;
    }
}

bool AppState::setSignedHex(const std::string& hex) noexcept {
    try {
        if (hex.empty() || !IsHex(hex)) {
            LOG_WARN("Invalid signed transaction hex");
            return false;
        }
        
        if (hex.length() > 100000) {  // Reasonable limit
            LOG_WARN("Signed transaction hex too large");
            return false;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        signed_hex = hex;
        has_signed.store(true);
        
        LOG_INFO("Signed transaction set successfully");
        return true;
    } catch (...) {
        LOG_ERROR("Failed to set signed hex");
        return false;
    }
}

void AppState::setStatus(const std::string& status_str) noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        status = status_str.substr(0, 1000);  // Limit length
        LOG_DEBUG("Status updated: " + status);
    } catch (...) {
        LOG_ERROR("Failed to set status");
    }
}

void AppState::setError(const std::string& error_str) noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        error = error_str.substr(0, 1000);  // Limit length
        last_error = error;
        last_error_time_ = std::chrono::system_clock::now();
        error_count_++;
        
        // Notify error callbacks
        for (const auto& callback : error_callbacks_) {
            try {
                callback(error);
            } catch (...) {
                // Ignore callback errors
            }
        }
        
        LOG_ERROR("Application error: " + error);
    } catch (...) {
        // Last resort - try to log to console
        try {
            std::cerr << "Failed to set error: " << error_str << std::endl;
        } catch (...) {
            // Give up silently
        }
    }
}

void AppState::setInfo(const std::string& info_str) noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        info = info_str.substr(0, 1000);  // Limit length
        LOG_INFO("Info message: " + info);
    } catch (...) {
        LOG_ERROR("Failed to set info message");
    }
}

bool AppState::setDevices(const std::vector<DeviceInfo>& device_list) noexcept {
    try {
        // Validate all devices
        for (const auto& device : device_list) {
            if (!device.isValid()) {
                LOG_WARN("Invalid device in device list");
                return false;
            }
        }
        
        // Limit number of devices for security
        if (device_list.size() > 10) {
            LOG_WARN("Too many devices (limit: 10)");
            return false;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        devices = device_list;
        
        LOG_INFO("Device list updated with " + std::to_string(device_list.size()) + " devices");
        return true;
    } catch (...) {
        LOG_ERROR("Failed to set device list");
        return false;
    }
}

bool AppState::addKnownAddress(const KnownAddress& address) noexcept {
    try {
        if (!address.isValid()) {
            LOG_WARN("Attempted to add invalid known address");
            return false;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check for duplicates
        auto it = std::find(known_addresses.begin(), known_addresses.end(), address);
        if (it != known_addresses.end()) {
            LOG_DEBUG("Address already exists in address book");
            return true;
        }
        
        // Limit address book size
        const auto& config = Config::getInstance().getAppConfig();
        if (known_addresses.size() >= static_cast<size_t>(config.max_address_book_entries)) {
            LOG_WARN("Address book is full (limit: " + 
                    std::to_string(config.max_address_book_entries) + ")");
            return false;
        }
        
        known_addresses.push_back(address);
        std::sort(known_addresses.begin(), known_addresses.end());
        
        LOG_INFO("Added address to address book: " + address.name);
        return true;
    } catch (...) {
        LOG_ERROR("Failed to add known address");
        return false;
    }
}

bool AppState::setKnownAddresses(const std::vector<KnownAddress>& addresses) noexcept {
    try {
        // Validate all addresses
        for (const auto& addr : addresses) {
            if (!addr.isValid()) {
                LOG_WARN("Invalid address in address list");
                return false;
            }
        }
        
        // Check size limits
        const auto& config = Config::getInstance().getAppConfig();
        if (addresses.size() > static_cast<size_t>(config.max_address_book_entries)) {
            LOG_WARN("Address list exceeds maximum size");
            return false;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        known_addresses = addresses;
        std::sort(known_addresses.begin(), known_addresses.end());
        
        LOG_INFO("Address book updated with " + std::to_string(addresses.size()) + " addresses");
        return true;
    } catch (...) {
        LOG_ERROR("Failed to set known addresses");
        return false;
    }
}

bool AppState::setUsbContacts(const std::vector<KnownAddress>& contacts) noexcept {
    try {
        // Validate all contacts
        for (const auto& contact : contacts) {
            if (!contact.isValid()) {
                LOG_WARN("Invalid contact in USB contact list");
                return false;
            }
        }
        
        // Limit USB contacts for security
        if (contacts.size() > 100) {
            LOG_WARN("Too many USB contacts (limit: 100)");
            return false;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        usb_contacts = contacts;
        
        LOG_INFO("USB contacts updated with " + std::to_string(contacts.size()) + " contacts");
        return true;
    } catch (...) {
        LOG_ERROR("Failed to set USB contacts");
        return false;
    }
}

void AppState::setAddressSuggestion(const std::string& suggestion) noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        address_suggestion = suggestion.substr(0, 500);  // Limit length
    } catch (...) {
        LOG_ERROR("Failed to set address suggestion");
    }
}

void AppState::setFieldErrors(const std::map<std::string, std::string>& errors) noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        field_errors.clear();
        
        // Limit number of errors and their length
        int count = 0;
        for (const auto& [field, error] : errors) {
            if (count >= 20) break;  // Max 20 field errors
            
            field_errors[field.substr(0, 100)] = error.substr(0, 500);
            count++;
        }
    } catch (...) {
        LOG_ERROR("Failed to set field errors");
    }
}

void AppState::setNetworkName(const std::string& name) noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        network_name = name.substr(0, 50);  // Limit length
        LOG_INFO("Network name set to: " + network_name);
    } catch (...) {
        LOG_ERROR("Failed to set network name");
    }
}

bool AppState::setAccountPath(const std::string& path) noexcept {
    try {
        // Validate BIP-44 path format
        static const std::regex bip44_regex(R"(m/44'/\d+'/\d+'/\d+/\d+)");
        if (!std::regex_match(path, bip44_regex)) {
            LOG_WARN("Invalid BIP-44 derivation path format");
            return false;
        }
        
        if (path.length() > 100) {
            LOG_WARN("Account path too long");
            return false;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        account_path = path;
        
        LOG_INFO("Account path set to: " + path);
        return true;
    } catch (...) {
        LOG_ERROR("Failed to set account path");
        return false;
    }
}

void AppState::setSelectedDevice(int index) noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        if (index >= 0 && index < static_cast<int>(devices.size())) {
            selected_device.store(index);
            LOG_DEBUG("Selected device index: " + std::to_string(index));
        } else {
            LOG_WARN("Invalid device index: " + std::to_string(index));
        }
    } catch (...) {
        LOG_ERROR("Failed to set selected device");
    }
}

void AppState::setSelectedContact(int index) noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        if (index >= 0 && index < static_cast<int>(usb_contacts.size())) {
            selected_contact.store(index);
            LOG_DEBUG("Selected contact index: " + std::to_string(index));
        } else {
            LOG_WARN("Invalid contact index: " + std::to_string(index));
        }
    } catch (...) {
        LOG_ERROR("Failed to set selected contact");
    }
}

bool AppState::isStateValid() const noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Basic state validation
        if (network_name.empty() || network_name.length() > 50) {
            return false;
        }
        
        if (account_path.empty() || account_path.length() > 100) {
            return false;
        }
        
        if (known_addresses.size() > 10000) {  // Reasonable limit
            return false;
        }
        
        if (usb_contacts.size() > 1000) {  // Reasonable limit
            return false;
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

std::vector<std::string> AppState::getStateValidationErrors() const noexcept {
    std::vector<std::string> errors;
    
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (network_name.empty()) {
            errors.push_back("Network name is required");
        } else if (network_name.length() > 50) {
            errors.push_back("Network name too long");
        }
        
        if (account_path.empty()) {
            errors.push_back("Account path is required");
        } else if (account_path.length() > 100) {
            errors.push_back("Account path too long");
        }
        
        if (known_addresses.size() > 10000) {
            errors.push_back("Too many known addresses");
        }
        
        if (usb_contacts.size() > 1000) {
            errors.push_back("Too many USB contacts");
        }
        
        // Validate transaction if present
        if (has_unsigned.load()) {
            auto tx_errors = unsigned_tx.getValidationErrors();
            errors.insert(errors.end(), tx_errors.begin(), tx_errors.end());
        }
        
    } catch (...) {
        errors.push_back("State validation error occurred");
    }
    
    return errors;
}

void AppState::clearTransaction() noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        unsigned_tx.clear();
        has_unsigned.store(false);
        signed_hex.clear();
        has_signed.store(false);
        field_errors.clear();
        
        LOG_INFO("Transaction data cleared");
    } catch (...) {
        LOG_ERROR("Failed to clear transaction data");
    }
}

void AppState::clearError() noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        error.clear();
        field_errors.clear();
        
        LOG_DEBUG("Errors cleared");
    } catch (...) {
        // Silent failure for error clearing
    }
}


void AppState::clearAll() noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        
        route = Route::ConnectWallet;
        previous_route = Route::ConnectWallet;
        
        unsigned_tx.clear();
        has_unsigned.store(false);
        signed_hex.clear();
        has_signed.store(false);
        
        status.clear();
        error.clear();
        info.clear();
        
        selected_device.store(0);
        devices.clear();
        wallet_connected.store(false);
        
        known_addresses.clear();
        usb_contacts.clear();
        address_suggestion.clear();
        selected_contact.store(0);
        is_scanning_usb.store(false);
        usb_scan_complete.store(false);
        
        animation_frame.store(0);
        is_signing.store(false);
        is_detecting_wallet.store(false);
        
        field_errors.clear();
        
        LOG_INFO("Application state cleared");
    } catch (...) {
        LOG_ERROR("Failed to clear application state");
    }
}

void AppState::resetErrorState() noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        error_count_ = 0;
        last_error_time_ = std::chrono::system_clock::time_point{};
        
        LOG_DEBUG("Error state reset");
    } catch (...) {
        // Silent failure
    }
}

bool AppState::isInErrorRecovery() const noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::system_clock::now();
        auto time_since_error = std::chrono::duration_cast<std::chrono::seconds>(
            now - last_error_time_);
        
        return error_count_ > 5 && time_since_error.count() < 60;  // 1 minute recovery period
    } catch (...) {
        return false;
    }
}

void AppState::addRouteChangeCallback(std::function<void(Route, Route)> callback) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        route_change_callbacks_.push_back(std::move(callback));
    } catch (...) {
        LOG_ERROR("Failed to add route change callback");
    }
}

void AppState::addErrorCallback(std::function<void(const std::string&)> callback) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        error_callbacks_.push_back(std::move(callback));
    } catch (...) {
        LOG_ERROR("Failed to add error callback");
    }
}

void AppState::clearCallbacks() noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        route_change_callbacks_.clear();
        error_callbacks_.clear();
        
        LOG_DEBUG("Callbacks cleared");
    } catch (...) {
        LOG_ERROR("Failed to clear callbacks");
    }
}

size_t AppState::getKnownAddressCount() const noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        return known_addresses.size();
    } catch (...) {
        return 0;
    }
}

size_t AppState::getUsbContactCount() const noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        return usb_contacts.size();
    } catch (...) {
        return 0;
    }
}

bool AppState::hasFieldError(const std::string& field) const noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        return field_errors.count(field) > 0;
    } catch (...) {
        return false;
    }
}

bool AppState::loadFromConfig() noexcept {
    try {
        const auto& config = Config::getInstance();
        const auto& network_config = config.getNetworkConfig();
        const auto& app_config = config.getAppConfig();
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        network_name = network_config.name;
        use_eip1559.store(network_config.use_eip1559);
        show_wei.store(app_config.show_amounts_in_wei);
        
        // Initialize transaction with defaults
        unsigned_tx = UnsignedTx::createFromDefaults(network_config.chain_id, network_config.use_eip1559);
        
        LOG_INFO("State loaded from configuration");
        return true;
    } catch (...) {
        LOG_ERROR("Failed to load state from configuration");
        return false;
    }
}

bool AppState::saveToConfig() const noexcept {
    try {
        auto& config = Config::getInstance();
        auto app_config = config.getAppConfig();
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        app_config.show_amounts_in_wei = show_wei.load();
        config.setAppConfig(app_config);
        
        LOG_INFO("State saved to configuration");
        return config.save();
    } catch (...) {
        LOG_ERROR("Failed to save state to configuration");
        return false;
    }
}

} // namespace app

