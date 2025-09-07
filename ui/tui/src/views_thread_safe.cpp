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
#include <atomic>
#include <memory>

using namespace ftxui;
using app::AppState;
using app::UnsignedTx;

namespace {

// Thread-safe UI update helper
class UIUpdater {
private:
    ScreenInteractive* screen_;
    std::atomic<bool> shutdown_requested_{false};
    
public:
    explicit UIUpdater(ScreenInteractive* screen) : screen_(screen) {}
    
    void requestShutdown() {
        shutdown_requested_.store(true);
    }
    
    bool isShutdownRequested() const {
        return shutdown_requested_.load();
    }
    
    void postUpdate() {
        if (screen_ && !shutdown_requested_.load()) {
            screen_->PostEvent(Event::Custom);
        }
    }
    
    // Safe async operation that properly updates UI
    template<typename Func>
    void runAsyncOperation(AppState& state, Func&& operation) {
        std::thread([this, &state, op = std::forward<Func>(operation)]() {
            try {
                op();
                if (!shutdown_requested_.load()) {
                    postUpdate();
                }
            } catch (const std::exception& e) {
                state.setError("Operation failed: " + std::string(e.what()));
                postUpdate();
            } catch (...) {
                state.setError("Unknown error occurred during operation");
                postUpdate();
            }
        }).detach();
    }
};

// Global UI updater (will be initialized in RunApp)
static std::unique_ptr<UIUpdater> g_ui_updater;

// Utility: Convert Wei to ETH string
std::string weiToEth(const std::string& wei) {
  if (wei.empty() || wei == "0") return "0";
  return wei + " Wei";
}

// Utility: Format address for display (shorten if needed)
std::string formatAddress(const std::string& addr, bool shorten = false) {
  if (addr.length() <= 10 || !shorten) return addr;
  return addr.substr(0, 6) + "..." + addr.substr(addr.length() - 4);
}

// Utility: Load address book from file (thread-safe)
void loadAddressBook(AppState& s) {
  std::vector<app::KnownAddress> addresses;
  
  // Create known addresses safely
  auto addr1 = app::KnownAddress::create(
    "0x4200000000000000000000000000000000000016", 
    "Base Bridge", 
    "Official Base L1->L2 Bridge", 
    app::ContactType::Contract
  );
  auto addr2 = app::KnownAddress::create(
    "0x833589fCD6eDb6E08f4c7C32D4f71b54bdA02913", 
    "USDC", 
    "USD Coin on Base", 
    app::ContactType::Contract
  );
  auto addr3 = app::KnownAddress::create(
    "0x50c5725949A6F0c72E6C4a641F24049A917DB0Cb", 
    "DAI", 
    "DAI Stablecoin on Base", 
    app::ContactType::Contract
  );
  
  if (addr1) addresses.push_back(*addr1);
  if (addr2) addresses.push_back(*addr2);
  if (addr3) addresses.push_back(*addr3);
  
  s.setKnownAddresses(addresses);
}

// Utility: Load USB contacts from mounted devices (thread-safe)
void loadUSBContacts(AppState& s) {
  if (!g_ui_updater) return;
  
  s.setScanningUsb(true);
  s.setUsbContacts({});  // Clear existing contacts
  
  g_ui_updater->runAsyncOperation(s, [&s]() {
    // Simulate USB scanning delay
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    if (g_ui_updater->isShutdownRequested()) return;
    
    // Create mock USB contacts safely
    std::vector<app::KnownAddress> contacts;
    
    auto contact1 = app::KnownAddress::create(
      "0x742d35Cc6641C154db0bEF6a74B0742e5b4b4e7c", 
      "bob.base.eth", 
      "Base name for Bob", 
      app::ContactType::Base
    );
    auto contact2 = app::KnownAddress::create(
      "0x8ba1f109551bD432803012645Hac136c", 
      "Team Multisig", 
      "Development team multisig wallet", 
      app::ContactType::Multisig
    );
    auto contact3 = app::KnownAddress::create(
      "0x1234567890abcdef1234567890abcdef12345678", 
      "DEX Contract", 
      "Decentralized exchange contract", 
      app::ContactType::Contract
    );
    auto contact4 = app::KnownAddress::create(
      "0x9876543210fedcba9876543210fedcba98765432", 
      "John Doe", 
      "Personal wallet", 
      app::ContactType::EOA
    );
    
    if (contact1) contacts.push_back(*contact1);
    if (contact2) contacts.push_back(*contact2);
    if (contact3) contacts.push_back(*contact3);
    if (contact4) contacts.push_back(*contact4);
    
    // Thread-safe state updates
    s.setUsbContacts(contacts);
    s.setScanningUsb(false);
    s.setUsbScanComplete(true);
  });
}

// Utility: Get contact type icon
std::string getContactIcon(app::ContactType type) {
  switch(type) {
    case app::ContactType::ENS: return "🌐";
    case app::ContactType::Base: return "🔵";
    case app::ContactType::Multisig: return "🔶";
    case app::ContactType::Contract: return "📄";
    case app::ContactType::EOA: return "👤";
    default: return "";
  }
}

// Utility: Get contact type color
Color getContactColor(app::ContactType type) {
  switch(type) {
    case app::ContactType::ENS: return Color::Blue;
    case app::ContactType::Base: return Color::Magenta;
    case app::ContactType::Multisig: return Color::Yellow;
    case app::ContactType::Contract: return Color::Cyan;
    case app::ContactType::EOA: return Color::Green;
    default: return Color::White;
  }
}

// Component: Animated spinner (no threading)
Element Spinner(int frame) {
  const std::vector<std::string> frames = {
    "⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"
  };
  return text(frames[frame % frames.size()]) | color(Color::GreenLight);
}

// Component: Progress dots animation (no threading)
Element ProgressDots(int frame) {
  std::string dots = "";
  for (int i = 0; i < (frame % 4); ++i) {
    dots += ".";
  }
  return text(dots);
}

// Component: Status bar at bottom
Component StatusBar(AppState& s) {
  return Renderer([&] {
    auto left = text("Offline Signer • " + s.getNetworkName() + " (Chain " + std::to_string(s.getUnsignedTx().chain_id) + ")") | color(Color::GrayDark);
    auto mid = s.getStatus().empty() ? text("") : text(" " + s.getStatus() + " ") | color(Color::Green);
    auto right = text("hjkl:Move 1-5:Screens g:Home u:USB F1:Help") | color(Color::GrayDark);
    return hbox({ left, filler(), mid, filler(), right }) | bgcolor(Color::Black) | color(Color::Green);
  });
}

// Component: Error/Info banner at top
Component Banner(AppState& s) {
  return Renderer([&]{
    Elements lines;
    std::string error = s.getError();
    std::string info = s.getInfo();
    
    if (!error.empty()) {
      lines.push_back(hbox({
        text("[ERROR] ") | color(Color::Red) | bold,
        text(error) | color(Color::Red)
      }));
    }
    if (!info.empty()) {
      lines.push_back(hbox({
        text("[INFO] ") | color(Color::Blue) | bold,
        text(info) | color(Color::Blue)
      }));
    }
    if (lines.empty()) return text("");
    return vbox(std::move(lines)) | border | color(Color::Green) | bgcolor(Color::Black);
  });
}

// Screen 1: Connect Wallet (Thread-Safe)
Component ConnectWalletView(AppState& s) {
  auto continue_btn = Button("Continue", [&]{
    if (!g_ui_updater) return;
    
    s.setDetectingWallet(true);
    s.clearError();
    
    g_ui_updater->runAsyncOperation(s, [&s]() {
      // Simulate wallet detection
      std::this_thread::sleep_for(std::chrono::seconds(1));
      
      if (g_ui_updater->isShutdownRequested()) return;
      
      // Create mock devices safely
      std::vector<app::DeviceInfo> devices = {
        {"Ledger Nano X", "/dev/hidraw0", true, false, "2.1.0", "ABC123"},
        {"Trezor Model T", "/dev/hidraw1", false, false, "2.4.3", "XYZ789"}
      };
      
      // Thread-safe state updates
      s.setDevices(devices);
      s.setWalletConnected(!devices.empty());
      s.setDetectingWallet(false);
      
      if (s.isWalletConnected()) {
        s.setRoute(app::Route::USBContacts);
      } else {
        s.setError("No hardware wallet detected. Please connect your device and try again.");
      }
    });
  });
  
  auto content = Renderer(continue_btn, [&]{
    Elements content;
    
    // Title
    content.push_back(text(""));
    content.push_back(text("Connect Hardware Wallet") | bold | center | color(Color::Green));
    content.push_back(text("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━") | center | color(Color::GrayDark));
    content.push_back(text(""));
    
    // Instructions
    content.push_back(text("Please follow these steps:") | color(Color::GrayDark));
    content.push_back(text(""));
    content.push_back(hbox({text("  1. ") | color(Color::Green), text("Connect your hardware wallet via USB") | color(Color::GreenLight)}));
    content.push_back(hbox({text("  2. ") | color(Color::Green), text("Unlock your device with PIN/password") | color(Color::GreenLight)}));
    content.push_back(hbox({text("  3. ") | color(Color::Green), text("Open the Ethereum application on the device") | color(Color::GreenLight)}));
    content.push_back(text(""));
    
    // Status
    if (s.isDetectingWallet()) {
      content.push_back(hbox({
        Spinner(s.getAnimationFrame()),
        text(" Detecting hardware wallets"),
        ProgressDots(s.getAnimationFrame())
      }) | center);
    } else {
      auto devices = s.getDevices();
      if (!devices.empty()) {
        content.push_back(text("Detected devices:") | dim);
        for (const auto& dev : devices) {
          std::string status = dev.connected ? "✓ Connected" : "✗ Not connected";
          if (dev.connected && !dev.app_open) status += " (Open Ethereum app)";
          content.push_back(hbox({
            text("  • "),
            text(dev.model) | bold,
            text(" - "),
            text(status) | (dev.connected ? color(Color::Green) : color(Color::Red))
          }));
        }
        content.push_back(text(""));
      }
    }
    
    content.push_back(text(""));
    content.push_back(hbox({filler(), continue_btn->Render(), filler()}) | size(HEIGHT, EQUAL, 3));
    
    // Help text
    content.push_back(text(""));
    content.push_back(text("Once your wallet is connected and ready, press Continue") | center | dim);
    
    return vbox(std::move(content)) | border | size(WIDTH, EQUAL, 80) | center;
  });
  
  return content;
}

// Screen 2: USB Contacts (Thread-Safe)
Component USBContactsView(AppState& s) {
  auto scan_btn = Button("Scan USB Devices", [&]{
    loadUSBContacts(s);
  });
  
  auto skip_btn = Button("Skip", [&]{
    s.setRoute(app::Route::TransactionInput);
  });
  
  auto select_btn = Button("Select Contact", [&]{
    auto contacts = s.getUsbContacts();
    int selected = s.getSelectedContact();
    if (!contacts.empty() && selected >= 0 && selected < static_cast<int>(contacts.size())) {
      auto tx = s.getUnsignedTx();
      tx.to = contacts[selected].address;
      s.setUnsignedTx(tx);
      s.setRoute(app::Route::TransactionInput);
    }
  });
  
  auto back_btn = Button("Back", [&]{
    s.setRoute(app::Route::ConnectWallet);
  });
  
  auto layout = Container::Horizontal({scan_btn, skip_btn, select_btn, back_btn});
  
  return Renderer(layout, [&]{
    Elements content;
    
    content.push_back(text(""));
    content.push_back(text("USB Contacts") | bold | center | color(Color::Green));
    content.push_back(text("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━") | center | color(Color::GrayDark));
    content.push_back(text(""));
    
    if (s.isScanningUsb()) {
      content.push_back(hbox({
        Spinner(s.getAnimationFrame()),
        text(" Scanning USB devices for contacts.json files"),
        ProgressDots(s.getAnimationFrame())
      }) | center | color(Color::GreenLight));
    } else if (s.isUsbScanComplete()) {
      auto contacts = s.getUsbContacts();
      if (contacts.empty()) {
        content.push_back(text("No contacts.json files found on USB devices") | center | color(Color::Yellow));
        content.push_back(text("You can skip this step or manually scan again") | center | color(Color::GrayDark));
      } else {
        content.push_back(text("Found " + std::to_string(contacts.size()) + " contacts:") | color(Color::GreenLight));
        content.push_back(text(""));
        
        int selected_contact = s.getSelectedContact();
        // Display contacts with selection
        for (size_t i = 0; i < contacts.size(); ++i) {
          const auto& contact = contacts[i];
          std::string icon = getContactIcon(contact.type);
          Color typeColor = getContactColor(contact.type);
          
          Element line;
          if (static_cast<int>(i) == selected_contact) {
            line = hbox({
              text("> ") | color(Color::Green) | bold,
              text(icon + " ") | color(typeColor),
              text(contact.name) | color(Color::Green) | bold,
              text(" - ") | color(Color::GrayDark),
              text(formatAddress(contact.address, true)) | color(Color::GreenLight)
            }) | bgcolor(Color::Black);
          } else {
            line = hbox({
              text("  "),
              text(icon + " ") | color(typeColor),
              text(contact.name) | color(Color::GreenLight),
              text(" - ") | color(Color::GrayDark),
              text(formatAddress(contact.address, true)) | color(Color::GrayDark)
            });
          }
          content.push_back(line);
          
          if (static_cast<int>(i) == selected_contact && !contact.description.empty()) {
            content.push_back(hbox({
              text("    "),
              text(contact.description) | color(Color::GrayDark) | italic
            }));
          }
        }
        
        content.push_back(text(""));
        content.push_back(text("Use j/k to navigate, Enter to select") | center | color(Color::GrayDark));
      }
    } else {
      content.push_back(text("Scan USB devices to find saved contacts") | center | color(Color::GreenLight));
      content.push_back(text(""));
      content.push_back(text("Looking for contacts.json files on mounted USB devices") | center | color(Color::GrayDark));
    }
    
    content.push_back(text(""));
    content.push_back(separator());
    
    // Buttons
    auto contacts = s.getUsbContacts();
    content.push_back(hbox({
      filler(),
      scan_btn->Render(),
      text("  "),
      skip_btn->Render(),
      text("  "),
      select_btn->Render() | (contacts.empty() ? dim : color(Color::Green)),
      text("  "),
      back_btn->Render(),
      filler()
    }));
    
    return vbox(std::move(content)) | border | size(WIDTH, EQUAL, 80) | center | bgcolor(Color::Black);
  }) | CatchEvent([&](Event e) {
    auto contacts = s.getUsbContacts();
    int selected = s.getSelectedContact();
    
    // Vim-like navigation for contacts
    if (e == Event::Character('j') && selected < static_cast<int>(contacts.size()) - 1) {
      s.setSelectedContact(selected + 1);
      return true;
    }
    if (e == Event::Character('k') && selected > 0) {
      s.setSelectedContact(selected - 1);
      return true;
    }
    if (e == Event::Return && !contacts.empty() && selected >= 0 && selected < static_cast<int>(contacts.size())) {
      auto tx = s.getUnsignedTx();
      tx.to = contacts[selected].address;
      s.setUnsignedTx(tx);
      s.setRoute(app::Route::TransactionInput);
      return true;
    }
    if (e == Event::Character('u')) {
      loadUSBContacts(s);
      return true;
    }
    return false;
  });
}

// Screen 3: Transaction Input (Thread-Safe)
Component TransactionInputView(AppState& s) {
  // Use state-backed values instead of static variables
  auto tx = s.getUnsignedTx();
  static std::string to = tx.to;
  static std::string value = tx.value;
  static std::string nonce = tx.nonce;
  static std::string gas_limit = tx.gas_limit.empty() ? "21000" : tx.gas_limit;
  static std::string gas_price = tx.gas_price;
  static std::string max_fee = tx.max_fee_per_gas;
  static std::string max_priority = tx.max_priority_fee_per_gas;
  static std::string data = tx.data.empty() ? "0x" : tx.data;
  
  // Input components
  auto in_to = Input(&to, "0x...");
  auto in_value = Input(&value, "Amount in Wei (e.g., 1000000000000000000 for 1 ETH)");
  auto in_nonce = Input(&nonce, "Transaction nonce");
  auto in_gas_limit = Input(&gas_limit, "21000");
  
  Component gas_inputs;
  if (s.useEip1559()) {
    auto in_max_fee = Input(&max_fee, "Max fee per gas (Gwei)");
    auto in_max_priority = Input(&max_priority, "Priority fee (Gwei)");
    gas_inputs = Container::Vertical({in_max_fee, in_max_priority});
  } else {
    auto in_gas_price = Input(&gas_price, "Gas price (Gwei)");
    gas_inputs = Container::Vertical({in_gas_price});
  }
  
  auto in_data = Input(&data, "0x");
  
  // Validation and next button
  auto validate_and_continue = [&](){
    // Clear previous errors
    s.setFieldErrors({});
    std::map<std::string, std::string> field_errors;
    
    // Validate address
    if (!app::IsAddress(to)) {
      field_errors["to"] = "Invalid Ethereum address format";
    }
    
    // Validate numeric fields
    if (value.empty() || !app::IsNumeric(value)) {
      field_errors["value"] = "Amount must be a number";
    }
    
    if (nonce.empty() || !app::IsNumeric(nonce)) {
      field_errors["nonce"] = "Nonce must be a number";
    }
    
    if (gas_limit.empty() || !app::IsNumeric(gas_limit)) {
      field_errors["gas_limit"] = "Gas limit must be a number";
    }
    
    if (!field_errors.empty()) {
      std::string err = "Please fix the following errors:\n";
      for (const auto& [field, msg] : field_errors) {
        err += "  • " + msg + "\n";
      }
      s.setFieldErrors(field_errors);
      s.setError(err);
      return;
    }
    
    // Create and validate transaction
    UnsignedTx new_tx = s.getUnsignedTx();
    new_tx.to = to;
    new_tx.value = value;
    new_tx.nonce = nonce;
    new_tx.gas_limit = gas_limit;
    new_tx.data = data;
    
    if (s.useEip1559()) {
      new_tx.max_fee_per_gas = max_fee;
      new_tx.max_priority_fee_per_gas = max_priority;
      new_tx.type = 2;
    } else {
      new_tx.gas_price = gas_price;
      new_tx.type = 0;
    }
    
    if (!s.setUnsignedTx(new_tx)) {
      s.setError("Failed to save transaction. Please check all fields.");
      return;
    }
    
    s.clearError();
    s.setRoute(app::Route::Confirmation);
  };
  
  auto next_btn = Button("Review Transaction", validate_and_continue);
  auto back_btn = Button("Back", [&]{ 
    s.setRoute(app::Route::USBContacts);
  });
  
  // Build input container
  std::vector<Component> inputs = {
    in_to, in_value, in_nonce, in_gas_limit
  };
  
  if (s.useEip1559()) {
    inputs.push_back(Input(&max_fee, "Max fee per gas (Gwei)"));
    inputs.push_back(Input(&max_priority, "Priority fee (Gwei)"));
  } else {
    inputs.push_back(Input(&gas_price, "Gas price (Gwei)"));
  }
  
  inputs.push_back(in_data);
  inputs.push_back(Container::Horizontal({next_btn, back_btn}));
  
  auto layout = Container::Vertical(inputs);
  
  return Renderer(layout, [&]{
    Elements content;
    
    content.push_back(text("Enter Transaction Details") | bold | center | color(Color::Green));
    content.push_back(separator() | color(Color::GrayDark));
    content.push_back(text(""));
    
    // To address
    content.push_back(hbox({
      text("To Address:") | size(WIDTH, EQUAL, 20) | color(Color::GreenLight),
      in_to->Render()
    }));
    
    auto field_errors = s.getFieldErrors();
    if (field_errors.count("to")) {
      content.push_back(hbox({
        text("") | size(WIDTH, EQUAL, 20),
        text("  ⚠ " + field_errors.at("to")) | color(Color::Red)
      }));
    }
    
    content.push_back(text(""));
    
    // Amount
    content.push_back(hbox({
      text("Amount (Wei):") | size(WIDTH, EQUAL, 20) | color(Color::GreenLight),
      in_value->Render()
    }));
    if (!value.empty() && app::IsNumeric(value)) {
      content.push_back(hbox({
        text("") | size(WIDTH, EQUAL, 20),
        text("  ≈ " + weiToEth(value)) | color(Color::GrayDark)
      }));
    }
    
    content.push_back(text(""));
    
    // Nonce and Gas Limit
    content.push_back(hbox({
      text("Nonce:") | size(WIDTH, EQUAL, 20),
      in_nonce->Render() | size(WIDTH, EQUAL, 20),
      text("  Gas Limit:") | size(WIDTH, EQUAL, 12),
      in_gas_limit->Render()
    }));
    
    content.push_back(text(""));
    
    // Gas pricing (EIP-1559 or Legacy)
    if (s.useEip1559()) {
      content.push_back(text("EIP-1559 Gas Settings:") | dim);
      content.push_back(hbox({
        text("Max Fee:") | size(WIDTH, EQUAL, 20),
        Input(&max_fee, "Max fee per gas")->Render()
      }));
      content.push_back(hbox({
        text("Priority Fee:") | size(WIDTH, EQUAL, 20),
        Input(&max_priority, "Priority fee")->Render()
      }));
    } else {
      content.push_back(hbox({
        text("Gas Price (Gwei):") | size(WIDTH, EQUAL, 20),
        Input(&gas_price, "Gas price")->Render()
      }));
    }
    
    content.push_back(text(""));
    
    // Data field
    content.push_back(hbox({
      text("Data (hex):") | size(WIDTH, EQUAL, 20),
      in_data->Render()
    }));
    
    content.push_back(text(""));
    content.push_back(separator());
    
    // Buttons
    content.push_back(hbox({
      filler(),
      next_btn->Render(),
      text("  "),
      back_btn->Render(),
      filler()
    }));
    
    return vbox(std::move(content)) | border | size(WIDTH, EQUAL, 100);
  });
}

// Screen 4: Confirmation (Thread-Safe)
Component ConfirmationView(AppState& s) {
  auto sign_btn = Button("Sign Transaction", [&]{
    if (!g_ui_updater) return;
    
    s.setRoute(app::Route::Signing);
    s.setSigning(true);
    s.setAnimationFrame(0);
    
    g_ui_updater->runAsyncOperation(s, [&s]() {
      // Simulate signing process
      std::this_thread::sleep_for(std::chrono::seconds(3));
      
      if (g_ui_updater->isShutdownRequested()) return;
      
      // Mock signed transaction
      auto tx = s.getUnsignedTx();
      std::string signed_hex = "0xf86c0185046c7cfe0083016dea94" + 
                               tx.to.substr(2) +
                               "880de0b6b3a764000080269fc7eaaa9c21f59adf8ad43ed66cf5ef9ee1c317bd4d32cd65401e7aacbda51687";
      
      // Thread-safe state updates
      if (s.setSignedHex(signed_hex)) {
        s.setSigning(false);
        s.setRoute(app::Route::Result);
      } else {
        s.setSigning(false);
        s.setError("Failed to process signed transaction");
        s.setRoute(app::Route::Confirmation);
      }
    });
  });
  
  auto edit_btn = Button("Edit", [&]{
    s.setRoute(app::Route::TransactionInput);
  });
  
  auto layout = Container::Horizontal({sign_btn, edit_btn});
  
  return Renderer(layout, [&]{
    auto tx = s.getUnsignedTx();
    
    Elements content;
    content.push_back(text("Review Transaction") | bold | center | color(Color::Green));
    content.push_back(text("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━") | center | color(Color::GrayDark));
    content.push_back(text(""));
    content.push_back(text("Please review the following details carefully:") | color(Color::GrayDark));
    content.push_back(text(""));
    
    // Transaction details box
    Elements details;
    
    // To address
    details.push_back(hbox({
      text("To: ") | bold | color(Color::Green),
      text(tx.to) | color(Color::GreenLight)
    }));
    
    // Check if known address
    auto known_addresses = s.getKnownAddresses();
    for (const auto& ka : known_addresses) {
      if (ka.address == tx.to) {
        details.push_back(hbox({
          text("    "),
          text("(" + ka.name + ")") | color(Color::Cyan)
        }));
        break;
      }
    }
    
    details.push_back(text(""));
    
    // Amount
    details.push_back(hbox({
      text("Amount: ") | bold | color(Color::Green),
      text(weiToEth(tx.value)) | color(Color::GreenLight)
    }));
    
    details.push_back(text(""));
    
    // Transaction parameters
    details.push_back(hbox({
      text("Nonce: ") | bold | color(Color::Green),
      text(tx.nonce) | color(Color::GreenLight)
    }));
    
    details.push_back(hbox({
      text("Gas Limit: ") | bold | color(Color::Green),
      text(tx.gas_limit) | color(Color::GreenLight)
    }));
    
    if (tx.isEIP1559()) {
      details.push_back(hbox({
        text("Max Fee: ") | bold | color(Color::Green),
        text(tx.max_fee_per_gas + " Gwei") | color(Color::GreenLight)
      }));
      details.push_back(hbox({
        text("Priority Fee: ") | bold | color(Color::Green),
        text(tx.max_priority_fee_per_gas + " Gwei") | color(Color::GreenLight)
      }));
    } else {
      details.push_back(hbox({
        text("Gas Price: ") | bold,
        text(tx.gas_price + " Gwei")
      }));
    }
    
    if (!tx.data.empty() && tx.data != "0x") {
      details.push_back(text(""));
      details.push_back(hbox({
        text("Data: ") | bold,
        text(tx.data.substr(0, 20) + "...") | dim
      }));
    }
    
    details.push_back(text(""));
    details.push_back(hbox({
      text("Network: ") | bold,
      text(s.getNetworkName() + " (Chain ID: " + std::to_string(tx.chain_id) + ")")
    }));
    
    content.push_back(vbox(std::move(details)) | border);
    content.push_back(text(""));
    
    // Warning
    content.push_back(hbox({
      text("[!] ") | color(Color::Yellow) | bold,
      text("After signing, you will need to confirm on your hardware wallet") | color(Color::Yellow)
    }));
    
    content.push_back(text(""));
    content.push_back(separator());
    
    // Buttons
    content.push_back(hbox({
      filler(),
      sign_btn->Render() | size(WIDTH, EQUAL, 20),
      text("  "),
      edit_btn->Render() | size(WIDTH, EQUAL, 10),
      filler()
    }));
    
    return vbox(std::move(content)) | border | size(WIDTH, EQUAL, 80) | center;
  });
}

// Screen 5: Signing Progress (Thread-Safe)
Component SigningView(AppState& s) {
  auto cancel_btn = Button("Cancel", [&]{
    s.setSigning(false);
    s.setRoute(app::Route::Confirmation);
  });
  
  return Renderer(cancel_btn, [&]{
    Elements content;
    
    content.push_back(text(""));
    content.push_back(text("Signing Transaction") | bold | center | color(Color::Green));
    content.push_back(text("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━") | center | color(Color::GrayDark));
    content.push_back(text(""));
    content.push_back(text(""));
    
    // Animation and status
    content.push_back(hbox({
      filler(),
      Spinner(s.getAnimationFrame()),
      text("  Please confirm the transaction on your hardware wallet") | color(Color::Green),
      ProgressDots(s.getAnimationFrame()),
      filler()
    }) | bold);
    
    content.push_back(text(""));
    content.push_back(text(""));
    
    // Device-specific instructions
    auto devices = s.getDevices();
    int selected_device = s.getSelectedDevice();
    if (selected_device >= 0 && selected_device < static_cast<int>(devices.size())) {
      const auto& device = devices[selected_device];
      content.push_back(text("Device: " + device.model) | center | color(Color::GreenLight));
      content.push_back(text(""));
      
      if (device.model.find("Ledger") != std::string::npos) {
        content.push_back(text("On your Ledger device:") | color(Color::GrayDark));
        content.push_back(text("  1. Review the transaction details"));
        content.push_back(text("  2. Verify the recipient address"));
        content.push_back(text("  3. Check the amount"));
        content.push_back(text("  4. Press both buttons to approve"));
      } else if (device.model.find("Trezor") != std::string::npos) {
        content.push_back(text("On your Trezor device:") | color(Color::GrayDark));
        content.push_back(text("  1. Review all transaction details"));
        content.push_back(text("  2. Tap 'Confirm' to approve"));
      }
    }
    
    content.push_back(text(""));
    content.push_back(text(""));
    
    // Progress indicator
    int progress = (s.getAnimationFrame() * 5) % 100;
    std::string bar = "[";
    for (int i = 0; i < 40; ++i) {
      if (i < progress * 40 / 100) {
        bar += "=";
      } else if (i == progress * 40 / 100) {
        bar += ">";
      } else {
        bar += " ";
      }
    }
    bar += "]";
    content.push_back(text(bar) | center | color(Color::GreenLight));
    
    content.push_back(text(""));
    content.push_back(hbox({
      filler(),
      cancel_btn->Render() | dim,
      filler()
    }));
    
    return vbox(std::move(content)) | border | size(WIDTH, EQUAL, 80) | center;
  });
}

// Screen 6: Result with QR Code (Thread-Safe)
Component ResultView(AppState& s) {
  static std::string save_path = "/home/user/signed_transaction.txt";
  static std::string save_status;
  
  auto save_to_file = [&](){
    std::string signed_hex = s.getSignedHex();
    if (signed_hex.empty()) {
      save_status = "[ERROR] No signed transaction to save";
      return;
    }
    
    try {
      std::ofstream file(save_path);
      if (file.good()) {
        file << signed_hex << std::endl;
        save_status = "[OK] Saved to " + save_path;
      } else {
        save_status = "[ERROR] Failed to save file";
      }
    } catch (const std::exception& e) {
      save_status = "[ERROR] " + std::string(e.what());
    }
  };
  
  auto save_btn = Button("Save to File", save_to_file);
  auto new_tx_btn = Button("New Transaction", [&]{
    s.clearTransaction();
    s.setRoute(app::Route::TransactionInput);
  });
  auto exit_btn = Button("Exit", [&]{
    if (g_ui_updater) {
      g_ui_updater->requestShutdown();
    }
    std::exit(0);
  });
  
  auto layout = Container::Horizontal({save_btn, new_tx_btn, exit_btn});
  
  return Renderer(layout, [&]{
    Elements content;
    
    content.push_back(text("Transaction Signed Successfully!") | bold | center | color(Color::Green));
    content.push_back(text("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━") | center | dim);
    content.push_back(text(""));
    
    // Instructions
    content.push_back(text("Scan the QR code below with an online device to broadcast the transaction") | center | color(Color::GreenLight));
    content.push_back(text(""));
    
    // QR Code
    std::string signed_hex = s.getSignedHex();
    if (!signed_hex.empty()) {
      try {
        // Use chunked QR generation for consistency
        auto qrCodes = app::GenerateQRs(signed_hex, 100);
        if (!qrCodes.empty()) {
          // For thread-safe view, just show the first part for simplicity
          auto& qrCode = qrCodes[0];
          std::string qrAscii = qrCode.toCompactAscii();
          
          std::istringstream stream(qrAscii);
          std::string line;
          Elements qrLines;
          while (std::getline(stream, line)) {
            qrLines.push_back(text(line) | center);
          }
          
          // Add part indicator if multi-part
          if (qrCode.total_parts > 1) {
            qrLines.insert(qrLines.begin(), 
              text("Part 1 of " + std::to_string(qrCode.total_parts) + " (use views_new.cpp for full navigation)") | 
              center | color(Color::Yellow));
          }
          
          content.push_back(vbox(std::move(qrLines)) | border);
        } else {
          content.push_back(text("QR generation failed") | center | color(Color::Red));
        }
      } catch (const std::exception& e) {
        content.push_back(text("QR Code generation failed: " + std::string(e.what())) | center | color(Color::Red));
      }
    }
    
    content.push_back(text(""));
    
    // Raw hex (collapsible)
    content.push_back(text("Signed Transaction Hex:") | color(Color::GrayDark));
    
    // Break hex into multiple lines for readability
    for (size_t i = 0; i < signed_hex.length(); i += 64) {
      content.push_back(text(signed_hex.substr(i, 64)) | color(Color::GrayDark) | center);
    }
    
    content.push_back(text(""));
    
    // Save status
    if (!save_status.empty()) {
      bool is_success = (save_status.find("Saved") != std::string::npos);
      content.push_back(text(save_status) | center | 
                       (is_success ? color(Color::Green) : color(Color::Red)));
      content.push_back(text(""));
    }
    
    // Action buttons
    content.push_back(hbox({
      filler(),
      save_btn->Render(),
      text("  "),
      new_tx_btn->Render(),
      text("  "),
      exit_btn->Render(),
      filler()
    }));
    
    return vbox(std::move(content)) | border | size(WIDTH, LESS_THAN, 120) | center;
  });
}

// Additional screens (Error, Help, Settings) remain the same but use thread-safe getters
Component ErrorView(AppState& s) {
  auto retry_btn = Button("Retry", [&]{
    s.clearError();
    s.setRoute(s.getPreviousRoute());
  });
  
  auto home_btn = Button("Start Over", [&]{
    s.clearError();
    s.clearTransaction();
    s.setRoute(app::Route::ConnectWallet);
  });
  
  auto layout = Container::Horizontal({retry_btn, home_btn});
  
  return Renderer(layout, [&]{
    return vbox({
      text(""),
      text("Error Occurred") | bold | center | color(Color::Red),
      text("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━") | center | color(Color::Red),
      text(""),
      text(s.getLastError()) | center,
      text(""),
      hbox({
        filler(),
        retry_btn->Render(),
        text("  "),
        home_btn->Render(),
        filler()
      }),
      text("")
    }) | border | size(WIDTH, EQUAL, 60) | center;
  });
}

Component HelpView(AppState& s) {
  auto back_btn = Button("Back", [&]{
    s.setRoute(s.getPreviousRoute());
  });
  
  return Renderer(back_btn, [&]{
    return vbox({
      text("Help & Keyboard Shortcuts") | bold | center,
      separator(),
      text(""),
      text("Navigation:"),
      text("  Tab/Shift+Tab : Move between fields"),
      text("  Arrow Keys    : Navigate menus and options"),
      text("  Enter         : Select/Activate"),
      text("  Escape        : Go back / Cancel"),
      text(""),
      text("Global Shortcuts:"),
      text("  F1            : Show this help"),
      text("  Ctrl+Q        : Quit application"),
      text(""),
      text("Transaction Flow:"),
      text("  1. Connect your hardware wallet"),
      text("  2. Enter transaction details"),
      text("  3. Review and confirm"),
      text("  4. Sign on hardware wallet"),
      text("  5. Scan QR code to broadcast"),
      text(""),
      hbox({filler(), back_btn->Render(), filler()})
    }) | border | size(WIDTH, EQUAL, 60) | center;
  });
}

Component SettingsView(AppState& s) {
  static int tx_type = s.useEip1559() ? 1 : 0;
  std::vector<std::string> tx_types = {"Legacy", "EIP-1559"};
  auto tx_type_select = Radiobox(&tx_types, &tx_type);
  
  auto save_btn = Button("Save", [&]{
    s.setUseEip1559(tx_type == 1);
    s.setRoute(s.getPreviousRoute());
  });
  
  auto cancel_btn = Button("Cancel", [&]{
    s.setRoute(s.getPreviousRoute());
  });
  
  auto layout = Container::Vertical({
    tx_type_select,
    Container::Horizontal({save_btn, cancel_btn})
  });
  
  return Renderer(layout, [&]{
    auto tx = s.getUnsignedTx();
    return vbox({
      text("Settings") | bold | center,
      separator(),
      text(""),
      hbox({
        text("Transaction Type: ") | size(WIDTH, EQUAL, 20),
        tx_type_select->Render()
      }),
      text(""),
      hbox({
        text("Network: ") | size(WIDTH, EQUAL, 20),
        text(s.getNetworkName() + " (Chain " + std::to_string(tx.chain_id) + ")")
      }),
      text(""),
      separator(),
      hbox({
        filler(),
        save_btn->Render(),
        text("  "),
        cancel_btn->Render(),
        filler()
      })
    }) | border | size(WIDTH, EQUAL, 60) | center;
  });
}

} // namespace

// Main route component (Thread-Safe)
Component MakeThreadSafeRoute(AppState& s) {
  switch(s.getRoute()) {
    case app::Route::ConnectWallet:
      return ConnectWalletView(s);
    case app::Route::USBContacts:
      return USBContactsView(s);
    case app::Route::TransactionInput:
      return TransactionInputView(s);
    case app::Route::Confirmation:
      return ConfirmationView(s);
    case app::Route::Signing:
      return SigningView(s);
    case app::Route::Result:
      return ResultView(s);
    case app::Route::Error:
      return ErrorView(s);
    case app::Route::Help:
      return HelpView(s);
    case app::Route::Settings:
      return SettingsView(s);
    default:
      return ConnectWalletView(s);
  }
}

// Main application runner (Thread-Safe)
int RunThreadSafeApp() {
  AppState state;
  
  // Load address book on startup
  loadAddressBook(state);
  
  // Use FitComponent for better terminal compatibility
  auto screen = ScreenInteractive::FitComponent();
  auto exit = screen.ExitLoopClosure();
  
  // Initialize global UI updater
  g_ui_updater = std::make_unique<UIUpdater>(&screen);
  
  // Main route renderer
  auto route = Renderer([&]{ 
    return MakeThreadSafeRoute(state)->Render(); 
  });
  
  // Layout with banner and status bar
  auto layout = Container::Vertical({ route });
  auto banner = Banner(state);
  auto status = StatusBar(state);
  
  auto root = Renderer(layout, [&]{
    return vbox({ 
      banner->Render(), 
      route->Render() | flex,
      separator(), 
      status->Render() 
    });
  });
  
  // Global keyboard shortcuts (same as before)
  root |= CatchEvent([&](Event e){
    // F1 - Help
    if (e == Event::F1) {
      state.setRoute(app::Route::Help);
      return true;
    }
    
    // Direct screen jumps (1-5)
    if (e == Event::Character('1')) {
      state.setRoute(app::Route::ConnectWallet);
      return true;
    }
    if (e == Event::Character('2')) {
      state.setRoute(app::Route::USBContacts);
      return true;
    }
    if (e == Event::Character('3')) {
      state.setRoute(app::Route::TransactionInput);
      return true;
    }
    if (e == Event::Character('4')) {
      state.setRoute(app::Route::Confirmation);
      return true;
    }
    if (e == Event::Character('5')) {
      state.setRoute(app::Route::Result);
      return true;
    }
    
    // g - Go to first screen (home)
    if (e == Event::Character('g')) {
      state.setRoute(app::Route::ConnectWallet);
      return true;
    }
    
    // u - USB scan
    if (e == Event::Character('u') && state.getRoute() != app::Route::USBContacts) {
      state.setRoute(app::Route::USBContacts);
      loadUSBContacts(state);
      return true;
    }
    
    // : - Command mode (settings for now)
    if (e == Event::Character(':')) {
      state.setRoute(app::Route::Settings);
      return true;
    }
    
    // Escape - Back navigation
    if (e == Event::Escape) {
      switch(state.getRoute()) {
        case app::Route::USBContacts:
          state.setRoute(app::Route::ConnectWallet);
          return true;
        case app::Route::TransactionInput:
          state.setRoute(app::Route::USBContacts);
          return true;
        case app::Route::Confirmation:
          state.setRoute(app::Route::TransactionInput);
          return true;
        case app::Route::Help:
        case app::Route::Settings:
        case app::Route::Error:
          state.setRoute(state.getPreviousRoute());
          return true;
        case app::Route::ConnectWallet:
          exit();
          return true;
        default:
          break;
      }
    }
    
    // Ctrl+Q - Quit
    if (e == Event::CtrlQ) {
      exit();
      return true;
    }
    
    return false;
  });
  
  // Managed animation loop (replaces the problematic detached thread)
  std::thread animator([&state](){
    while (!state.isShutdownRequested() && !g_ui_updater->isShutdownRequested()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      state.incrementAnimationFrame();
      if (g_ui_updater) {
        g_ui_updater->postUpdate();
      }
    }
  });
  
  screen.Loop(root);
  
  // Clean shutdown
  state.requestShutdown();
  if (g_ui_updater) {
    g_ui_updater->requestShutdown();
  }
  animator.join();
  g_ui_updater.reset();
  
  return 0;
}
