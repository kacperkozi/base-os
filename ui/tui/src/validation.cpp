#include "app/validation.hpp"
#include "app/state.hpp"
#include "app/logger.hpp"
#include <regex>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <iomanip>

namespace app {

// Rate limiting state (simple DoS protection)
namespace {
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_input_times;
    std::unordered_map<std::string, int> input_counts;
    constexpr int MAX_INPUTS_PER_MINUTE = 100;
    constexpr auto INPUT_TIMEOUT = std::chrono::minutes(1);
}

// Validator class implementation
bool Validator::isHex(const std::string& s) noexcept {
    try {
        if (s.empty()) return false;
        if (s.length() < 2) return false;
        if (s.substr(0, 2) != "0x" && s.substr(0, 2) != "0X") return false;
        
        // Check length limits
        if (s.length() > ValidationLimits::MAX_INPUT_LENGTH) return false;
        
        // Check each character
        for (size_t i = 2; i < s.length(); ++i) {
            char c = s[i];
            if (!((c >= '0' && c <= '9') || 
                  (c >= 'a' && c <= 'f') || 
                  (c >= 'A' && c <= 'F'))) {
                return false;
            }
        }
        
        return true;
    } catch (...) {
        return false;
    }
}

bool Validator::isAddress(const std::string& s) noexcept {
    try {
        if (s.length() != ValidationLimits::MAX_ADDRESS_LENGTH) return false;
        if (!isHex(s)) return false;
        
        // Additional validation: should have exactly 40 hex chars after 0x
        return s.length() == 42 && s.substr(0, 2) == "0x";
    } catch (...) {
        return false;
    }
}

bool Validator::isAddressChecksum(const std::string& address) noexcept {
    try {
        if (!isAddress(address)) return false;
        
        std::string calculated_checksum = calculateAddressChecksum(address);
        return address == calculated_checksum;
    } catch (...) {
        return false;
    }
}

std::optional<std::string> Validator::toChecksumAddress(const std::string& address) noexcept {
    try {
        if (!isAddress(address)) {
            return std::nullopt;
        }
        
        return calculateAddressChecksum(address);
    } catch (...) {
        return std::nullopt;
    }
}

bool Validator::isENSName(const std::string& s) noexcept {
    try {
        if (s.empty() || s.length() > 255) return false;
        
        // Basic ENS validation - ends with .eth or contains .
        if (s.find('.') == std::string::npos) return false;
        
        // Must contain only valid DNS characters
        static const std::regex ens_regex(R"([a-zA-Z0-9\-\.]+\.[a-zA-Z]{2,})");
        return std::regex_match(s, ens_regex);
    } catch (...) {
        return false;
    }
}

bool Validator::isNumeric(const std::string& s) noexcept {
    try {
        if (s.empty() || s.length() > 78) return false; // Max digits for uint256
        
        // Check all characters are digits
        return std::all_of(s.begin(), s.end(), [](char c) {
            return c >= '0' && c <= '9';
        });
    } catch (...) {
        return false;
    }
}

bool Validator::isValidWeiAmount(const std::string& value) noexcept {
    try {
        if (!isNumeric(value)) return false;
        if (value == "0") return true;
        
        return isOverflowSafe(value, ValidationLimits::MAX_WEI_AMOUNT);
    } catch (...) {
        return false;
    }
}

bool Validator::isValidGasLimit(const std::string& gas_limit) noexcept {
    try {
        if (!isNumeric(gas_limit)) return false;
        
        uint64_t gas = std::stoull(gas_limit);
        return gas >= ValidationLimits::MIN_GAS_LIMIT && 
               gas <= ValidationLimits::MAX_GAS_LIMIT;
    } catch (...) {
        return false;
    }
}

bool Validator::isValidGasPrice(const std::string& gas_price) noexcept {
    try {
        if (!isNumeric(gas_price)) return false;
        
        return isOverflowSafe(gas_price, ValidationLimits::MAX_GAS_PRICE);
    } catch (...) {
        return false;
    }
}

bool Validator::isValidNonce(const std::string& nonce) noexcept {
    try {
        if (!isNumeric(nonce)) return false;
        
        uint64_t nonce_val = std::stoull(nonce);
        return nonce_val <= ValidationLimits::MAX_NONCE;
    } catch (...) {
        return false;
    }
}

bool Validator::isValidChainId(int chain_id) noexcept {
    return chain_id >= ValidationLimits::MIN_CHAIN_ID && 
           chain_id <= ValidationLimits::MAX_CHAIN_ID;
}

std::string Validator::sanitizeInput(const std::string& input, size_t max_length) noexcept {
    try {
        if (input.empty()) return "";
        
        std::string sanitized;
        sanitized.reserve(std::min(input.length(), max_length));
        
        for (char c : input) {
            if (sanitized.length() >= max_length) break;
            
            // Only allow printable ASCII characters and basic whitespace
            if ((c >= 32 && c <= 126) || c == '\t' || c == '\n' || c == '\r') {
                sanitized += c;
            }
        }
        
        return sanitized;
    } catch (...) {
        return "";
    }
}

bool Validator::isValidFilePath(const std::string& path) noexcept {
    try {
        if (path.empty() || path.length() > ValidationLimits::MAX_PATH_LENGTH) {
            return false;
        }
        
        // Check for dangerous path components
        if (path.find("..") != std::string::npos) return false;
        if (path.find("//") != std::string::npos) return false;
        
        // Basic path validation
        std::filesystem::path p(path);
        return !p.empty();
    } catch (...) {
        return false;
    }
}

bool Validator::isValidLogLevel(const std::string& level) noexcept {
    static const std::vector<std::string> valid_levels = {
        "trace", "debug", "info", "warn", "error", "fatal"
    };
    
    std::string lower_level = toLowerCase(level);
    return std::find(valid_levels.begin(), valid_levels.end(), lower_level) != valid_levels.end();
}

std::vector<std::string> Validator::validateTransaction(const UnsignedTx& tx, bool strict_mode) noexcept {
    std::vector<std::string> errors;
    
    try {
        // Validate recipient address
        if (tx.to.empty()) {
            errors.push_back("Recipient address is required");
        } else if (!isAddress(tx.to)) {
            errors.push_back("Invalid recipient address format");
        } else if (strict_mode && !isAddressChecksum(tx.to)) {
            errors.push_back("Address checksum validation failed");
        }
        
        // Validate amount
        if (tx.value.empty()) {
            errors.push_back("Transaction amount is required");
        } else if (!isValidWeiAmount(tx.value)) {
            errors.push_back("Invalid transaction amount");
        }
        
        // Validate nonce
        if (tx.nonce.empty()) {
            errors.push_back("Transaction nonce is required");
        } else if (!isValidNonce(tx.nonce)) {
            errors.push_back("Invalid nonce value");
        }
        
        // Validate gas limit
        if (tx.gas_limit.empty()) {
            errors.push_back("Gas limit is required");
        } else if (!isValidGasLimit(tx.gas_limit)) {
            errors.push_back("Gas limit must be between 21,000 and 30,000,000");
        }
        
        // Validate chain ID
        if (!isValidChainId(tx.chain_id)) {
            errors.push_back("Invalid chain ID");
        }
        
        // Validate transaction type
        if (tx.type != 0 && tx.type != 2) {
            errors.push_back("Transaction type must be 0 (legacy) or 2 (EIP-1559)");
        }
        
        // EIP-1559 specific validation
        if (tx.isEIP1559()) {
            if (tx.max_fee_per_gas.empty()) {
                errors.push_back("Max fee per gas is required for EIP-1559 transactions");
            } else if (!isValidGasPrice(tx.max_fee_per_gas)) {
                errors.push_back("Invalid max fee per gas");
            }
            
            if (tx.max_priority_fee_per_gas.empty()) {
                errors.push_back("Priority fee is required for EIP-1559 transactions");
            } else if (!isValidGasPrice(tx.max_priority_fee_per_gas)) {
                errors.push_back("Invalid priority fee");
            }
            
            // Priority fee should not exceed max fee
            if (!tx.max_fee_per_gas.empty() && !tx.max_priority_fee_per_gas.empty()) {
                try {
                    uint64_t max_fee = std::stoull(tx.max_fee_per_gas);
                    uint64_t priority_fee = std::stoull(tx.max_priority_fee_per_gas);
                    if (priority_fee > max_fee) {
                        errors.push_back("Priority fee cannot exceed max fee");
                    }
                } catch (...) {
                    // Already caught by individual validations above
                }
            }
        } else {
            // Legacy transaction validation
            if (tx.gas_price.empty()) {
                errors.push_back("Gas price is required for legacy transactions");
            } else if (!isValidGasPrice(tx.gas_price)) {
                errors.push_back("Invalid gas price");
            }
        }
        
        // Validate transaction data
        if (!tx.data.empty() && !isValidTransactionData(tx.data)) {
            errors.push_back("Invalid transaction data");
        }
        
        // Additional strict mode validations
        if (strict_mode) {
            if (tx.chain_id != 8453 && tx.chain_id != 84532 && tx.chain_id != 1 && tx.chain_id != 11155111) {
                errors.push_back("Unsupported network");
            }
        }
        
    } catch (const std::exception& e) {
        errors.push_back("Transaction validation error: " + std::string(e.what()));
        LOG_ERROR("Transaction validation exception: " + std::string(e.what()));
    } catch (...) {
        errors.push_back("Unknown transaction validation error");
        LOG_ERROR("Unknown transaction validation exception");
    }
    
    return errors;
}

bool Validator::isValidTransactionData(const std::string& data) noexcept {
    try {
        if (data.empty()) return true;
        if (data == "0x") return true;
        
        if (!isHex(data)) return false;
        if (data.length() > ValidationLimits::MAX_DATA_LENGTH) return false;
        
        // Data length should be even (each byte = 2 hex chars)
        return (data.length() - 2) % 2 == 0;
    } catch (...) {
        return false;
    }
}

bool Validator::isValidKnownAddress(const KnownAddress& address) noexcept {
    try {
        return isAddress(address.address) &&
               isValidName(address.name) &&
               isValidDescription(address.description);
    } catch (...) {
        return false;
    }
}

std::vector<std::string> Validator::validateKnownAddress(const KnownAddress& address) noexcept {
    std::vector<std::string> errors;
    
    try {
        if (address.address.empty()) {
            errors.push_back("Address is required");
        } else if (!isAddress(address.address)) {
            errors.push_back("Invalid address format");
        }
        
        if (address.name.empty()) {
            errors.push_back("Name is required");
        } else if (!isValidName(address.name)) {
            errors.push_back("Name contains invalid characters or is too long");
        }
        
        if (!isValidDescription(address.description)) {
            errors.push_back("Description is too long or contains invalid characters");
        }
        
        if (!containsNoControlChars(address.name) || !containsNoControlChars(address.description)) {
            errors.push_back("Name or description contains control characters");
        }
        
    } catch (...) {
        errors.push_back("Address validation error");
    }
    
    return errors;
}

bool Validator::isValidName(const std::string& name, size_t min_length, size_t max_length) noexcept {
    try {
        if (name.length() < min_length || name.length() > max_length) {
            return false;
        }
        
        return containsOnlyPrintableAscii(name) && containsNoControlChars(name);
    } catch (...) {
        return false;
    }
}

bool Validator::isValidDescription(const std::string& description, size_t max_length) noexcept {
    try {
        if (description.length() > max_length) return false;
        
        return containsOnlyPrintableAscii(description) && containsNoControlChars(description);
    } catch (...) {
        return false;
    }
}

bool Validator::containsOnlyPrintableAscii(const std::string& s) noexcept {
    try {
        return std::all_of(s.begin(), s.end(), [](unsigned char c) {
            return c >= 32 && c <= 126;
        });
    } catch (...) {
        return false;
    }
}

bool Validator::containsNoControlChars(const std::string& s) noexcept {
    try {
        return std::all_of(s.begin(), s.end(), [](unsigned char c) {
            return c >= 32 || c == '\t' || c == '\n' || c == '\r';
        });
    } catch (...) {
        return false;
    }
}

bool Validator::isValidDerivationPath(const std::string& path) noexcept {
    try {
        // BIP-44 path validation: m/44'/coin'/account'/change/index
        static const std::regex bip44_regex(R"(^m/44'/\d+'/\d+'/[01]/\d+$)");
        return std::regex_match(path, bip44_regex) && path.length() <= 50;
    } catch (...) {
        return false;
    }
}

bool Validator::isValidUSBPath(const std::string& path) noexcept {
    try {
        if (path.empty() || path.length() > ValidationLimits::MAX_PATH_LENGTH) {
            return false;
        }
        
        // Basic USB device path validation (Linux/Unix style)
        return path.find("/dev/") == 0 || path.find("/sys/") == 0 || 
               path.find("hidraw") != std::string::npos;
    } catch (...) {
        return false;
    }
}

bool Validator::isSafeForLogging(const std::string& s) noexcept {
    try {
        if (s.empty() || s.length() > 1000) return false;
        
        // Don't log potential private keys or sensitive data
        if (s.length() == 64 && isHex("0x" + s)) return false;  // Potential private key
        if (s.find("private") != std::string::npos) return false;
        if (s.find("secret") != std::string::npos) return false;
        if (s.find("mnemonic") != std::string::npos) return false;
        
        return containsOnlyPrintableAscii(s);
    } catch (...) {
        return false;
    }
}

bool Validator::checkInputFrequency(const std::string& input_type) noexcept {
    try {
        auto now = std::chrono::steady_clock::now();
        
        // Clean up old entries
        auto it = last_input_times.begin();
        while (it != last_input_times.end()) {
            if (now - it->second > INPUT_TIMEOUT) {
                input_counts.erase(it->first);
                it = last_input_times.erase(it);
            } else {
                ++it;
            }
        }
        
        // Check current input frequency
        auto& count = input_counts[input_type];
        auto& last_time = last_input_times[input_type];
        
        if (now - last_time < std::chrono::milliseconds(100)) {
            count++;
            if (count > MAX_INPUTS_PER_MINUTE) {
                return false;  // Rate limit exceeded
            }
        } else {
            count = 1;
        }
        
        last_time = now;
        return true;
    } catch (...) {
        return true;  // Allow on error to avoid blocking user
    }
}

void Validator::resetInputFrequency() noexcept {
    try {
        last_input_times.clear();
        input_counts.clear();
    } catch (...) {
        // Silent failure
    }
}

// Private helper methods
std::string Validator::toLowerCase(const std::string& s) noexcept {
    try {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), 
                      [](unsigned char c) { return std::tolower(c); });
        return result;
    } catch (...) {
        return s;
    }
}

std::string Validator::toUpperCase(const std::string& s) noexcept {
    try {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), 
                      [](unsigned char c) { return std::toupper(c); });
        return result;
    } catch (...) {
        return s;
    }
}

bool Validator::isOverflowSafe(const std::string& numeric_str, uint64_t max_value) noexcept {
    try {
        if (numeric_str.empty()) return false;
        
        // Quick length check - if longer than max uint64 string, it's too big
        if (numeric_str.length() > 20) return false;
        
        uint64_t value = std::stoull(numeric_str);
        return value <= max_value;
    } catch (...) {
        return false;
    }
}

std::string Validator::calculateAddressChecksum(const std::string& address) noexcept {
    try {
        if (!isAddress(address)) return "";
        
        // Simplified checksum calculation (not full Keccak-256)
        // In production, you'd use a proper Keccak-256 implementation
        std::string addr_lower = toLowerCase(address.substr(2));  // Remove 0x prefix
        std::string result = "0x";
        
        // Simple checksum: uppercase if position is even and char >= 'a'
        for (size_t i = 0; i < addr_lower.length(); ++i) {
            char c = addr_lower[i];
            if (c >= 'a' && c <= 'f' && i % 2 == 0) {
                result += std::toupper(c);
            } else {
                result += c;
            }
        }
        
        return result;
    } catch (...) {
        return "";
    }
}

// InputValidators implementation
ValidationResult InputValidators::validateAddressInput(const std::string& input) noexcept {
    try {
        if (input.empty()) {
            return ValidationResult(false, "Address is required", "Enter a valid Ethereum address starting with 0x");
        }
        
        if (input.length() > ValidationLimits::MAX_ADDRESS_LENGTH) {
            return ValidationResult(false, "Address is too long", "Ethereum addresses should be exactly 42 characters");
        }
        
        if (Validator::isENSName(input)) {
            return ValidationResult(true, "", "ENS name detected - will be resolved to address");
        }
        
        if (!Validator::isAddress(input)) {
            return ValidationResult(false, "Invalid address format", "Address must be 42 characters starting with 0x");
        }
        
        if (!Validator::isAddressChecksum(input)) {
            auto checksum = Validator::toChecksumAddress(input);
            if (checksum) {
                return ValidationResult(true, "", "Consider using checksum format: " + *checksum);
            }
        }
        
        return ValidationResult(true);
    } catch (...) {
        return ValidationResult(false, "Address validation error");
    }
}

ValidationResult InputValidators::validateAmountInput(const std::string& input, bool allow_empty) noexcept {
    try {
        if (input.empty()) {
            if (allow_empty) {
                return ValidationResult(true);
            }
            return ValidationResult(false, "Amount is required", "Enter amount in Wei (e.g., 1000000000000000000 for 1 ETH)");
        }
        
        if (!Validator::isValidWeiAmount(input)) {
            return ValidationResult(false, "Invalid amount", "Amount must be a positive number in Wei");
        }
        
        if (input == "0") {
            return ValidationResult(false, "Amount must be greater than 0", "Enter a positive amount");
        }
        
        // Helpful conversion info
        try {
            uint64_t wei_amount = std::stoull(input);
            if (wei_amount >= 1000000000000000000ULL) {  // 1 ETH in Wei
                double eth_amount = static_cast<double>(wei_amount) / 1000000000000000000.0;
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(6) << eth_amount;
                return ValidationResult(true, "", "≈ " + oss.str() + " ETH");
            }
        } catch (...) {
            // Ignore conversion errors
        }
        
        return ValidationResult(true);
    } catch (...) {
        return ValidationResult(false, "Amount validation error");
    }
}

ValidationResult InputValidators::validateGasInput(const std::string& input, const std::string& field_name) noexcept {
    try {
        if (input.empty()) {
            return ValidationResult(false, field_name + " is required", "Enter a valid gas value");
        }
        
        if (field_name == "Gas Limit") {
            if (!Validator::isValidGasLimit(input)) {
                return ValidationResult(false, "Invalid gas limit", 
                    "Gas limit must be between 21,000 and 30,000,000");
            }
        } else {
            if (!Validator::isValidGasPrice(input)) {
                return ValidationResult(false, "Invalid " + field_name, 
                    field_name + " must be a valid number");
            }
        }
        
        return ValidationResult(true);
    } catch (...) {
        return ValidationResult(false, field_name + " validation error");
    }
}

ValidationResult InputValidators::validateNonceInput(const std::string& input) noexcept {
    try {
        if (input.empty()) {
            return ValidationResult(false, "Nonce is required", "Enter the transaction nonce (usually account transaction count)");
        }
        
        if (!Validator::isValidNonce(input)) {
            return ValidationResult(false, "Invalid nonce", "Nonce must be a non-negative integer");
        }
        
        return ValidationResult(true);
    } catch (...) {
        return ValidationResult(false, "Nonce validation error");
    }
}

ValidationResult InputValidators::validateDataInput(const std::string& input) noexcept {
    try {
        if (input.empty()) {
            return ValidationResult(true, "", "Leave empty for simple transfers, or enter hex data for contract calls");
        }
        
        if (!Validator::isValidTransactionData(input)) {
            return ValidationResult(false, "Invalid transaction data", "Data must be valid hex starting with 0x");
        }
        
        if (input.length() > 1000) {
            return ValidationResult(true, "", "Large data detected - ensure this is correct");
        }
        
        return ValidationResult(true);
    } catch (...) {
        return ValidationResult(false, "Data validation error");
    }
}

ValidationResult InputValidators::validatePathInput(const std::string& input) noexcept {
    try {
        if (input.empty()) {
            return ValidationResult(false, "Path is required");
        }
        
        if (!Validator::isValidFilePath(input)) {
            return ValidationResult(false, "Invalid file path", "Enter a valid file path");
        }
        
        return ValidationResult(true);
    } catch (...) {
        return ValidationResult(false, "Path validation error");
    }
}

std::string InputValidators::formatValidationError(const std::string& field, const std::string& error) noexcept {
    try {
        return field + ": " + error;
    } catch (...) {
        return "Validation error";
    }
}

} // namespace app

