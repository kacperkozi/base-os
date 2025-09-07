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

// Utility: Convert Wei to ETH string
std::string weiToEth(const std::string& wei) {
  if (wei.empty() || wei == "0") return "0";
  
  // Simple conversion - TODO: Implement proper big number math
  // For now, just show Wei with note
  return wei + " Wei";
}

// Utility: Convert ETH to Wei string  
std::string ethToWei(const std::string& eth) {
  // TODO: Implement proper conversion
  // For now, accept Wei directly
  return eth;
}

// Utility: Format address for display (shorten if needed)
std::string formatAddress(const std::string& addr, bool shorten = false) {
  if (addr.length() <= 10 || !shorten) return addr;
  return addr.substr(0, 6) + "..." + addr.substr(addr.length() - 4);
}

// Utility: Load address book from file
void loadAddressBook(AppState& s) {
  // TODO: Implement actual file loading
  // For now, hardcode some test addresses
  s.known_addresses = {
    {"0x4200000000000000000000000000000000000016", "Base Bridge", "Official Base L1->L2 Bridge", app::ContactType::Contract},
    {"0x833589fCD6eDb6E08f4c7C32D4f71b54bdA02913", "USDC", "USD Coin on Base", app::ContactType::Contract},
    {"0x50c5725949A6F0c72E6C4a641F24049A917DB0Cb", "DAI", "DAI Stablecoin on Base", app::ContactType::Contract},
  };
}

// Utility: Load USB contacts from mounted devices
void loadUSBContacts(AppState& s) {
  s.is_scanning_usb = true;
  s.usb_contacts.clear();
  
  // TODO: Implement actual USB scanning for contacts.json files
  // Simulate finding contacts with different types
  std::thread([&](){
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Mock USB contacts with various types
    s.usb_contacts = {
      {"alice.eth", "Alice Johnson", "ENS name for Alice", app::ContactType::ENS},
      {"0x742d35Cc6641C154db0bEF6a74B0742e5b4b4e7c", "bob.base.eth", "Base name for Bob", app::ContactType::Base},
      {"0x8ba1f109551bD432803012645Hac136c", "Team Multisig", "Development team multisig wallet", app::ContactType::Multisig},
      {"0x1234567890abcdef1234567890abcdef12345678", "DEX Contract", "Decentralized exchange contract", app::ContactType::Contract},
      {"0x9876543210fedcba9876543210fedcba98765432", "John Doe", "Personal wallet", app::ContactType::EOA}
    };
    
    s.is_scanning_usb = false;
    s.usb_scan_complete = true;
  }).detach();
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
    case app::ContactType::ENS: return Color::Blue;         // #60a5fa
    case app::ContactType::Base: return Color::Magenta;     // #a78bfa 
    case app::ContactType::Multisig: return Color::Yellow;  // #fb923c
    case app::ContactType::Contract: return Color::Cyan;    // #22d3ee
    case app::ContactType::EOA: return Color::Green;       // #22c55e
    default: return Color::White;
  }
}

// Utility: Find address suggestion
std::string findAddressSuggestion(const std::string& input, const std::vector<app::KnownAddress>& addresses) {
  if (input.length() < 3) return "";
  
  std::string lower_input = input;
  for (auto& c : lower_input) c = std::tolower(c);
  
  for (const auto& ka : addresses) {
    std::string lower_addr = ka.address;
    for (auto& c : lower_addr) c = std::tolower(c);
    
    if (lower_addr.find(lower_input) != std::string::npos) {
      return ka.name + " (" + formatAddress(ka.address, true) + ")";
    }
    
    std::string lower_name = ka.name;
    for (auto& c : lower_name) c = std::tolower(c);
    
    if (lower_name.find(lower_input) != std::string::npos) {
      return ka.name + " - " + ka.address;
    }
  }
  
  return "";
}

// Component: Animated spinner
Element Spinner(int frame) {
  const std::vector<std::string> frames = {
    "⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"
  };
  return text(frames[frame % frames.size()]) | color(Color::GreenLight);
}

// Component: Progress dots animation
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
    auto left = text("Offline Signer • " + s.network_name + " (Chain " + std::to_string(s.unsigned_tx.chain_id) + ")") | color(Color::GrayDark);
    auto mid = s.status.empty() ? text("") : text(" " + s.status + " ") | color(Color::Green);
    auto right = text("hjkl:Move 1-5:Screens :cmd g:Home u:USB F1:Help") | color(Color::GrayDark);
    return hbox({ left, filler(), mid, filler(), right }) | bgcolor(Color::Black) | color(Color::Green);
  });
}

// Component: Error/Info banner at top
Component Banner(AppState& s) {
  return Renderer([&]{
    Elements lines;
    if (!s.error.empty()) {
      lines.push_back(hbox({
        text("[ERROR] ") | color(Color::Red) | bold,
        text(s.error) | color(Color::Red)
      }));
    }
    if (!s.info.empty()) {
      lines.push_back(hbox({
        text("[INFO] ") | color(Color::Blue) | bold,
        text(s.info) | color(Color::Blue)
      }));
    }
    if (lines.empty()) return text("");
    return vbox(std::move(lines)) | border | color(Color::Green) | bgcolor(Color::Black);
  });
}

// Screen 1: Connect Wallet
Component ConnectWalletView(AppState& s) {
  auto continue_btn = Button("Continue", [&]{
    // TODO: Actually detect hardware wallet
    // For now, simulate detection
    s.is_detecting_wallet = true;
    std::thread([&](){
      std::this_thread::sleep_for(std::chrono::seconds(1));
      s.devices = {
        {"Ledger Nano X", "/dev/hidraw0", true, false},
        {"Trezor Model T", "/dev/hidraw1", false, false}
      };
      s.wallet_connected = !s.devices.empty();
      s.is_detecting_wallet = false;
      
      if (s.wallet_connected) {
        s.route = app::Route::USBContacts;
        s.clearError();
      } else {
        s.setError("No hardware wallet detected. Please connect your device and try again.");
      }
    }).detach();
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
    if (s.is_detecting_wallet) {
      content.push_back(hbox({
        Spinner(s.animation_frame),
        text(" Detecting hardware wallets"),
        ProgressDots(s.animation_frame)
      }) | center);
      
      // Update animation
      std::thread([&](){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (s.is_detecting_wallet) s.animation_frame++;
      }).detach();
    } else if (!s.devices.empty()) {
      content.push_back(text("Detected devices:") | dim);
      for (const auto& dev : s.devices) {
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
    
    content.push_back(text(""));
    content.push_back(hbox({filler(), continue_btn->Render(), filler()}) | size(HEIGHT, EQUAL, 3));
    
    // Help text
    content.push_back(text(""));
    content.push_back(text("Once your wallet is connected and ready, press Continue") | center | dim);
    
    return vbox(std::move(content)) | border | size(WIDTH, EQUAL, 80) | center;
  });
  
  return content;
}

// Screen 2: USB Contacts
Component USBContactsView(AppState& s) {
  auto scan_btn = Button("Scan USB Devices", [&]{
    loadUSBContacts(s);
  });
  
  auto skip_btn = Button("Skip", [&]{
    s.route = app::Route::TransactionInput;
  });
  
  auto select_btn = Button("Select Contact", [&]{
    if (!s.usb_contacts.empty() && s.selected_contact < s.usb_contacts.size()) {
      auto& contact = s.usb_contacts[s.selected_contact];
      s.unsigned_tx.to = contact.address;
      s.route = app::Route::TransactionInput;
    }
  });
  
  auto back_btn = Button("Back", [&]{
    s.route = app::Route::ConnectWallet;
  });
  
  auto layout = Container::Horizontal({scan_btn, skip_btn, select_btn, back_btn});
  
  return Renderer(layout, [&]{
    Elements content;
    
    content.push_back(text(""));
    content.push_back(text("USB Contacts") | bold | center | color(Color::Green));
    content.push_back(text("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━") | center | color(Color::GrayDark));
    content.push_back(text(""));
    
    if (s.is_scanning_usb) {
      content.push_back(hbox({
        Spinner(s.animation_frame),
        text(" Scanning USB devices for contacts.json files"),
        ProgressDots(s.animation_frame)
      }) | center | color(Color::GreenLight));
      
      // Update animation
      std::thread([&](){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (s.is_scanning_usb) s.animation_frame++;
      }).detach();
    } else if (s.usb_scan_complete) {
      if (s.usb_contacts.empty()) {
        content.push_back(text("No contacts.json files found on USB devices") | center | color(Color::Yellow));
        content.push_back(text("You can skip this step or manually scan again") | center | color(Color::GrayDark));
      } else {
        content.push_back(text("Found " + std::to_string(s.usb_contacts.size()) + " contacts:") | color(Color::GreenLight));
        content.push_back(text(""));
        
        // Display contacts with selection
        for (size_t i = 0; i < s.usb_contacts.size(); ++i) {
          const auto& contact = s.usb_contacts[i];
          std::string icon = getContactIcon(contact.type);
          Color typeColor = getContactColor(contact.type);
          
          Element line;
          if (i == s.selected_contact) {
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
          
          if (i == s.selected_contact && !contact.description.empty()) {
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
    content.push_back(hbox({
      filler(),
      scan_btn->Render(),
      text("  "),
      skip_btn->Render(),
      text("  "),
      select_btn->Render() | (s.usb_contacts.empty() ? dim : color(Color::Green)),
      text("  "),
      back_btn->Render(),
      filler()
    }));
    
    return vbox(std::move(content)) | border | size(WIDTH, EQUAL, 80) | center | bgcolor(Color::Black);
  }) | CatchEvent([&](Event e) {
    // Vim-like navigation for contacts
    if (e == Event::Character('j') && s.selected_contact < s.usb_contacts.size() - 1) {
      s.selected_contact++;
      return true;
    }
    if (e == Event::Character('k') && s.selected_contact > 0) {
      s.selected_contact--;
      return true;
    }
    if (e == Event::Return && !s.usb_contacts.empty()) {
      auto& contact = s.usb_contacts[s.selected_contact];
      s.unsigned_tx.to = contact.address;
      s.route = app::Route::TransactionInput;
      return true;
    }
    if (e == Event::Character('u')) {
      loadUSBContacts(s);
      return true;
    }
    return false;
  });
}

// Screen 3: Transaction Input
Component TransactionInputView(AppState& s) {
  // Local copies for input binding
  static std::string to = s.unsigned_tx.to;
  static std::string value = s.unsigned_tx.value;
  static std::string nonce = s.unsigned_tx.nonce;
  static std::string gas_limit = s.unsigned_tx.gas_limit.empty() ? "21000" : s.unsigned_tx.gas_limit;
  static std::string gas_price = s.unsigned_tx.gas_price;
  static std::string max_fee = s.unsigned_tx.max_fee_per_gas;
  static std::string max_priority = s.unsigned_tx.max_priority_fee_per_gas;
  static std::string data = s.unsigned_tx.data.empty() ? "0x" : s.unsigned_tx.data;
  
  // Input components
  auto in_to = Input(&to, "0x...");
  auto in_value = Input(&value, "Amount in Wei (e.g., 1000000000000000000 for 1 ETH)");
  auto in_nonce = Input(&nonce, "Transaction nonce");
  auto in_gas_limit = Input(&gas_limit, "21000");
  
  Component gas_inputs;
  if (s.use_eip1559) {
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
    s.field_errors.clear();
    
    // Validate address
    if (!app::IsAddress(to)) {
      s.field_errors["to"] = "Invalid Ethereum address format";
    }
    
    // Validate numeric fields
    if (value.empty() || !std::all_of(value.begin(), value.end(), ::isdigit)) {
      s.field_errors["value"] = "Amount must be a number";
    }
    
    if (nonce.empty() || !std::all_of(nonce.begin(), nonce.end(), ::isdigit)) {
      s.field_errors["nonce"] = "Nonce must be a number";
    }
    
    if (gas_limit.empty() || !std::all_of(gas_limit.begin(), gas_limit.end(), ::isdigit)) {
      s.field_errors["gas_limit"] = "Gas limit must be a number";
    }
    
    if (!s.field_errors.empty()) {
      std::string err = "Please fix the following errors:\n";
      for (const auto& [field, msg] : s.field_errors) {
        err += "  • " + msg + "\n";
      }
      s.setError(err);
      return;
    }
    
    // Save to state
    s.unsigned_tx.to = to;
    s.unsigned_tx.value = value;
    s.unsigned_tx.nonce = nonce;
    s.unsigned_tx.gas_limit = gas_limit;
    s.unsigned_tx.data = data;
    
    if (s.use_eip1559) {
      s.unsigned_tx.max_fee_per_gas = max_fee;
      s.unsigned_tx.max_priority_fee_per_gas = max_priority;
      s.unsigned_tx.type = 2;
    } else {
      s.unsigned_tx.gas_price = gas_price;
      s.unsigned_tx.type = 0;
    }
    
    s.has_unsigned = true;
    s.clearError();
    s.route = app::Route::Confirmation;
  };
  
  auto next_btn = Button("Review Transaction", validate_and_continue);
  auto back_btn = Button("Back", [&]{ 
    s.route = app::Route::USBContacts;
  });
  
  // Build input container
  std::vector<Component> inputs = {
    in_to, in_value, in_nonce, in_gas_limit
  };
  
  if (s.use_eip1559) {
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
    
    // To address with suggestion
    content.push_back(hbox({
      text("To Address:") | size(WIDTH, EQUAL, 20) | color(Color::GreenLight),
      in_to->Render()
    }));
    
    // Show address suggestion if found
    s.address_suggestion = findAddressSuggestion(to, s.known_addresses);
    if (!s.address_suggestion.empty()) {
      content.push_back(hbox({
        text("") | size(WIDTH, EQUAL, 20),
        text("  → " + s.address_suggestion) | color(Color::GreenLight)
      }));
    }
    
    // Also check USB contacts for suggestions
    std::string usb_suggestion = findAddressSuggestion(to, s.usb_contacts);
    if (!usb_suggestion.empty() && usb_suggestion != s.address_suggestion) {
      content.push_back(hbox({
        text("") | size(WIDTH, EQUAL, 20),
        text("  → " + usb_suggestion + " (USB)") | color(Color::Cyan)
      }));
    }
    
    // Show field error if any
    if (s.field_errors.count("to")) {
      content.push_back(hbox({
        text("") | size(WIDTH, EQUAL, 20),
        text("  ⚠ " + s.field_errors["to"]) | color(Color::Red)
      }));
    }
    
    content.push_back(text(""));
    
    // Amount
    content.push_back(hbox({
      text("Amount (Wei):") | size(WIDTH, EQUAL, 20) | color(Color::GreenLight),
      in_value->Render()
    }));
    if (!value.empty() && std::all_of(value.begin(), value.end(), ::isdigit)) {
      // Show ETH equivalent
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
    if (s.use_eip1559) {
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

// Screen 4: Confirmation
Component ConfirmationView(AppState& s) {
  auto sign_btn = Button("Sign Transaction", [&]{
    s.route = app::Route::Signing;
    s.is_signing = true;
    s.animation_frame = 0;
    
    // TODO: Actual hardware wallet signing
    // Simulate signing process
    std::thread([&](){
      std::this_thread::sleep_for(std::chrono::seconds(3));
      
      // Mock signed transaction
      s.signed_hex = "0xf86c0185046c7cfe0083016dea94" + 
                     s.unsigned_tx.to.substr(2) +
                     "880de0b6b3a764000080269fc7eaaa9c21f59adf8ad43ed66cf5ef9ee1c317bd4d32cd65401e7aacbda51687";
      s.has_signed = true;
      s.is_signing = false;
      s.route = app::Route::Result;
    }).detach();
  });
  
  auto edit_btn = Button("Edit", [&]{
    s.route = app::Route::TransactionInput;
  });
  
  auto layout = Container::Horizontal({sign_btn, edit_btn});
  
  return Renderer(layout, [&]{
    auto& tx = s.unsigned_tx;
    
    Elements content;
    content.push_back(text("Review Transaction") | bold | center | color(Color::Green));
    content.push_back(text("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━") | center | color(Color::GrayDark));
    content.push_back(text(""));
    content.push_back(text("Please review the following details carefully:") | color(Color::GrayDark));
    content.push_back(text(""));
    
    // Transaction details box
    Elements details;
    
    // To address with known name if available
    details.push_back(hbox({
      text("To: ") | bold | color(Color::Green),
      text(tx.to) | color(Color::GreenLight)
    }));
    
    // Check if known address
    for (const auto& ka : s.known_addresses) {
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
      text(s.network_name + " (Chain ID: " + std::to_string(tx.chain_id) + ")")
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

// Screen 5: Signing Progress
Component SigningView(AppState& s) {
  auto cancel_btn = Button("Cancel", [&]{
    // TODO: Actually cancel hardware wallet operation
    s.is_signing = false;
    s.route = app::Route::Confirmation;
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
      Spinner(s.animation_frame),
      text("  Please confirm the transaction on your hardware wallet") | color(Color::Green),
      ProgressDots(s.animation_frame),
      filler()
    }) | bold);
    
    content.push_back(text(""));
    content.push_back(text(""));
    
    // Device-specific instructions
    if (s.selected_device < s.devices.size()) {
      auto& device = s.devices[s.selected_device];
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
    int progress = (s.animation_frame * 5) % 100;
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
    
    // Update animation
    std::thread([&](){
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if (s.is_signing) s.animation_frame++;
    }).detach();
    
    return vbox(std::move(content)) | border | size(WIDTH, EQUAL, 80) | center;
  });
}

// Screen 6: Result with QR Code
Component ResultView(AppState& s) {
  static std::string save_path = "/home/user/signed_transaction.txt";
  static std::string save_status;
  static int current_qr_part = 0;
  
  auto save_to_file = [&](){
    // TODO: Actually save to file
    std::ofstream file(save_path);
    if (file.good()) {
      file << s.signed_hex << std::endl;
      save_status = "[OK] Saved to " + save_path;
    } else {
      save_status = "[ERROR] Failed to save file";
    }
  };
  
  auto save_btn = Button("Save to File", save_to_file);
  auto new_tx_btn = Button("New Transaction", [&]{
    s.clearTransaction();
    s.route = app::Route::TransactionInput;
    current_qr_part = 0; // Reset QR part when starting new transaction
  });

  auto next_qr_btn = Button("Next QR >", [&]{
    if (!s.qr_codes.empty() && current_qr_part < static_cast<int>(s.qr_codes.size()) - 1) {
      current_qr_part++;
    }
  });

  auto prev_qr_btn = Button("< Prev QR", [&]{
    if (current_qr_part > 0) {
      current_qr_part--;
    }
  });
  auto exit_btn = Button("Exit", [&]{
    // TODO: Proper exit handling
    std::exit(0);
  });
  
  auto layout = Container::Horizontal({prev_qr_btn, next_qr_btn, save_btn, new_tx_btn, exit_btn});
  
  return Renderer(layout, [&]{
    Elements content;
    
    content.push_back(text("Transaction Signed Successfully!") | bold | center | color(Color::Green));
    content.push_back(text("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━") | center | dim);
    content.push_back(text(""));
    
    // Instructions
    content.push_back(text("Scan the QR code below with an online device to broadcast the transaction") | center | color(Color::GreenLight));
    content.push_back(text(""));
    
    // QR Code
    if (!s.signed_hex.empty()) {
      // Generate QR codes with chunking (max 100 chars per chunk)
      s.qr_codes = app::GenerateQRs(s.signed_hex, 100);
      
      if (!s.qr_codes.empty()) {
        // Ensure current_qr_part is within bounds
        if (current_qr_part >= static_cast<int>(s.qr_codes.size())) {
          current_qr_part = 0;
        }
        
        auto& qrCode = s.qr_codes[current_qr_part];
        
        if (qrCode.size > 0) {
          // Always use compact ASCII for multi-part QR codes to ensure they fit
          std::string qrAscii = qrCode.toCompactAscii();
          
          std::istringstream stream(qrAscii);
          std::string line;
          Elements qrLines;
          while (std::getline(stream, line)) {
            qrLines.push_back(text(line) | center);
          }
          
          content.push_back(vbox(std::move(qrLines)) | border);

          // Show part information and navigation if multi-part
          if (qrCode.total_parts > 1) {
            content.push_back(text("Part " + std::to_string(qrCode.part) + " of " + std::to_string(qrCode.total_parts)) | center | color(Color::Yellow));
            content.push_back(hbox({
              prev_qr_btn->Render(),
              filler(),
              text("Use ← → to navigate QR parts") | center | dim,
              filler(),
              next_qr_btn->Render()
            }));
          }
        } else {
          content.push_back(text("Failed to generate QR code for part " + std::to_string(qrCode.part)) | center | color(Color::Red));
        }
      } else {
        content.push_back(text("Failed to generate QR codes") | center | color(Color::Red));
      }
    }
    
    content.push_back(text(""));
    
    // Raw hex (collapsible)
    content.push_back(text("Signed Transaction Hex:") | color(Color::GrayDark));
    
    // Break hex into multiple lines for readability
    std::string hex = s.signed_hex;
    for (size_t i = 0; i < hex.length(); i += 64) {
      content.push_back(text(hex.substr(i, 64)) | color(Color::GrayDark) | center);
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

// Error Screen
Component ErrorView(AppState& s) {
  auto retry_btn = Button("Retry", [&]{
    s.clearError();
    s.route = s.previous_route;
  });
  
  auto home_btn = Button("Start Over", [&]{
    s.clearError();
    s.clearTransaction();
    s.route = app::Route::ConnectWallet;
  });
  
  auto layout = Container::Horizontal({retry_btn, home_btn});
  
  return Renderer(layout, [&]{
    return vbox({
      text(""),
      text("Error Occurred") | bold | center | color(Color::Red),
      text("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━") | center | color(Color::Red),
      text(""),
      text(s.last_error) | center,
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

// Help Screen
Component HelpView(AppState& s) {
  auto back_btn = Button("Back", [&]{
    s.route = s.previous_route;
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

// Settings Screen
Component SettingsView(AppState& s) {
  static int tx_type = s.use_eip1559 ? 1 : 0;
  std::vector<std::string> tx_types = {"Legacy", "EIP-1559"};
  auto tx_type_select = Radiobox(&tx_types, &tx_type);
  
  auto save_btn = Button("Save", [&]{
    s.use_eip1559 = (tx_type == 1);
    s.route = s.previous_route;
  });
  
  auto cancel_btn = Button("Cancel", [&]{
    s.route = s.previous_route;
  });
  
  auto layout = Container::Vertical({
    tx_type_select,
    Container::Horizontal({save_btn, cancel_btn})
  });
  
  return Renderer(layout, [&]{
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
        text(s.network_name + " (Chain " + std::to_string(s.unsigned_tx.chain_id) + ")")
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

// Main route component
Component MakeRoute(AppState& s) {
  switch(s.route) {
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

// Main application runner
int RunApp() {
  AppState state;
  
  // Load address book on startup
  loadAddressBook(state);
  
  // Use FitComponent for better terminal compatibility
  // Fullscreen mode can cause display issues on some terminals
  auto screen = ScreenInteractive::FitComponent();
  auto exit = screen.ExitLoopClosure();
  
  // Main route renderer
  auto route = Renderer([&]{ 
    return MakeRoute(state)->Render(); 
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
  
  // Global keyboard shortcuts
  root |= CatchEvent([&](Event e){
    // F1 - Help
    if (e == Event::F1) {
      state.previous_route = state.route;
      state.route = app::Route::Help;
      return true;
    }
    
    // Direct screen jumps (1-5)
    if (e == Event::Character('1')) {
      state.route = app::Route::ConnectWallet;
      return true;
    }
    if (e == Event::Character('2')) {
      state.route = app::Route::USBContacts;
      return true;
    }
    if (e == Event::Character('3')) {
      state.route = app::Route::TransactionInput;
      return true;
    }
    if (e == Event::Character('4')) {
      state.route = app::Route::Confirmation;
      return true;
    }
    if (e == Event::Character('5')) {
      state.route = app::Route::Result;
      return true;
    }
    
    // g - Go to first screen (home)
    if (e == Event::Character('g')) {
      state.route = app::Route::ConnectWallet;
      return true;
    }
    
    // u - USB scan
    if (e == Event::Character('u') && state.route != app::Route::USBContacts) {
      state.route = app::Route::USBContacts;
      loadUSBContacts(state);
      return true;
    }
    
    // : - Command mode (settings for now)
    if (e == Event::Character(':')) {
      state.previous_route = state.route;
      state.route = app::Route::Settings;
      return true;
    }
    
    // Vim-like navigation (h/j/k/l) - basic left/down/up/right
    if (e == Event::Character('h')) {
      // Navigate backward in flow
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
        case app::Route::Signing:
          state.route = app::Route::Confirmation;
          return true;
        case app::Route::Result:
          state.route = app::Route::Signing;
          return true;
        default:
          break;
      }
    }
    
    if (e == Event::Character('l')) {
      // Navigate forward in flow
      switch(state.route) {
        case app::Route::ConnectWallet:
          if (state.wallet_connected) {
            state.route = app::Route::USBContacts;
            return true;
          }
          break;
        case app::Route::USBContacts:
          state.route = app::Route::TransactionInput;
          return true;
        case app::Route::TransactionInput:
          if (state.has_unsigned) {
            state.route = app::Route::Confirmation;
            return true;
          }
          break;
        case app::Route::Confirmation:
          state.route = app::Route::Signing;
          return true;
        case app::Route::Signing:
          if (state.has_signed) {
            state.route = app::Route::Result;
            return true;
          }
          break;
        default:
          break;
      }
    }
    
    // Escape - Back navigation
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
        case app::Route::Error:
          state.route = state.previous_route;
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
    
    // Ctrl+S - Settings
    if (e == Event::CtrlS && state.route != app::Route::Settings) {
      state.previous_route = state.route;
      state.route = app::Route::Settings;
      return true;
    }
    
    return false;
  });
  
  screen.Loop(root);
  return 0;
}