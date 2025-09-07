#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>

namespace app {

// Device information structure
struct WalletDevice {
    std::string path;
    std::string manufacturer;
    std::string product;
    std::string serial_number;
    bool connected = false;
    bool app_open = false;
    std::string version;
    
    bool isValid() const noexcept {
        return !path.empty() && !product.empty();
    }
    
    bool operator==(const WalletDevice& other) const noexcept {
        return path == other.path && product == other.product;
    }
};

// Wallet detection status
enum class DetectionStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR
};

// Wallet detector class for continuous USB device monitoring
class WalletDetector {
public:
    // Callback types
    using DeviceFoundCallback = std::function<void(const WalletDevice&)>;
    using StatusChangeCallback = std::function<void(DetectionStatus)>;
    using ErrorCallback = std::function<void(const std::string&)>;
    
    WalletDetector();
    ~WalletDetector();
    
    // Start/stop detection
    bool startDetection();
    void stopDetection();
    bool isDetecting() const noexcept { return is_detecting_.load(); }
    
    // Current status
    DetectionStatus getStatus() const noexcept { return status_.load(); }
    std::vector<WalletDevice> getDevices() const;
    WalletDevice getCurrentDevice() const;
    std::string getLastError() const;
    
    // Configuration
    void setPollInterval(std::chrono::milliseconds interval) noexcept;
    void setAutoConnect(bool auto_connect) noexcept { auto_connect_ = auto_connect; }
    
    // Callbacks
    void setDeviceFoundCallback(DeviceFoundCallback callback);
    void setStatusChangeCallback(StatusChangeCallback callback);
    void setErrorCallback(ErrorCallback callback);
    
    // Manual operations
    bool connectToDevice(const std::string& device_path);
    void disconnect();
    bool testConnection();
    
    // Utility methods
    static std::vector<WalletDevice> scanForDevices();
    static bool isLedgerDevice(const std::string& manufacturer, const std::string& product);
    static std::string getDeviceStatusString(DetectionStatus status);

private:
    // Detection thread
    void detectionLoop();
    void updateStatus(DetectionStatus new_status);
    void notifyDeviceFound(const WalletDevice& device);
    void notifyError(const std::string& error);
    
    // Device operations
    bool tryConnectToDevice(const WalletDevice& device);
    bool testDeviceConnection(const WalletDevice& device);
    void updateDeviceList();
    
    // Thread safety
    mutable std::mutex devices_mutex_;
    mutable std::mutex callbacks_mutex_;
    
    // State
    std::atomic<bool> is_detecting_{false};
    std::atomic<bool> should_stop_{false};
    std::atomic<DetectionStatus> status_{DetectionStatus::DISCONNECTED};
    std::atomic<std::chrono::milliseconds> poll_interval_{std::chrono::milliseconds(1000)}; // 1 second
    std::atomic<bool> auto_connect_{true};
    
    // Device data
    std::vector<WalletDevice> devices_;
    WalletDevice current_device_;
    std::string last_error_;
    
    // Threading
    std::unique_ptr<std::thread> detection_thread_;
    
    // Callbacks
    DeviceFoundCallback device_found_callback_;
    StatusChangeCallback status_change_callback_;
    ErrorCallback error_callback_;
    
    // Timing
    std::chrono::steady_clock::time_point last_scan_time_;
    std::chrono::steady_clock::time_point last_connection_attempt_;
};

} // namespace app
