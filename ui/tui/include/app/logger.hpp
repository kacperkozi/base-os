#pragma once
#include <string>
#include <memory>
#include <fstream>
#include <mutex>
#include <sstream>
#include <chrono>
#include <thread>

namespace app {

/**
 * Thread-safe logging system with configurable levels and output.
 * Supports both file and console output with timestamp formatting.
 */
class Logger {
public:
    enum class Level {
        TRACE = 0,
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5
    };

    // Singleton pattern for global access
    static Logger& getInstance();
    
    // Initialize logger with file path and level
    bool initialize(const std::string& log_file_path, Level min_level = Level::INFO, 
                   bool console_output = true, size_t max_file_size_mb = 10);
    
    // Cleanup resources
    void shutdown() noexcept;
    
    // Main logging methods
    void log(Level level, const std::string& message, 
             const char* file = __FILE__, 
             int line = __LINE__,
             const char* function = nullptr) noexcept;
    
    // Convenience methods
    void trace(const std::string& message, const char* file = __builtin_FILE(), 
               int line = __builtin_LINE(), const char* function = __builtin_FUNCTION()) noexcept {
        log(Level::TRACE, message, file, line, function);
    }
    
    void debug(const std::string& message, const char* file = __builtin_FILE(), 
               int line = __builtin_LINE(), const char* function = __builtin_FUNCTION()) noexcept {
        log(Level::DEBUG, message, file, line, function);
    }
    
    void info(const std::string& message, const char* file = __builtin_FILE(), 
              int line = __builtin_LINE(), const char* function = __builtin_FUNCTION()) noexcept {
        log(Level::INFO, message, file, line, function);
    }
    
    void warn(const std::string& message, const char* file = __builtin_FILE(), 
              int line = __builtin_LINE(), const char* function = __builtin_FUNCTION()) noexcept {
        log(Level::WARN, message, file, line, function);
    }
    
    void error(const std::string& message, const char* file = __builtin_FILE(), 
               int line = __LINE__, const char* function = __builtin_FUNCTION()) noexcept {
        log(Level::ERROR, message, file, line, function);
    }
    
    void fatal(const std::string& message, const char* file = __builtin_FILE(), 
               int line = __builtin_LINE(), const char* function = __builtin_FUNCTION()) noexcept {
        log(Level::FATAL, message, file, line, function);
    }
    
    // Configuration
    void setLevel(Level min_level) noexcept { min_level_ = min_level; }
    Level getLevel() const noexcept { return min_level_; }
    
    void setConsoleOutput(bool enable) noexcept { console_output_ = enable; }
    bool getConsoleOutput() const noexcept { return console_output_; }
    
    // Utilities
    static std::string levelToString(Level level) noexcept;
    static Level stringToLevel(const std::string& level_str) noexcept;
    
    // Performance monitoring
    struct PerformanceTimer {
        PerformanceTimer(const std::string& operation_name);
        ~PerformanceTimer();
        
    private:
        std::string operation_name_;
        std::chrono::high_resolution_clock::time_point start_time_;
    };
    
    // Create performance timer for RAII timing
    std::unique_ptr<PerformanceTimer> createTimer(const std::string& operation_name);
    
    // Safe exception logging
    void logException(const std::exception& e, const std::string& context = "",
                     const char* file = __builtin_FILE(), 
                     int line = __builtin_LINE(),
                     const char* function = __builtin_FUNCTION()) noexcept;

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // Thread-safe file writing
    mutable std::mutex mutex_;
    std::ofstream log_file_;
    std::string log_file_path_;
    Level min_level_ = Level::INFO;
    bool console_output_ = true;
    bool initialized_ = false;
    size_t max_file_size_bytes_ = 10 * 1024 * 1024; // 10MB default
    size_t current_file_size_ = 0;
    
    // Internal helpers
    std::string formatMessage(Level level, const std::string& message,
                             const char* file, int line, const char* function) const noexcept;
    
    std::string getCurrentTimestamp() const noexcept;
    void writeToFile(const std::string& formatted_message) noexcept;
    void writeToConsole(const std::string& formatted_message) noexcept;
    void rotateLogFileIfNeeded() noexcept;
    std::string extractFileName(const char* file_path) const noexcept;
};

} // namespace app

// Convenient macros for logging
#define LOG_TRACE(msg) app::Logger::getInstance().trace(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_DEBUG(msg) app::Logger::getInstance().debug(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_INFO(msg) app::Logger::getInstance().info(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_WARN(msg) app::Logger::getInstance().warn(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_ERROR(msg) app::Logger::getInstance().error(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_FATAL(msg) app::Logger::getInstance().fatal(msg, __FILE__, __LINE__, __FUNCTION__)

#define LOG_EXCEPTION(e, context) app::Logger::getInstance().logException(e, context, __FILE__, __LINE__, __FUNCTION__)

// Performance timing macro
#define PERF_TIMER(name) auto timer_##__LINE__ = app::Logger::getInstance().createTimer(name)