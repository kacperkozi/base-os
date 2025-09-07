#include "app/config.hpp"
#include "app/logger.hpp"
#include <iostream>

namespace app {

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

void Config::initializeDefaults() noexcept {
    // Initialize network config with Base defaults
    network_config_.name = "Base";
    network_config_.chain_id = 8453;
    network_config_.rpc_url = "https://mainnet.base.org";
    network_config_.use_eip1559 = true;
    network_config_.default_gas_limit = "21000";
    network_config_.default_gas_price = "0.027";
    network_config_.default_max_fee = "50";
    network_config_.default_priority_fee = "2";
    
    // Initialize app config with defaults
    app_config_.log_level = "INFO";
    app_config_.log_file = "";
    app_config_.debug_mode = false;
    app_config_.address_book_path = "";
    app_config_.last_save_path = "";
    app_config_.max_address_book_entries = 1000;
    app_config_.input_timeout_ms = 30000;
    app_config_.show_amounts_in_wei = false;
    app_config_.preferred_wallet_path = "m/44'/60'/0'/0/0";
    app_config_.animation_speed_ms = 100;
    app_config_.enable_qr_codes = true;
    app_config_.max_transaction_history = 100;
}

bool Config::load(const std::string& config_file_path) {
    // For now, just use default values
    initializeDefaults();
    LOG_INFO("Using default configuration");
    return true;
}

bool Config::save() const {
    // For now, just log that we would save
    LOG_INFO("Configuration save requested (not implemented)");
    return true;
}

std::optional<Config::NetworkConfig> Config::getNetworkByChainId(int chain_id) const noexcept {
    if (network_config_.chain_id == chain_id) {
        return network_config_;
    }
    return std::nullopt;
}

bool Config::isValidLogLevel(const std::string& level) const noexcept {
    return level == "DEBUG" || level == "INFO" || level == "WARN" || level == "ERROR";
}

bool Config::isValidPath(const std::string& path) const noexcept {
    return !path.empty() && path.length() < 1000;
}

void Config::resetToDefaults() noexcept {
    initializeDefaults();
}

std::string Config::serializeToJson() const {
    // Simple JSON serialization (not implemented)
    return "{}";
}

bool Config::deserializeFromJson(const std::string& json_content) {
    // Simple JSON deserialization (not implemented)
    return true;
}

std::optional<std::string> Config::readConfigFile(const std::string& file_path) const {
    // File reading (not implemented)
    return std::nullopt;
}

bool Config::writeConfigFile(const std::string& file_path, const std::string& content) const {
    // File writing (not implemented)
    return false;
}

std::string Config::getDefaultConfigPath() const {
    return "config.json";
}

} // namespace app
