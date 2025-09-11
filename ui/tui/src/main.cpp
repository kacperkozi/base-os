#include <cstdlib>
#include <iostream>
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/component.hpp>
#include "app/state.hpp"
#include "app/router.hpp"

using namespace ftxui;

int main(int argc, char* argv[]) {
  auto state = std::make_shared<app::AppState>();

  // Handle command-line arguments
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--dev") {
      state->setDevMode(true);
      state->setWalletConnected(true); // Auto-connect mock wallet
    } else if (arg == "--version" || arg == "-v") {
      std::cout << "Base OS TUI v1.0.0" << std::endl;
      std::cout << "Simple Ethereum transaction interface" << std::endl;
      return 0;
    } else if (arg == "--help" || arg == "-h") {
      std::cout << "Base OS TUI - Simple Ethereum Transaction Interface" << std::endl;
      std::cout << std::endl;
      std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
      std::cout << std::endl;
      std::cout << "Options:" << std::endl;
      std::cout << "  --dev            Enable development mode with a mock wallet" << std::endl;
      std::cout << "  --version, -v    Show version information" << std::endl;
      std::cout << "  --help, -h       Show this help message" << std::endl;
      std::cout << std::endl;
      std::cout << "Controls:" << std::endl;
      std::cout << "  e                Toggle edit mode in forms" << std::endl;
      std::cout << "  Tab / Arrows     Navigate between fields" << std::endl;
      std::cout << "  Enter            Submit/Continue" << std::endl;
      std::cout << "  Escape           Go back" << std::endl;
      std::cout << "  1-5              Jump to screen" << std::endl;
      std::cout << "  h/?              Show help" << std::endl;
      std::cout << "  s                Settings" << std::endl;
      std::cout << "  q / Ctrl+C       Quit" << std::endl;
      return 0;
    }
  }
  
  // Create and run the screen
  auto screen = ScreenInteractive::Fullscreen();
  
  // Create the router component with screen for refresh
  auto router = app::MakeRouter(*state);
  
  // Use FTXUI's Task system for animation updates instead of raw threads
  std::atomic<bool> running{true};
  auto animation_task = [&screen, &state, &running]() {
    if (!running.load()) return;
    state->incrementAnimationFrame();
    screen.PostEvent(Event::Custom);
  };
  
  // Schedule periodic animation updates using FTXUI's event system
  std::thread animation_thread([animation_task, &running]() {
    while (running.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      animation_task();
    }
  });
  
  // The router now handles the full UI including header/footer
  // Handle ALL events at the top level and dispatch based on route and mode
  auto final_component = router | CatchEvent([state, &screen, &running](Event event) {
    auto ui_state = state->getUIState();
    auto old_route = ui_state.route;
    
    // Handle 'q' key for quit (always works)
    if (event.is_character() && event.character() == "q") {
      running.store(false);
      screen.Exit();
      return true;
    }
    
    // In Edit Mode: Let input fields handle ALL character input including numbers
    if (ui_state.edit_mode) {
      // Only allow specific control keys in edit mode, block all number navigation
      if (event.is_character()) {
        std::string key_str = event.character();
        if (!key_str.empty()) {
          char key = key_str[0];
          // Block ALL number keys (0-9) when in edit mode to prevent navigation conflicts
          if (key >= '0' && key <= '9') {
            return false; // Let input fields handle numbers
          }
          // Allow 'e' to toggle edit mode and 'q' to quit
          if (key == 'e' || key == 'q') {
            // These will be handled by the global handlers below
            // Fall through to global handling
          } else {
            // Let input fields handle all other characters
            return false;
          }
        }
      }
      
      // Allow Escape and Enter in edit mode
      if (event == Event::Escape || event == Event::Return) {
        return false; // Let input fields or global handlers manage these
      }
      
      // For all other events in edit mode, let input fields handle first
      return false;
    }
    
    // Handle 'e' key to toggle between navigation and edit mode
    // This only runs when NOT in edit mode, or when input fields didn't consume the event
    if (event.is_character() && event.character() == "e") {
      state->toggleEditMode();
      auto new_ui_state = state->getUIState();
      state->setStatus(new_ui_state.edit_mode ? "Edit Mode - Press 'e' to return to Navigation" : "Navigation Mode - Press 'e' to enter Edit");
      return true;
    }
    
    // In Navigation Mode: Handle navigation keys
    if (event.is_character()) {
      auto route = state->getRoute();
      std::string key_str = event.character();
      if (key_str.empty()) return false;
      char key = key_str[0];
      
      // Handle 'd' key for wallet detection on Connect Wallet screen
      if (key == 'd' && route == app::Route::ConnectWallet) {
        auto device_state = state->getDeviceState();
        auto ui_state = state->getUIState();
        if (!ui_state.is_detecting_wallet && !device_state.wallet_connected) {
          state->setDetectingWallet(true);
        // Use PostEvent for async operations instead of raw threads
        state->setDetectingWallet(true);
        auto wallet_detection_task = [state, &screen]() {
          // Simulate wallet detection process
          for (int i = 0; i < 20; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            state->incrementAnimationFrame();
            screen.PostEvent(Event::Custom); // Trigger UI update
          }
          state->setDetectingWallet(false);
          state->setWalletConnected(true);
          state->setStatus("Wallet connected, navigating...");
          // Automatically navigate on success
          std::this_thread::sleep_for(std::chrono::milliseconds(500));
          state->setRoute(app::Route::USBContacts);
          screen.PostEvent(Event::Custom); // Final UI update
        };
        std::thread(wallet_detection_task).detach();
          return true;
        }
      }
      
      // Number keys for navigation
      switch(key) {
        case '1':
          state->setRoute(app::Route::ConnectWallet);
          break;
        case '2':
          state->setRoute(app::Route::USBContacts);
          break;
        case '3':
          state->setRoute(app::Route::TransactionInput);
          break;
        case '4':
          state->setRoute(app::Route::Confirmation);
          break;
        case '5':
          // This key is deprecated in favor of the button on the confirmation screen
          // but kept for quick access during development.
          state->setRoute(app::Route::Signing);
          break;
        case 'h':
        case '?':
          state->setRoute(app::Route::Help);
          break;
        case 's':
          state->setRoute(app::Route::Settings);
          break;
        default:
            return false; // Do not consume unhandled keys
      }
    } else if (event == Event::Escape) {
      // Handle Escape for back navigation
      auto ui_state = state->getUIState();
      state->setRoute(ui_state.previous_route);
    } else if (event == Event::Return) {
      // Handle Enter key for navigation
      auto route = state->getRoute();
      if (route == app::Route::ConnectWallet && state->getDeviceState().wallet_connected) {
        state->setRoute(app::Route::USBContacts);
      } else if (route == app::Route::Signing && state->hasSignedTx()) {
        state->setRoute(app::Route::Result);
      }
    } else {
        return false; // Do not consume unhandled events
    }

    // --- Post-navigation side effects ---
    auto new_ui_state = state->getUIState();
    if (new_ui_state.route != old_route && new_ui_state.route == app::Route::Signing) {
        // Set signing status immediately in the main thread to prevent race conditions.
        state->setSigning(true);
        if (new_ui_state.dev_mode) {
            // Use PostEvent for async signing simulation
            auto signing_task = [state, &screen]() {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                state->setSignedHex("0xf86c098504a817c800825208940000000000000000000000000000000000000000880de0b6b3a76400008025a00926b32d505f00376248e4325687283b5a2434543a204a875142b43550b115fa02918a8337b35f35974a4435134454334543453453454345345345345345345345");
                state->setSigning(false);
                screen.PostEvent(Event::Custom); // Notify UI of completion
            };
            std::thread(signing_task).detach();
        } else {
            // In normal mode, you would initiate real hardware wallet signing here.
            // For now, it will just show the signing screen until a real implementation.
        }
    }
    
    // Check if shutdown was requested from elsewhere
    if (state->isShutdownRequested()) {
      running.store(false);
      screen.Exit();
    }
    return true; // Event was handled
  });
  
  screen.Loop(final_component);
  
  running.store(false);
  animation_thread.join();
  
  return 0;
}