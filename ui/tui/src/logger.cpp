#include "app/logger.hpp"
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <algorithm>

namespace app {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    shutdown();
}

bool Logger::initialize(const std::string& log_file_path, Level min_level, 
                       bool console_output, size_t max_file_size_mb) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        // Close existing log file if open
        if (log_file_.is_open()) {
            log_file_.close();
        }
        
        log_file_path_ = log_file_path;
        min_level_ = min_level;
        console_output_ = console_output;
        max_file_size_bytes_ = max_file_size_mb * 1024 * 1024;
        current_file_size_ = 0;
        
        // Create directory if it doesn't exist
        std::filesystem::path log_path(log_file_path_);
        if (log_path.has_parent_path()) {
            std::filesystem::create_directories(log_path.parent_path());
        }
        
        // Open log file in append mode
        log_file_.open(log_file_path_, std::ios::app);
        if (!log_file_.is_open()) {
            std::cerr << "Logger: Failed to open log file: " << log_file_path_ << std::endl;
            return false;
        }
        
        // Get current file size
        try {
            if (std::filesystem::exists(log_file_path_)) {
                current_file_size_ = std::filesystem::file_size(log_file_path_);
            }
        } catch (...) {
            current_file_size_ = 0;
        }
        
        initialized_ = true;
        
        // Log initialization
        info("Logger initialized: level=" + levelToString(min_level_) + 
             ", console=" + (console_output_ ? "true" : "false") + 
             ", file=" + log_file_path_);
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Logger initialization error: " << e.what() << std::endl;
        return false;
    }
}

void Logger::shutdown() noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (initialized_) {
            info("Logger shutting down");
            initialized_ = false;
        }
        
        if (log_file_.is_open()) {
            log_file_.close();
        }
        
    } catch (...) {
        // Swallow all exceptions in shutdown
    }
}

void Logger::log(Level level, const std::string& message, 
                const char* file, int line, const char* function) noexcept {
    try {
        // Early return if level is below threshold
        if (level < min_level_) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!initialized_) {
            return;
        }
        
        std::string formatted_message = formatMessage(level, message, file, line, function);
        
        // Write to file
        writeToFile(formatted_message);
        
        // Write to console
        if (console_output_) {
            writeToConsole(formatted_message);
        }
        
        // Check if log rotation is needed
        rotateLogFileIfNeeded();
        
    } catch (...) {
        // Logging should never throw exceptions
        // Try to write error to stderr as last resort
        try {
            std::cerr << "Logger error while writing message: " << message << std::endl;
        } catch (...) {
            // Even this failed, give up silently
        }
    }
}

void Logger::logException(const std::exception& e, const std::string& context,
                         const char* file, int line, const char* function) noexcept {
    try {
        std::string msg = "Exception caught: " + std::string(e.what());
        if (!context.empty()) {
            msg += " (Context: " + context + ")";
        }
        log(Level::ERROR, msg, file, line, function);
        
    } catch (...) {
        // Last resort error handling
        try {
            std::cerr << "Fatal logger error while logging exception: " << e.what() << std::endl;
        } catch (...) {
            // Give up silently
        }
    }
}

std::string Logger::levelToString(Level level) noexcept {
    switch (level) {
        case Level::TRACE: return "TRACE";
        case Level::DEBUG: return "DEBUG";
        case Level::INFO:  return "INFO";
        case Level::WARN:  return "WARN";
        case Level::ERROR: return "ERROR";
        case Level::FATAL: return "FATAL";
        default:           return "UNKNOWN";
    }
}

Logger::Level Logger::stringToLevel(const std::string& level_str) noexcept {
    std::string lower_level = level_str;
    std::transform(lower_level.begin(), lower_level.end(), lower_level.begin(), ::tolower);
    
    if (lower_level == "trace") return Level::TRACE;
    if (lower_level == "debug") return Level::DEBUG;
    if (lower_level == "info")  return Level::INFO;
    if (lower_level == "warn")  return Level::WARN;
    if (lower_level == "error") return Level::ERROR;
    if (lower_level == "fatal") return Level::FATAL;
    
    return Level::INFO; // Default fallback
}

std::unique_ptr<Logger::PerformanceTimer> Logger::createTimer(const std::string& operation_name) {
    return std::make_unique<PerformanceTimer>(operation_name);
}

std::string Logger::formatMessage(Level level, const std::string& message,
                                 const char* file, int line, const char* function) const noexcept {
    try {
        std::ostringstream formatted;
        
        // Timestamp
        formatted << "[" << getCurrentTimestamp() << "] ";
        
        // Level
        formatted << "[" << std::setw(5) << levelToString(level) << "] ";
        
        // Thread ID
        formatted << "[" << std::this_thread::get_id() << "] ";
        
        // Message
        formatted << message;
        
        // Source location (only for debug/trace levels to reduce noise)
        if (level <= Level::DEBUG && file && function) {
            formatted << " (" << extractFileName(file) << ":" << line << " in " << function << ")";
        }
        
        return formatted.str();
        
    } catch (...) {
        return "[TIMESTAMP] [ERROR] Logger formatting error: " + message;
    }
}

std::string Logger::getCurrentTimestamp() const noexcept {
    try {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::ostringstream timestamp;
        timestamp << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        timestamp << "." << std::setfill('0') << std::setw(3) << milliseconds.count();
        
        return timestamp.str();
        
    } catch (...) {
        return "TIMESTAMP_ERROR";
    }
}

void Logger::writeToFile(const std::string& formatted_message) noexcept {
    try {
        if (!log_file_.is_open()) {
            return;
        }
        
        log_file_ << formatted_message << std::endl;
        log_file_.flush();
        
        current_file_size_ += formatted_message.length() + 1; // +1 for newline
        
    } catch (...) {
        // Silent failure for file writing
    }
}

void Logger::writeToConsole(const std::string& formatted_message) noexcept {
    try {
        std::cout << formatted_message << std::endl;
        std::cout.flush();
        
    } catch (...) {
        // Silent failure for console writing
    }
}

void Logger::rotateLogFileIfNeeded() noexcept {
    try {
        if (current_file_size_ < max_file_size_bytes_) {
            return;
        }
        
        // Close current file
        log_file_.close();
        
        // Rotate files: log.1 -> log.2, log -> log.1
        std::string backup_path = log_file_path_ + ".1";
        
        // Remove old backup if it exists
        if (std::filesystem::exists(backup_path)) {
            std::filesystem::remove(backup_path);
        }
        
        // Move current log to backup
        if (std::filesystem::exists(log_file_path_)) {
            std::filesystem::rename(log_file_path_, backup_path);
        }
        
        // Reopen log file
        log_file_.open(log_file_path_, std::ios::app);
        current_file_size_ = 0;
        
        if (log_file_.is_open()) {
            info("Log file rotated, backup saved as " + backup_path);
        }
        
    } catch (const std::exception& e) {
        // Try to continue with current file
        try {
            std::cerr << "Log rotation error: " << e.what() << std::endl;
        } catch (...) {
            // Give up silently
        }
    }
}

std::string Logger::extractFileName(const char* file_path) const noexcept {
    try {
        if (!file_path) {
            return "unknown_file";
        }
        
        std::string path_str(file_path);
        size_t last_slash = path_str.find_last_of("/\\");
        
        if (last_slash != std::string::npos && last_slash + 1 < path_str.length()) {
            return path_str.substr(last_slash + 1);
        }
        
        return path_str;
        
    } catch (...) {
        return "file_extract_error";
    }
}

// PerformanceTimer implementation
Logger::PerformanceTimer::PerformanceTimer(const std::string& operation_name) 
    : operation_name_(operation_name)
    , start_time_(std::chrono::high_resolution_clock::now()) {
}

Logger::PerformanceTimer::~PerformanceTimer() {
    try {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
        
        std::ostringstream msg;
        msg << "Performance: " << operation_name_ << " took " << duration.count() << " μs";
        
        Logger::getInstance().debug(msg.str());
        
    } catch (...) {
        // Silent failure in destructor
    }
}

} // namespace app