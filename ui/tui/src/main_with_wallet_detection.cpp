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
#include <iostream>

using namespace ftxui;

// Forward declarations
Component CreateWalletDetectionScreen(std::shared_ptr<app::AppState> state);
Component CreateTransactionInputScreen(std::shared_ptr<app::AppState> state);
Component CreateConfirmationScreen(std::shared_ptr<app::AppState> state);
Component CreateResultScreen(std::shared_ptr<app::AppState> state);

// Main application with wallet detection
int RunWalletDetectionApp() {
    // Create shared application state
    auto state = std::make_shared<app::AppState>();
    
    // Initialize state
    state->loadFromConfig();
    state->setRoute(app::Route::ConnectWallet);
    
    // Create screen
    auto screen = ScreenInteractive::Fullscreen();
    
    // Wallet detection status
    bool wallet_connected = false;
    std::string wallet_status = "Checking for devices...";
    
    // Create wallet detector
    auto detector = std::make_unique<app::WalletDetector>();
    
    // Set up detector callbacks
    detector->setStatusChangeCallback([&](app::DetectionStatus status) {
        switch (status) {
            case app::DetectionStatus::CONNECTED:
                wallet_connected = true;
                wallet_status = "Ledger device detected!";
                state->setWalletConnected(true);
                state->setStatus("Ledger device detected and accessible");
                break;
            case app::DetectionStatus::CONNECTING:
                wallet_connected = false;
                wallet_status = "Scanning for devices...";
                state->setWalletConnected(false);
                state->setStatus("Scanning for USB devices (every 1 second)...");
                break;
            case app::DetectionStatus::DISCONNECTED:
                wallet_connected = false;
                wallet_status = "No Ledger device detected";
                state->setWalletConnected(false);
                state->setStatus("No Ledger device detected");
                break;
            case app::DetectionStatus::ERROR:
                wallet_connected = false;
                wallet_status = "Device detection error";
                state->setWalletConnected(false);
                state->setError("Error detecting devices");
                break;
        }
    });
    
    detector->setDeviceFoundCallback([&](const app::WalletDevice& device) {
        if (device.connected) {
            state->setInfo("Ledger device detected: " + device.product);
        } else {
            state->setInfo("Device found but not accessible: " + device.product);
        }
    });
    
    detector->setErrorCallback([&](const std::string& error) {
        state->setError(error);
    });
    
    // Start wallet detection
    detector->startDetection();
    
    // Main render function
    auto render_main = [&]() -> Element {
        switch (state->getRoute()) {
            case app::Route::ConnectWallet:
                return CreateWalletDetectionScreen(state)->Render();
            case app::Route::TransactionInput:
                return CreateTransactionInputScreen(state)->Render();
            case app::Route::Confirmation:
                return CreateConfirmationScreen(state)->Render();
            case app::Route::Result:
                return CreateResultScreen(state)->Render();
            default:
                return CreateWalletDetectionScreen(state)->Render();
        }
    };
    
    // Main component
    auto main_component = Renderer(render_main);
    
    // Event handling
    auto event_handler = CatchEvent(main_component, [&](Event event) {
        // Global event handling
        if (event == Event::Character('q') || event == Event::Escape) {
            if (state->getRoute() == app::Route::ConnectWallet) {
                screen.ExitLoopClosure()();
                return true;
            } else {
                // Go back to wallet detection
                state->setRoute(app::Route::ConnectWallet);
                return true;
            }
        }
        
        // Route-specific event handling
        switch (state->getRoute()) {
            case app::Route::ConnectWallet:
                if (event == Event::Return && wallet_connected) {
                    state->setRoute(app::Route::TransactionInput);
                    return true;
                }
                break;
                
            case app::Route::TransactionInput:
                if (event == Event::Return) {
                    // Validate transaction and proceed to confirmation
                    state->setRoute(app::Route::Confirmation);
                    return true;
                }
                break;
                
            case app::Route::Confirmation:
                if (event == Event::Return) {
                    // Sign transaction and proceed to result
                    state->setRoute(app::Route::Result);
                    return true;
                }
                break;
                
            case app::Route::Result:
                if (event == Event::Return) {
                    // Start new transaction
                    state->clearTransaction();
                    state->setRoute(app::Route::TransactionInput);
                    return true;
                }
                break;
                
            case app::Route::USBContacts:
            case app::Route::Signing:
            case app::Route::Settings:
            case app::Route::Help:
            case app::Route::Error:
                // Handle other routes if needed
                break;
        }
        
        return false;
    });
    
    // Animation thread for status updates
    std::thread animation_thread([&] {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            state->incrementAnimationFrame();
            screen.PostEvent(Event::Custom);
        }
    });
    
    // Run the application
    screen.Loop(event_handler);
    
    // Cleanup
    detector->stopDetection();
    animation_thread.join();
    
    return 0;
}

// Wallet Detection Screen
Component CreateWalletDetectionScreen(std::shared_ptr<app::AppState> state) {
    return Renderer([state]() -> Element {
        auto devices = std::vector<app::WalletDevice>(); // Would get from detector
        auto current_device = app::WalletDevice();
        auto status = state->isWalletConnected() ? "CONNECTED" : "DISCONNECTED";
        auto status_color = state->isWalletConnected() ? Color::Green : Color::Red;
        auto status_icon = state->isWalletConnected() ? "🟢" : "🔴";
        
        return vbox({
            // Header
            text("╔══════════════════════════════════════════════════════════════╗") | color(Color::Blue),
            text("║                    🔍 WALLET DETECTOR                       ║") | color(Color::Blue),
            text("║              Based on eth-signer-cpp Architecture           ║") | color(Color::Blue),
            text("╚══════════════════════════════════════════════════════════════╝") | color(Color::Blue),
            text("") | flex,
            
            // Status
            text("📊 STATUS:") | bold,
            text("┌─────────────────────────────────────────────────────────────┐"),
            text("│ " + std::string(status_icon) + " WALLET " + status) | color(status_color),
            text("│                                                             │"),
            text("│ " + state->getStatus()),
            text("└─────────────────────────────────────────────────────────────┘"),
            text("") | flex,
            
            // Instructions
            text("📋 INSTRUCTIONS:") | bold,
            text("┌─────────────────────────────────────────────────────────────┐"),
            text("│ 1. Connect your Ledger device via USB                      │"),
            text("│ 2. Open the Ethereum app on your Ledger                    │"),
            text("│ 3. Enable \"Blind signing\" in the Ethereum app settings     │"),
            text("│ 4. Device detection runs every 1 second                    │"),
            text("│                                                             │"),
            text("│ Press Enter to continue (when device detected)             │"),
            text("│ Press 'q' to quit                                          │"),
            text("└─────────────────────────────────────────────────────────────┘"),
            text("") | flex,
            
            // Error display
            state->getError().empty() ? text("") : 
            vbox({
                text("❌ Error: " + state->getError()) | color(Color::Red),
                text("") | flex,
            }),
        }) | center;
    });
}

// Transaction Input Screen (simplified)
Component CreateTransactionInputScreen(std::shared_ptr<app::AppState> state) {
    return Renderer([state]() -> Element {
        return vbox({
            text("╔══════════════════════════════════════════════════════════════╗") | color(Color::Green),
            text("║                  📝 TRANSACTION INPUT                       ║") | color(Color::Green),
            text("╚══════════════════════════════════════════════════════════════╝") | color(Color::Green),
            text("") | flex,
            text("✅ Wallet connected! Ready to create transaction.") | color(Color::Green),
            text("") | flex,
            text("Press Enter to continue to confirmation...") | dim,
            text("Press 'q' to go back") | dim,
        }) | center;
    });
}

// Confirmation Screen (simplified)
Component CreateConfirmationScreen(std::shared_ptr<app::AppState> state) {
    return Renderer([state]() -> Element {
        return vbox({
            text("╔══════════════════════════════════════════════════════════════╗") | color(Color::Yellow),
            text("║                  ⚠️  CONFIRMATION                           ║") | color(Color::Yellow),
            text("╚══════════════════════════════════════════════════════════════╝") | color(Color::Yellow),
            text("") | flex,
            text("🔐 Ready to sign transaction with your Ledger device.") | color(Color::Yellow),
            text("") | flex,
            text("Press Enter to sign transaction...") | dim,
            text("Press 'q' to go back") | dim,
        }) | center;
    });
}

// Result Screen (simplified)
Component CreateResultScreen(std::shared_ptr<app::AppState> state) {
    return Renderer([state]() -> Element {
        return vbox({
            text("╔══════════════════════════════════════════════════════════════╗") | color(Color::Green),
            text("║                    🎉 SUCCESS!                              ║") | color(Color::Green),
            text("╚══════════════════════════════════════════════════════════════╝") | color(Color::Green),
            text("") | flex,
            text("✅ Transaction signed successfully!") | color(Color::Green),
            text("") | flex,
            text("Press Enter to create another transaction...") | dim,
            text("Press 'q' to quit") | dim,
        }) | center;
    });
}
