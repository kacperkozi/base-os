#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/component/event.hpp>
#include "app/state.hpp"
#include "app/wallet_detector.hpp"
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>
#include <memory>

using namespace ftxui;

namespace app {

// Wallet detection view component
class WalletDetectionView {
public:
    WalletDetectionView(std::shared_ptr<AppState> state) 
        : state_(state), detector_(std::make_unique<WalletDetector>()) {
        setupDetector();
    }
    
    ~WalletDetectionView() {
        if (detector_) {
            detector_->stopDetection();
        }
    }
    
    Component render() {
        return Container::Vertical({
            renderHeader(),
            renderStatus(),
            renderDevices(),
            renderInstructions(),
            renderControls()
        });
    }
    
private:
    std::shared_ptr<AppState> state_;
    std::unique_ptr<WalletDetector> detector_;
    std::vector<WalletDevice> devices_;
    DetectionStatus current_status_ = DetectionStatus::DISCONNECTED;
    std::string last_error_;
    int animation_frame_ = 0;
    bool show_help_ = false;
    
    void setupDetector() {
        detector_->setDeviceFoundCallback([this](const WalletDevice& device) {
            devices_.push_back(device);
            if (state_) {
                state_->setStatus("Device found: " + device.product);
            }
        });
        
        detector_->setStatusChangeCallback([this](DetectionStatus status) {
            current_status_ = status;
            if (state_) {
                switch (status) {
                    case DetectionStatus::CONNECTED:
                        state_->setStatus("Wallet connected successfully");
                        state_->setWalletConnected(true);
                        break;
                    case DetectionStatus::CONNECTING:
                        state_->setStatus("Connecting to wallet...");
                        break;
                    case DetectionStatus::DISCONNECTED:
                        state_->setStatus("No wallet detected");
                        state_->setWalletConnected(false);
                        break;
                    case DetectionStatus::ERROR:
                        state_->setStatus("Connection error");
                        break;
                }
            }
        });
        
        detector_->setErrorCallback([this](const std::string& error) {
            last_error_ = error;
            if (state_) {
                state_->setError(error);
            }
        });
        
        // Start detection
        detector_->startDetection();
    }
    
    Component renderHeader() {
        return Renderer([] {
            return vbox({
                text("╔══════════════════════════════════════════════════════════════╗") | color(Color::Blue),
                text("║                    🔍 WALLET DETECTOR                       ║") | color(Color::Blue),
                text("║              Based on eth-signer-cpp Architecture           ║") | color(Color::Blue),
                text("╚══════════════════════════════════════════════════════════════╝") | color(Color::Blue),
                text("") | flex,
            });
        });
    }
    
    Component renderStatus() {
        return Renderer([this] {
            std::string status_text;
            Color status_color;
            
            switch (current_status_) {
                case DetectionStatus::CONNECTED:
                    status_text = "🟢 WALLET CONNECTED";
                    status_color = Color::Green;
                    break;
                case DetectionStatus::CONNECTING:
                    status_text = "🟡 CONNECTING...";
                    status_color = Color::Yellow;
                    break;
                case DetectionStatus::DISCONNECTED:
                    status_text = "🔴 NO WALLET DETECTED";
                    status_color = Color::Red;
                    break;
                case DetectionStatus::ERROR:
                    status_text = "❌ CONNECTION ERROR";
                    status_color = Color::Red;
                    break;
            }
            
            auto status_box = vbox({
                text("📊 STATUS:") | bold,
                text("┌─────────────────────────────────────────────────────────────┐"),
                text("│ " + status_text) | color(status_color),
                text("│                                                             │"),
                text("│ Last checked: " + getCurrentTimeString()),
                text("└─────────────────────────────────────────────────────────────┘"),
                text("") | flex,
            });
            
            if (!last_error_.empty()) {
                status_box = vbox({
                    status_box,
                    text("❌ Error: " + last_error_) | color(Color::Red),
                    text("") | flex,
                });
            }
            
            return status_box;
        });
    }
    
    Component renderDevices() {
        return Renderer([this] {
            auto devices = detector_->getDevices();
            auto current_device = detector_->getCurrentDevice();
            
            Elements device_elements;
            
            if (devices.empty()) {
                device_elements.push_back(text("│ No USB devices found                                    │"));
            } else {
                for (const auto& device : devices) {
                    std::string device_text = "│ " + device.product;
                    if (device.path == current_device.path && current_device.connected) {
                        device_text += " (CONNECTED)";
                    }
                    device_text += std::string(60 - device_text.length(), ' ') + "│";
                    device_elements.push_back(text(device_text));
                }
            }
            
            return vbox({
                text("🔌 DETECTED DEVICES:") | bold,
                text("┌─────────────────────────────────────────────────────────────┐"),
                device_elements,
                text("└─────────────────────────────────────────────────────────────┘"),
                text("") | flex,
            });
        });
    }
    
    Component renderInstructions() {
        return Renderer([this] {
            return vbox({
                text("📋 INSTRUCTIONS:") | bold,
                text("┌─────────────────────────────────────────────────────────────┐"),
                text("│ 1. Connect your Ledger device via USB                      │"),
                text("│ 2. Open the Ethereum app on your Ledger                    │"),
                text("│ 3. Enable \"Blind signing\" in the Ethereum app settings     │"),
                text("│ 4. Wait for the device to be detected                      │"),
                text("│                                                             │"),
                text("│ Press 'h' for help, 'q' to quit, 'r' to refresh            │"),
                text("└─────────────────────────────────────────────────────────────┘"),
                text("") | flex,
            });
        });
    }
    
    Component renderControls() {
        return Renderer([this] {
            return vbox({
                text("⌨️  CONTROLS:") | bold,
                text("┌─────────────────────────────────────────────────────────────┐"),
                text("│ h - Show/Hide help                                         │"),
                text("│ r - Refresh device list                                    │"),
                text("│ q - Quit application                                       │"),
                text("│ Enter - Continue (when wallet connected)                   │"),
                text("└─────────────────────────────────────────────────────────────┘"),
                text("") | flex,
            });
        });
    }
    
    std::string getCurrentTimeString() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);
        
        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &tm);
        return std::string(buffer);
    }
};

// Main wallet detection screen
Component CreateWalletDetectionScreen(std::shared_ptr<AppState> state) {
    auto view = std::make_shared<WalletDetectionView>(state);
    
    return Renderer([view] {
        return view->render();
    }) | CatchEvent([view, state](Event event) {
        if (event == Event::Character('q') || event == Event::Escape) {
            if (state) {
                state->requestShutdown();
            }
            return true;
        } else if (event == Event::Character('r')) {
            // Refresh devices
            if (view) {
                // Trigger device refresh
                return true;
            }
        } else if (event == Event::Character('h')) {
            // Toggle help
            if (view) {
                // Toggle help display
                return true;
            }
        } else if (event == Event::Return) {
            // Check if wallet is connected and proceed
            if (state && state->isWalletConnected()) {
                state->setRoute(Route::TransactionInput);
                return true;
            }
        }
        return false;
    });
}

} // namespace app
