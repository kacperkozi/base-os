#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include "app/state.hpp"
#include "app/validation.hpp"
#include "app/qr_generator.hpp"
#include <functional>
#include <fstream>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cmath>

using namespace ftxui;
using app::AppState;
using app::UnsignedTx;

namespace {

// Utility: Convert Wei to ETH string with better formatting
std::string weiToEth(const std::string& wei) {
  if (wei.empty() || wei == "0") return "0 ETH";
  
  // For display purposes, show both Wei and estimated ETH
  if (wei.length() > 18) {
    return wei + " Wei (~" + wei.substr(0, wei.length() - 18) + "." + 
           wei.substr(wei.length() - 18, 4) + " ETH)";
  }
  return wei + " Wei";
}

// Utility: Format address for display with middle ellipsis
std::string formatAddress(const std::string& addr, bool shorten = false) {
  if (addr.length() <= 10 || !shorten) return addr;
  return addr.substr(0, 6) + "..." + addr.substr(addr.length() - 4);
}

// Enhanced Banner with breadcrumbs and progress indicator
Component EnhancedBanner(AppState& s) {
  return Renderer([&]{
    // Title bar
    std::string title = "🔐 Base OS - Offline Transaction Signer";
    std::string network = "[Base Network • Chain ID: 8453]";
    
    // Progress indicator
    std::vector<std::string> steps = {
      "Connect", "Contacts", "Transaction", "Review", "Sign", "Result"
    };
    int current_step = 0;
    switch(s.route) {
      case app::Route::ConnectWallet: current_step = 0; break;
      case app::Route::USBContacts: current_step = 1; break;
      case app::Route::TransactionInput: current_step = 2; break;
      case app::Route::Confirmation: current_step = 3; break;
      case app::Route::Signing: current_step = 4; break;
      case app::Route::Result: current_step = 5; break;
      default: current_step = -1;
    }
    
    // Build progress bar
    Elements progress_elements;
    for (int i = 0; i < steps.size(); ++i) {
      if (i > 0) {
        progress_elements.push_back(text(" → ") | dim);
      }
      
      Color step_color;
      if (i < current_step) step_color = Color::Green;  // Completed
      else if (i == current_step) step_color = Color::Cyan;  // Current
      else step_color = Color::GrayDark;  // Future
      
      std::string step_text = std::to_string(i + 1) + "." + steps[i];
      if (i == current_step) {
        step_text = "[" + step_text + "]";
      }
      
      progress_elements.push_back(text(step_text) | color(step_color));
    }
    
    // Wallet status with icon
    std::string wallet_icon;
    std::string wallet_status;
    Color wallet_color;
    
    if (s.wallet_connected) {
      wallet_icon = "✓";
      wallet_status = "Connected";
      wallet_color = Color::Green;
    } else if (s.is_detecting_wallet) {
      wallet_icon = "⟳";
      wallet_status = "Detecting...";
      wallet_color = Color::Yellow;
    } else {
      wallet_icon = "○";
      wallet_status = "Not Connected";
      wallet_color = Color::Red;
    }
    
    return vbox({
      // Title bar
      hbox({
        text(title) | bold,
        filler(),
        text(network) | color(Color::Blue),
        text(" | "),
        text(wallet_icon + " ") | color(wallet_color),
        text(wallet_status) | color(wallet_color)
      }),
      separator(),
      // Progress indicator
      hbox(progress_elements) | center,
      separator()
    });
  });
}

// Enhanced Status Bar with contextual hints
Component EnhancedStatusBar(AppState& s) {
  return Renderer([&]{
    // Context-aware keyboard shortcuts
    std::vector<std::pair<std::string, std::string>> shortcuts;
    
    switch(s.route) {
      case app::Route::ConnectWallet:
        shortcuts = {
          {"Enter", "Continue"},
          {"F1", "Help"},
          {"Ctrl+Q", "Quit"}
        };
        break;
        
      case app::Route::USBContacts:
        shortcuts = {
          {"↑↓/jk", "Navigate"},
          {"Enter", "Select"},
          {"u", "Scan USB"},
          {"Tab", "Skip"},
          {"Esc", "Back"}
        };
        break;
        
      case app::Route::TransactionInput:
        shortcuts = {
          {"Tab", "Next Field"},
          {"Shift+Tab", "Previous"},
          {"Enter", "Review"},
          {"Esc", "Back"}
        };
        break;
        
      case app::Route::Confirmation:
        shortcuts = {
          {"Enter", "Sign Transaction"},
          {"e", "Edit"},
          {"Esc", "Back"}
        };
        break;
        
      case app::Route::Signing:
        shortcuts = {
          {"Esc", "Cancel"},
          {"", "Waiting for hardware wallet..."}
        };
        break;
        
      case app::Route::Result:
        shortcuts = {
          {"s", "Save to File"},
          {"n", "New Transaction"},
          {"q", "Quit"}
        };
        break;
        
      default:
        shortcuts = {
          {"F1", "Help"},
          {"Esc", "Back"}
        };
    }
    
    // Build shortcut display
    Elements shortcut_elements;
    for (const auto& [key, action] : shortcuts) {
      if (!shortcut_elements.empty() && !key.empty()) {
        shortcut_elements.push_back(text(" • ") | dim);
      }
      
      if (!key.empty()) {
        shortcut_elements.push_back(
          hbox({
            text("[" + key + "]") | bold | color(Color::Cyan),
            text(" " + action) | color(Color::GrayLight)
          })
        );
      } else {
        shortcut_elements.push_back(text(action) | color(Color::Yellow));
      }
    }
    
    // Add tip at the end
    shortcut_elements.push_back(filler());
    shortcut_elements.push_back(text("💡 Press F1 for full help") | dim);
    
    // Show errors/warnings inline if present
    if (!s.error.empty()) {
      return vbox({
        hbox({
          text("⚠ Error: ") | color(Color::Red) | bold,
          text(s.error) | color(Color::Red),
          filler()
        }),
        separator(),
        hbox(shortcut_elements)
      });
    }
    
    if (!s.info.empty()) {
      return vbox({
        hbox({
          text("ℹ ") | color(Color::Blue),
          text(s.info) | color(Color::Blue),
          filler()
        }),
        separator(),
        hbox(shortcut_elements)
      });
    }
    
    return hbox(shortcut_elements);
  });
}

// Enhanced Connect Wallet View with visual feedback
Component ImprovedConnectWalletView(AppState& s) {
  auto continue_btn = Button("→ Continue", [&]{
    s.is_detecting_wallet = true;
    std::thread([&](){
      std::this_thread::sleep_for(std::chrono::seconds(1));
      s.devices = {
        {"Ledger Nano X", "/dev/hidraw0", true, false, "2.1.0", "ABC123"},
        {"Trezor Model T", "/dev/hidraw1", false, false, "2.4.3", "XYZ789"}
      };
      s.wallet_connected = true;
      s.is_detecting_wallet = false;
      s.route = app::Route::USBContacts;
    }).detach();
  });
  
  auto refresh_btn = Button("↻ Refresh", [&]{
    s.is_detecting_wallet = true;
    std::thread([&](){
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      s.is_detecting_wallet = false;
    }).detach();
  });
  
  auto layout = Container::Horizontal({continue_btn, refresh_btn});
  
  return Renderer(layout, [&]{
    Elements content;
    
    // Header with icon
    content.push_back(text(""));
    content.push_back(text("🔌 Hardware Wallet Connection") | bold | center);
    content.push_back(text("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━") | center | dim);
    content.push_back(text(""));
    
    // Connection status card
    Elements status_card;
    
    if (s.is_detecting_wallet) {
      // Animated detection
      std::string dots = std::string((s.animation_frame / 5) % 4, '.');
      status_card.push_back(text("🔍 Detecting hardware wallets" + dots) | color(Color::Yellow) | center);
      status_card.push_back(text(""));
      
      // Spinning animation
      const char* spinner = "⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏";
      char spin = spinner[s.animation_frame % 10];
      status_card.push_back(text(std::string(1, spin) + " Please wait...") | center | color(Color::Yellow));
    } else if (!s.devices.empty()) {
      // Show detected devices
      status_card.push_back(text("✓ Devices Detected:") | color(Color::Green) | bold);
      status_card.push_back(text(""));
      
      for (const auto& device : s.devices) {
        Elements device_info;
        device_info.push_back(text(device.connected ? "🟢" : "🔴"));
        device_info.push_back(text(" " + device.model));
        if (!device.version.empty()) {
          device_info.push_back(text(" (v" + device.version + ")") | dim);
        }
        status_card.push_back(hbox(device_info));
      }
    } else {
      // No devices found
      status_card.push_back(text("No hardware wallets detected") | color(Color::GrayDark) | center);
    }
    
    content.push_back(vbox(status_card) | border | size(WIDTH, EQUAL, 50));
    content.push_back(text(""));
    
    // Checklist with visual indicators
    content.push_back(text("Prerequisites:") | bold);
    
    struct CheckItem {
      bool checked;
      std::string text;
      std::string tip;
    };
    
    std::vector<CheckItem> checklist = {
      {!s.devices.empty(), "Hardware wallet connected via USB", "Check cable connection"},
      {s.wallet_connected, "Device unlocked with PIN", "Enter PIN on device"},
      {s.wallet_connected, "Ethereum app opened", "Navigate to Ethereum app"},
      {true, "USB permissions granted", "May require sudo on Linux"}
    };
    
    for (const auto& item : checklist) {
      Elements check_elem;
      check_elem.push_back(text(item.checked ? " ✓ " : " ○ ") | 
                          color(item.checked ? Color::Green : Color::GrayDark));
      check_elem.push_back(text(item.text));
      if (!item.checked && !item.tip.empty()) {
        check_elem.push_back(text(" (" + item.tip + ")") | dim);
      }
      content.push_back(hbox(check_elem));
    }
    
    content.push_back(text(""));
    
    // Action buttons with icons
    content.push_back(hbox({
      filler(),
      continue_btn->Render() | (s.wallet_connected ? color(Color::Green) : color(Color::White)),
      text("  "),
      refresh_btn->Render(),
      filler()
    }));
    
    // Help text
    content.push_back(text(""));
    content.push_back(text("Need help? Common issues:") | dim | center);
    content.push_back(text("• Ensure device firmware is up to date") | dim | center);
    content.push_back(text("• Try different USB port or cable") | dim | center);
    content.push_back(text("• Check device compatibility") | dim | center);
    
    return vbox(std::move(content)) | border | size(WIDTH, LESS_THAN, 80) | center;
  });
}

// Enhanced Transaction Input with inline validation
Component ImprovedTransactionInputView(AppState& s) {
  static std::string to_addr;
  static std::string value_wei;
  static std::string gas_limit = "21000";
  static std::string data_hex = "0x";
  static int selected_field = 0;
  
  // Real-time validation messages
  static std::string to_validation;
  static std::string value_validation;
  static std::string gas_validation;
  
  // Validate as user types
  auto validate_address = [](const std::string& addr) -> std::string {
    if (addr.empty()) return "Required";
    if (addr.length() < 2 || addr.substr(0, 2) != "0x") return "Must start with 0x";
    if (addr.length() != 42) return std::to_string(42 - addr.length()) + " chars remaining";
    if (!app::IsValidAddress(addr)) return "Invalid checksum";
    return "✓ Valid address";
  };
  
  auto validate_value = [](const std::string& val) -> std::string {
    if (val.empty()) return "Required";
    try {
      auto num = std::stoull(val);
      if (num == 0) return "Warning: Sending 0 Wei";
      return "✓ " + weiToEth(val);
    } catch (...) {
      return "Invalid number";
    }
  };
  
  // Create labeled inputs with validation
  auto make_field_renderer = [](const std::string& label, 
                                const std::string& value,
                                const std::string& placeholder,
                                const std::string& validation,
                                const std::string& icon,
                                bool selected) {
    Color border_color = Color::GrayDark;
    if (selected) border_color = Color::Cyan;
    else if (validation.find("✓") != std::string::npos) border_color = Color::Green;
    else if (!validation.empty() && validation != "Required") border_color = Color::Red;
    
    return vbox({
      hbox({
        text(icon + " " + label) | bold,
        filler(),
        text(validation) | color(
          validation.find("✓") != std::string::npos ? Color::Green :
          validation == "Required" || validation.find("remaining") != std::string::npos ? Color::GrayDark :
          Color::Red
        )
      }),
      hbox({
        text(value.empty() ? placeholder : value) | 
        (value.empty() ? dim : color(Color::White))
      }) | border | borderColor(border_color)
    });
  };
  
  auto input_to = Input(&to_addr, "0x...");
  auto input_value = Input(&value_wei, "Amount in Wei");
  auto input_gas = Input(&gas_limit, "21000");
  auto input_data = Input(&data_hex, "0x");
  
  auto review_btn = Button("→ Review Transaction", [&]{
    if (!app::IsValidAddress(to_addr)) {
      s.error = "Invalid recipient address";
      return;
    }
    if (value_wei.empty()) {
      s.error = "Amount is required";
      return;
    }
    
    s.unsigned_tx.to = to_addr;
    s.unsigned_tx.value = value_wei;
    s.unsigned_tx.gas_limit = gas_limit;
    s.unsigned_tx.data = data_hex;
    s.has_unsigned = true;
    s.route = app::Route::Confirmation;
  });
  
  auto layout = Container::Vertical({
    input_to,
    input_value,
    input_gas,
    input_data,
    review_btn
  });
  
  return Renderer(layout, [&]{
    // Update validations
    to_validation = validate_address(to_addr);
    value_validation = value_wei.empty() ? "Required" : validate_value(value_wei);
    
    Elements content;
    
    content.push_back(text(""));
    content.push_back(text("📝 Transaction Details") | bold | center);
    content.push_back(text("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━") | center | dim);
    content.push_back(text(""));
    
    // Show current network context
    content.push_back(hbox({
      text("Network: ") | dim,
      text("Base Mainnet") | color(Color::Blue),
      text(" • ") | dim,
      text("Chain ID: ") | dim,
      text("8453") | color(Color::Blue),
      text(" • ") | dim,
      text("Type: ") | dim,
      text(s.use_eip1559 ? "EIP-1559" : "Legacy") | color(Color::Yellow)
    }) | center);
    content.push_back(text(""));
    
    // Form fields with enhanced visual feedback
    content.push_back(make_field_renderer(
      "Recipient Address",
      to_addr,
      "0x0000000000000000000000000000000000000000",
      to_validation,
      "📍",
      selected_field == 0
    ));
    content.push_back(text(""));
    
    content.push_back(make_field_renderer(
      "Amount",
      value_wei,
      "Enter amount in Wei (1 ETH = 10^18 Wei)",
      value_validation,
      "💰",
      selected_field == 1
    ));
    
    // Quick amount buttons
    content.push_back(hbox({
      text("Quick amounts: ") | dim,
      text("[0.001 ETH]") | color(Color::Cyan),
      text(" ") | dim,
      text("[0.01 ETH]") | color(Color::Cyan),
      text(" ") | dim,
      text("[0.1 ETH]") | color(Color::Cyan),
      text(" ") | dim,
      text("[1 ETH]") | color(Color::Cyan)
    }) | center);
    content.push_back(text(""));
    
    content.push_back(make_field_renderer(
      "Gas Limit",
      gas_limit,
      "21000 (standard transfer)",
      gas_limit.empty() ? "Using default: 21000" : "✓ Set to " + gas_limit,
      "⛽",
      selected_field == 2
    ));
    content.push_back(text(""));
    
    content.push_back(make_field_renderer(
      "Data (Optional)",
      data_hex,
      "0x (empty)",
      data_hex == "0x" ? "No data" : "Hex data: " + std::to_string((data_hex.length() - 2) / 2) + " bytes",
      "📄",
      selected_field == 3
    ));
    content.push_back(text(""));
    
    // Action button
    bool can_continue = to_validation.find("✓") != std::string::npos && 
                       !value_wei.empty();
    
    content.push_back(hbox({
      filler(),
      review_btn->Render() | (can_continue ? color(Color::Green) : dim),
      filler()
    }));
    
    // Tips
    content.push_back(text(""));
    content.push_back(text("💡 Tips:") | dim);
    content.push_back(text("• Double-check the recipient address") | dim);
    content.push_back(text("• 1 Gwei = 10^9 Wei, 1 ETH = 10^18 Wei") | dim);
    content.push_back(text("• Standard transfer uses 21000 gas") | dim);
    
    return vbox(std::move(content)) | border | size(WIDTH, LESS_THAN, 90) | center;
  });
}

} // namespace

// Export the improved run function
int RunImprovedApp() {
  AppState state;
  
  auto screen = ScreenInteractive::FitComponent();
  auto exit = screen.ExitLoopClosure();
  
  // Use enhanced components
  auto banner = EnhancedBanner(state);
  auto status = EnhancedStatusBar(state);
  
  // Route to appropriate view
  auto route = Renderer([&]{
    switch(state.route) {
      case app::Route::ConnectWallet:
        return ImprovedConnectWalletView(state)->Render();
      case app::Route::TransactionInput:
        return ImprovedTransactionInputView(state)->Render();
      // ... other routes use existing views for now
      default:
        return ImprovedConnectWalletView(state)->Render();
    }
  });
  
  // Main layout with enhanced banner and status
  auto root = Renderer(Container::Vertical({route}), [&]{
    return vbox({
      banner->Render(),
      route->Render() | flex,
      separator(),
      status->Render()
    });
  });
  
  // Enhanced global shortcuts
  root |= CatchEvent([&](Event e){
    // Quick help toggle
    if (e == Event::F1 || e == Event::Character('?')) {
      state.previous_route = state.route;
      state.route = app::Route::Help;
      return true;
    }
    
    // Number keys for quick navigation
    if (e.is_character() && e.character() >= "1" && e.character() <= "6") {
      int screen = e.character()[0] - '1';
      app::Route routes[] = {
        app::Route::ConnectWallet,
        app::Route::USBContacts,
        app::Route::TransactionInput,
        app::Route::Confirmation,
        app::Route::Signing,
        app::Route::Result
      };
      if (screen >= 0 && screen < 6) {
        state.route = routes[screen];
        return true;
      }
    }
    
    // Ctrl+Q to quit
    if (e == Event::Special("\\q") || 
        (e.is_character() && e.character() == "q" && e.is_ctrl())) {
      exit();
      return true;
    }
    
    // ESC for back navigation
    if (e == Event::Escape) {
      switch(state.route) {
        case app::Route::USBContacts:
          state.route = app::Route::ConnectWallet;
          return true;
        case app::Route::TransactionInput:
          state.route = app::Route::USBContacts;
          return true;
        case app::Route::Confirmation:
          state.route = app::Route::TransactionInput;
          return true;
        case app::Route::Help:
        case app::Route::Settings:
          state.route = state.previous_route;
          return true;
        case app::Route::ConnectWallet:
          exit();
          return true;
        default:
          break;
      }
    }
    
    return false;
  });
  
  // Animation loop
  std::thread animator([&](){
    while (!state.isShutdownRequested()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      state.incrementAnimationFrame();
      screen.PostEvent(Event::Custom);
    }
  });
  
  screen.Loop(root);
  
  state.requestShutdown();
  animator.join();
  
  return 0;
}