#pragma once
#include <string>
#include <vector>
#include <optional>
#include <regex>
#include <limits>
#include <cstdint>

namespace app {

// Forward declarations
struct UnsignedTx;
struct KnownAddress;

/**
 * Comprehensive validation system with bounds checking and security features.
 */
class Validator {
public:
    // Ethereum address validation
    static bool isHex(const std::string& s) noexcept;
    static bool isAddress(const std::string& s) noexcept;
    static bool isAddressChecksum(const std::string& address) noexcept;
    static std::optional<std::string> toChecksumAddress(const std::string& address) noexcept;
    static bool isENSName(const std::string& s) noexcept;
    
    // Numeric validation with bounds checking
    static bool isNumeric(const std::string& s) noexcept;
    static bool isValidWeiAmount(const std::string& value) noexcept;
    static bool isValidGasLimit(const std::string& gas_limit) noexcept;
    static bool isValidGasPrice(const std::string& gas_price) noexcept;
    static bool isValidNonce(const std::string& nonce) noexcept;
    static bool isValidChainId(int chain_id) noexcept;
    
    // Input sanitization and limits
    static std::string sanitizeInput(const std::string& input, size_t max_length = 1000) noexcept;
    static bool isValidFilePath(const std::string& path) noexcept;
    static bool isValidLogLevel(const std::string& level) noexcept;
    
    // Transaction validation
    static std::vector<std::string> validateTransaction(const UnsignedTx& tx, bool strict_mode = false) noexcept;
    static bool isValidTransactionData(const std::string& data) noexcept;
    
    // Address book validation
    static bool isValidKnownAddress(const KnownAddress& address) noexcept;
    static std::vector<std::string> validateKnownAddress(const KnownAddress& address) noexcept;
    
    // String validation helpers
    static bool isValidName(const std::string& name, size_t min_length = 1, size_t max_length = 100) noexcept;
    static bool isValidDescription(const std::string& description, size_t max_length = 500) noexcept;
    static bool containsOnlyPrintableAscii(const std::string& s) noexcept;
    static bool containsNoControlChars(const std::string& s) noexcept;
    
    // Network and security validation
    static bool isValidDerivationPath(const std::string& path) noexcept;
    static bool isValidUSBPath(const std::string& path) noexcept;
    static bool isSafeForLogging(const std::string& s) noexcept;
    
    // Rate limiting and DoS protection
    static bool checkInputFrequency(const std::string& input_type) noexcept;
    static void resetInputFrequency() noexcept;
    
private:
    // Internal helpers
    static std::string toLowerCase(const std::string& s) noexcept;
    static std::string toUpperCase(const std::string& s) noexcept;
    static bool isOverflowSafe(const std::string& numeric_str, uint64_t max_value) noexcept;
    
    // Checksum calculation (simplified Keccak-256 alternative for validation)
    static std::string calculateAddressChecksum(const std::string& address) noexcept;
};

// Legacy compatibility functions (delegating to Validator class)
inline std::vector<std::string> Validate(const UnsignedTx& tx, bool strict_chain = true) {
    return Validator::validateTransaction(tx, strict_chain);
}

inline bool IsHex(const std::string& s) {
    return Validator::isHex(s);
}

inline bool IsAddress(const std::string& s) {
    return Validator::isAddress(s);
}

inline bool IsNumeric(const std::string& s) {
    return Validator::isNumeric(s);
}

// Input field validation results
struct ValidationResult {
    bool is_valid;
    std::string error_message;
    std::string suggestion;  // Optional suggestion for fixing the error
    
    ValidationResult(bool valid = true, const std::string& error = "", const std::string& hint = "") 
        : is_valid(valid), error_message(error), suggestion(hint) {}
        
    operator bool() const noexcept { return is_valid; }
};

// Specialized validators for different input types
class InputValidators {
public:
    static ValidationResult validateAddressInput(const std::string& input) noexcept;
    static ValidationResult validateAmountInput(const std::string& input, bool allow_empty = false) noexcept;
    static ValidationResult validateGasInput(const std::string& input, const std::string& field_name) noexcept;
    static ValidationResult validateNonceInput(const std::string& input) noexcept;
    static ValidationResult validateDataInput(const std::string& input) noexcept;
    static ValidationResult validatePathInput(const std::string& input) noexcept;
    
private:
    static std::string formatValidationError(const std::string& field, const std::string& error) noexcept;
};

// Security-focused validation constants
namespace ValidationLimits {
    constexpr size_t MAX_ADDRESS_LENGTH = 42;
    constexpr size_t MAX_NAME_LENGTH = 100;
    constexpr size_t MAX_DESCRIPTION_LENGTH = 500;
    constexpr size_t MAX_DATA_LENGTH = 1000000;  // 1MB
    constexpr size_t MAX_INPUT_LENGTH = 10000;   // General input limit
    constexpr size_t MAX_PATH_LENGTH = 1000;
    
    constexpr uint64_t MIN_GAS_LIMIT = 21000;
    constexpr uint64_t MAX_GAS_LIMIT = 30000000;
    constexpr uint64_t MAX_GAS_PRICE = 1000000000000ULL;  // 1000 Gwei
    constexpr uint64_t MAX_WEI_AMOUNT = std::numeric_limits<uint64_t>::max() / 2;
    constexpr uint32_t MAX_NONCE = std::numeric_limits<uint32_t>::max();
    
    constexpr int MIN_CHAIN_ID = 1;
    constexpr int MAX_CHAIN_ID = 2147483647;  // 2^31 - 1
}

} // namespace app

