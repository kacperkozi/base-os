#pragma once
#include <string>
#include <map>
#include <optional>
#include <memory>

namespace app {

/**
 * Configuration management class for the TUI application.
 * Supports JSON configuration files with defaults and validation.
 */
class Config {
public:
    struct NetworkConfig {
        std::string name;
        int chain_id;
        std::string rpc_url;
        bool use_eip1559;
        std::string default_gas_limit;
        std::string default_gas_price;
        std::string default_max_fee;
        std::string default_priority_fee;
    };

    struct AppConfig {
        std::string log_level;
        std::string log_file;
        bool debug_mode;
        std::string address_book_path;
        std::string last_save_path;
        int max_address_book_entries;
        int input_timeout_ms;
        bool show_amounts_in_wei;
        std::string preferred_wallet_path;
        int animation_speed_ms;
        bool enable_qr_codes;
        int max_transaction_history;
    };

    // Singleton pattern for global access
    static Config& getInstance();
    
    // Load configuration from file (creates with defaults if not exists)
    bool load(const std::string& config_file_path = "");
    
    // Save current configuration to file
    bool save() const;
    
    // Get configuration values with type safety
    const NetworkConfig& getNetworkConfig() const noexcept { return network_config_; }
    const AppConfig& getAppConfig() const noexcept { return app_config_; }
    
    // Update configuration values
    void setNetworkConfig(const NetworkConfig& config) noexcept { network_config_ = config; }
    void setAppConfig(const AppConfig& config) noexcept { app_config_ = config; }
    
    // Get specific network by chain ID
    std::optional<NetworkConfig> getNetworkByChainId(int chain_id) const noexcept;
    
    // Validation helpers
    bool isValidLogLevel(const std::string& level) const noexcept;
    bool isValidPath(const std::string& path) const noexcept;
    
    // Config file path management
    std::string getConfigFilePath() const noexcept { return config_file_path_; }
    void setConfigFilePath(const std::string& path) noexcept { config_file_path_ = path; }
    
    // Reset to defaults
    void resetToDefaults() noexcept;

private:
    Config() = default;
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    
    // Configuration data
    NetworkConfig network_config_;
    AppConfig app_config_;
    std::map<int, NetworkConfig> supported_networks_;
    std::string config_file_path_;
    
    // Initialize default values
    void initializeDefaults() noexcept;
    
    // Validation methods
    bool validateNetworkConfig(const NetworkConfig& config) const noexcept;
    bool validateAppConfig(const AppConfig& config) const noexcept;
    
    // JSON serialization helpers
    std::string serializeToJson() const;
    bool deserializeFromJson(const std::string& json_content);
    
    // File I/O helpers with error handling
    std::optional<std::string> readConfigFile(const std::string& file_path) const;
    bool writeConfigFile(const std::string& file_path, const std::string& content) const;
    
    // Get default config file path
    std::string getDefaultConfigPath() const;
};

} // namespace app