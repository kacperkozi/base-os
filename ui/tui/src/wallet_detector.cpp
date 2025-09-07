#include "app/wallet_detector.hpp"
#include "app/logger.hpp"
#include <iostream>
#include <algorithm>
#include <regex>
#include <cstring>
#include <iomanip>

#ifdef _WIN32
    #include <windows.h>
    #include <setupapi.h>
    #include <devguid.h>
    #pragma comment(lib, "setupapi.lib")
#elif defined(__APPLE__)
    #include <IOKit/IOKitLib.h>
    #include <IOKit/usb/IOUSBLib.h>
    #include <CoreFoundation/CoreFoundation.h>
#else
    #include <libusb-1.0/libusb.h>
#endif

namespace app {

WalletDetector::WalletDetector() 
    : last_scan_time_(std::chrono::steady_clock::now())
    , last_connection_attempt_(std::chrono::steady_clock::now()) {
    LOG_DEBUG("WalletDetector initialized");
}

WalletDetector::~WalletDetector() {
    stopDetection();
    LOG_DEBUG("WalletDetector destroyed");
}

bool WalletDetector::startDetection() {
    if (is_detecting_.load()) {
        LOG_WARN("Detection already running");
        return true;
    }
    
    should_stop_.store(false);
    is_detecting_.store(true);
    updateStatus(DetectionStatus::DISCONNECTED);
    
    // Perform initial device scan immediately
    LOG_INFO("Performing initial device scan...");
    updateDeviceList();
    
    try {
        detection_thread_ = std::make_unique<std::thread>(&WalletDetector::detectionLoop, this);
        LOG_INFO("Wallet detection started with 1-second polling");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to start detection thread: " + std::string(e.what()));
        is_detecting_.store(false);
        return false;
    }
}

void WalletDetector::stopDetection() {
    if (!is_detecting_.load()) {
        return;
    }
    
    should_stop_.store(true);
    
    if (detection_thread_ && detection_thread_->joinable()) {
        detection_thread_->join();
    }
    
    detection_thread_.reset();
    is_detecting_.store(false);
    updateStatus(DetectionStatus::DISCONNECTED);
    
    LOG_INFO("Wallet detection stopped");
}

std::vector<WalletDevice> WalletDetector::getDevices() const {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    return devices_;
}

WalletDevice WalletDetector::getCurrentDevice() const {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    return current_device_;
}

std::string WalletDetector::getLastError() const {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    return last_error_;
}

void WalletDetector::setPollInterval(std::chrono::milliseconds interval) noexcept {
    if (interval.count() >= 500) { // Minimum 500ms for reasonable polling
        poll_interval_.store(interval);
        LOG_DEBUG("Poll interval set to " + std::to_string(interval.count()) + "ms");
    }
}

void WalletDetector::setDeviceFoundCallback(DeviceFoundCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    device_found_callback_ = std::move(callback);
}

void WalletDetector::setStatusChangeCallback(StatusChangeCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    status_change_callback_ = std::move(callback);
}

void WalletDetector::setErrorCallback(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    error_callback_ = std::move(callback);
}

bool WalletDetector::connectToDevice(const std::string& device_path) {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    
    auto it = std::find_if(devices_.begin(), devices_.end(),
        [&device_path](const WalletDevice& device) {
            return device.path == device_path;
        });
    
    if (it == devices_.end()) {
        LOG_WARN("Device not found: " + device_path);
        return false;
    }
    
    return tryConnectToDevice(*it);
}

void WalletDetector::disconnect() {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    current_device_.connected = false;
    updateStatus(DetectionStatus::DISCONNECTED);
    LOG_INFO("Disconnected from wallet device");
}

bool WalletDetector::testConnection() {
    std::lock_guard<std::mutex> lock(devices_mutex_);
    if (!current_device_.isValid()) {
        return false;
    }
    return testDeviceConnection(current_device_);
}

void WalletDetector::detectionLoop() {
    LOG_DEBUG("Detection loop started");
    
    while (!should_stop_.load()) {
        try {
            updateDeviceList();
            
            // Just check device status, don't auto-connect
            // The status will be updated based on device detection in updateDeviceList()
            
            // Sleep for poll interval
            std::this_thread::sleep_for(poll_interval_.load());
            
        } catch (const std::exception& e) {
            LOG_ERROR("Error in detection loop: " + std::string(e.what()));
            notifyError("Detection error: " + std::string(e.what()));
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    LOG_DEBUG("Detection loop ended");
}

void WalletDetector::updateStatus(DetectionStatus new_status) {
    DetectionStatus old_status = status_.load();
    if (old_status != new_status) {
        status_.store(new_status);
        
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        if (status_change_callback_) {
            try {
                status_change_callback_(new_status);
            } catch (const std::exception& e) {
                LOG_ERROR("Error in status change callback: " + std::string(e.what()));
            }
        }
        
        LOG_DEBUG("Status changed from " + getDeviceStatusString(old_status) + 
                 " to " + getDeviceStatusString(new_status));
    }
}

void WalletDetector::notifyDeviceFound(const WalletDevice& device) {
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    if (device_found_callback_) {
        try {
            device_found_callback_(device);
        } catch (const std::exception& e) {
            LOG_ERROR("Error in device found callback: " + std::string(e.what()));
        }
    }
}

void WalletDetector::notifyError(const std::string& error) {
    {
        std::lock_guard<std::mutex> lock(devices_mutex_);
        last_error_ = error;
    }
    
    std::lock_guard<std::mutex> lock(callbacks_mutex_);
    if (error_callback_) {
        try {
            error_callback_(error);
        } catch (const std::exception& e) {
            LOG_ERROR("Error in error callback: " + std::string(e.what()));
        }
    }
}

bool WalletDetector::tryConnectToDevice(const WalletDevice& device) {
    // Just return the device's connection status without actually connecting
    // This is for compatibility with the interface
    return device.connected;
}

bool WalletDetector::testDeviceConnection(const WalletDevice& device) {
    // Test actual device connection similar to eth-signer-cpp
    if (!device.isValid()) {
        return false;
    }
    
    // Simulate connection test delay
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
#ifdef _WIN32
    // Windows: Test if device is accessible via SetupAPI
    // This is a simplified test - in practice you'd try to open the device
    return !device.path.empty() && device.path.find("VID_2C97") != std::string::npos;
    
#elif defined(__APPLE__)
    // macOS: Test if device is accessible via IOKit
    io_iterator_t iterator;
    kern_return_t result = IOServiceGetMatchingServices(kIOMainPortDefault,
                                                       IOServiceMatching("IOUSBDevice"),
                                                       &iterator);
    
    if (result == KERN_SUCCESS) {
        io_service_t service;
        while ((service = IOIteratorNext(iterator)) != 0) {
            CFNumberRef vendor_id_ref = (CFNumberRef)IORegistryEntryCreateCFProperty(
                service, CFSTR("idVendor"), kCFAllocatorDefault, 0);
            
            if (vendor_id_ref) {
                uint32_t vendor_id;
                if (CFNumberGetValue(vendor_id_ref, kCFNumberSInt32Type, &vendor_id)) {
                    if (vendor_id == 0x2c97) {
                        CFRelease(vendor_id_ref);
                        IOObjectRelease(service);
                        IOObjectRelease(iterator);
                        return true;
                    }
                }
                CFRelease(vendor_id_ref);
            }
            
            IOObjectRelease(service);
        }
        IOObjectRelease(iterator);
    }
    
    return false;
    
#else
    // Linux: Test if device is accessible via libusb
    // This is a simplified test - in practice you'd try to open the device
    libusb_context* context = nullptr;
    if (libusb_init(&context) < 0) {
        return false;
    }
    
    libusb_device** device_list;
    ssize_t device_count = libusb_get_device_list(context, &device_list);
    bool found = false;
    
    if (device_count >= 0) {
        for (ssize_t i = 0; i < device_count; i++) {
            libusb_device* usb_device = device_list[i];
            libusb_device_descriptor desc;
            
            if (libusb_get_device_descriptor(usb_device, &desc) == 0) {
                if (desc.idVendor == 0x2c97) {
                    // Try to open the device to test connection
                    libusb_device_handle* handle;
                    if (libusb_open(usb_device, &handle) == 0) {
                        libusb_close(handle);
                        found = true;
                        break;
                    }
                }
            }
        }
        libusb_free_device_list(device_list, 1);
    }
    
    libusb_exit(context);
    return found;
#endif
}

void WalletDetector::updateDeviceList() {
    auto new_devices = scanForDevices();
    
    {
        std::lock_guard<std::mutex> lock(devices_mutex_);
        
        // Check for new devices
        for (const auto& new_device : new_devices) {
            auto it = std::find(devices_.begin(), devices_.end(), new_device);
            if (it == devices_.end()) {
                LOG_INFO("New device detected: " + new_device.product);
                notifyDeviceFound(new_device);
                
                // If this is a connected Ledger device, update status
                if (new_device.connected && isLedgerDevice(new_device.manufacturer, new_device.product)) {
                    updateStatus(DetectionStatus::CONNECTED);
                    // Set as current device
                    current_device_ = new_device;
                }
            }
        }
        
        // Check for removed devices
        for (auto it = devices_.begin(); it != devices_.end();) {
            auto new_it = std::find(new_devices.begin(), new_devices.end(), *it);
            if (new_it == new_devices.end()) {
                LOG_INFO("Device removed: " + it->product);
                if (current_device_.path == it->path) {
                    current_device_.connected = false;
                    updateStatus(DetectionStatus::DISCONNECTED);
                }
                it = devices_.erase(it);
            } else {
                ++it;
            }
        }
        
        // Update device list
        devices_ = std::move(new_devices);
        
        // Check if we still have any connected Ledger devices
        bool has_connected_ledger = false;
        for (const auto& device : devices_) {
            if (device.connected && isLedgerDevice(device.manufacturer, device.product)) {
                has_connected_ledger = true;
                break;
            }
        }
        
        // Update status based on connected devices
        if (!has_connected_ledger && status_.load() == DetectionStatus::CONNECTED) {
            updateStatus(DetectionStatus::DISCONNECTED);
        }
    }
}

std::vector<WalletDevice> WalletDetector::scanForDevices() {
    std::vector<WalletDevice> devices;
    
#ifdef _WIN32
    // Windows implementation using SetupAPI
    HDEVINFO device_info_set = SetupDiGetClassDevs(&GUID_DEVCLASS_USB, nullptr, nullptr, 
                                                   DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (device_info_set == INVALID_HANDLE_VALUE) {
        LOG_ERROR("Failed to get USB device list on Windows");
        return devices;
    }
    
    SP_DEVINFO_DATA device_info_data;
    device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);
    
    for (DWORD i = 0; SetupDiEnumDeviceInfo(device_info_set, i, &device_info_data); i++) {
        char device_id[256];
        if (SetupDiGetDeviceInstanceId(device_info_set, &device_info_data, device_id, 
                                      sizeof(device_id), nullptr)) {
            std::string device_id_str(device_id);
            
            // Check for Ledger devices (VID 0x2c97)
            if (device_id_str.find("VID_2C97") != std::string::npos) {
                WalletDevice device;
                device.path = device_id_str;
                device.manufacturer = "Ledger";
                
                // Extract product name
                size_t pid_pos = device_id_str.find("PID_");
                if (pid_pos != std::string::npos) {
                    size_t start = pid_pos + 4;
                    size_t end = device_id_str.find('&', start);
                    if (end == std::string::npos) end = device_id_str.length();
                    
                    std::string pid = device_id_str.substr(start, end - start);
                    if (pid == "0001") device.product = "Ledger Nano S";
                    else if (pid == "0004") device.product = "Ledger Nano X";
                    else if (pid == "0005") device.product = "Ledger Nano S Plus";
                    else device.product = "Ledger Device";
                } else {
                    device.product = "Ledger Device";
                }
                
                devices.push_back(device);
            }
        }
    }
    
    SetupDiDestroyDeviceInfoList(device_info_set);
    
#elif defined(__APPLE__)
    // macOS implementation using IOKit (improved)
    io_iterator_t iterator;
    kern_return_t result = IOServiceGetMatchingServices(kIOMainPortDefault,
                                                       IOServiceMatching("IOUSBDevice"),
                                                       &iterator);
    
    if (result == KERN_SUCCESS) {
        io_service_t service;
        while ((service = IOIteratorNext(iterator)) != 0) {
            // Get vendor ID
            CFNumberRef vendor_id_ref = (CFNumberRef)IORegistryEntryCreateCFProperty(
                service, CFSTR("idVendor"), kCFAllocatorDefault, 0);
            
            if (vendor_id_ref) {
                uint32_t vendor_id;
                if (CFNumberGetValue(vendor_id_ref, kCFNumberSInt32Type, &vendor_id)) {
                    // Check for Ledger devices (VID 0x2c97)
                    if (vendor_id == 0x2c97) {
                        WalletDevice device;
                        device.manufacturer = "Ledger";
                        
                        // Get product name
                        CFStringRef product = (CFStringRef)IORegistryEntryCreateCFProperty(
                            service, CFSTR("USB Product Name"), kCFAllocatorDefault, 0);
                        
                        if (product) {
                            char product_cstr[256];
                            if (CFStringGetCString(product, product_cstr, sizeof(product_cstr), 
                                                  kCFStringEncodingUTF8)) {
                                device.product = product_cstr;
                            } else {
                                device.product = "Unknown Ledger Device";
                            }
                            CFRelease(product);
                        } else {
                            device.product = "Unknown Ledger Device";
                        }
                        
                        // Get product ID for path
                        CFNumberRef product_id_ref = (CFNumberRef)IORegistryEntryCreateCFProperty(
                            service, CFSTR("idProduct"), kCFAllocatorDefault, 0);
                        
                        if (product_id_ref) {
                            uint32_t product_id;
                            if (CFNumberGetValue(product_id_ref, kCFNumberSInt32Type, &product_id)) {
                                // Create device path (same format as eth-signer-cpp)
                                std::stringstream path_stream;
                                path_stream << std::hex << std::setfill('0') 
                                           << std::setw(4) << vendor_id << ":"
                                           << std::setw(4) << product_id;
                                device.path = path_stream.str();
                            }
                            CFRelease(product_id_ref);
                        }
                        
                        // Mark as connected (device is accessible via IOKit)
                        device.connected = true;
                        devices.push_back(device);
                    }
                }
                CFRelease(vendor_id_ref);
            }
            
            IOObjectRelease(service);
        }
        IOObjectRelease(iterator);
    }
    
#else
    // Linux implementation using libusb
    libusb_context* context = nullptr;
    if (libusb_init(&context) < 0) {
        LOG_ERROR("Failed to initialize libusb");
        return devices;
    }
    
    libusb_device** device_list;
    ssize_t device_count = libusb_get_device_list(context, &device_list);
    
    if (device_count >= 0) {
        for (ssize_t i = 0; i < device_count; i++) {
            libusb_device* device = device_list[i];
            libusb_device_descriptor desc;
            
            if (libusb_get_device_descriptor(device, &desc) == 0) {
                // Check for Ledger devices (VID 0x2c97)
                if (desc.idVendor == 0x2c97) {
                    WalletDevice wallet_device;
                    wallet_device.manufacturer = "Ledger";
                    
                    // Get product name and test connection
                    libusb_device_handle* handle;
                    if (libusb_open(device, &handle) == 0) {
                        char product[256];
                        if (libusb_get_string_descriptor_ascii(handle, desc.iProduct, 
                                                              (unsigned char*)product, 
                                                              sizeof(product)) > 0) {
                            wallet_device.product = product;
                        } else {
                            wallet_device.product = "Ledger Device";
                        }
                        // Device is accessible, mark as connected
                        wallet_device.connected = true;
                        libusb_close(handle);
                    } else {
                        wallet_device.product = "Ledger Device";
                        wallet_device.connected = false;
                    }
                    
                    // Create device path
                    std::stringstream path_stream;
                    path_stream << std::hex << std::setfill('0') 
                               << std::setw(4) << desc.idVendor << ":"
                               << std::setw(4) << desc.idProduct;
                    wallet_device.path = path_stream.str();
                    
                    devices.push_back(wallet_device);
                }
            }
        }
        libusb_free_device_list(device_list, 1);
    }
    
    libusb_exit(context);
#endif
    
    return devices;
}

bool WalletDetector::isLedgerDevice(const std::string& manufacturer, const std::string& product) {
    std::string manufacturer_lower = manufacturer;
    std::string product_lower = product;
    
    std::transform(manufacturer_lower.begin(), manufacturer_lower.end(), 
                   manufacturer_lower.begin(), ::tolower);
    std::transform(product_lower.begin(), product_lower.end(), 
                   product_lower.begin(), ::tolower);
    
    return manufacturer_lower.find("ledger") != std::string::npos ||
           product_lower.find("nano") != std::string::npos ||
           product_lower.find("ledger") != std::string::npos;
}

std::string WalletDetector::getDeviceStatusString(DetectionStatus status) {
    switch (status) {
        case DetectionStatus::DISCONNECTED: return "DISCONNECTED";
        case DetectionStatus::CONNECTING: return "CONNECTING";
        case DetectionStatus::CONNECTED: return "CONNECTED";
        case DetectionStatus::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

} // namespace app
