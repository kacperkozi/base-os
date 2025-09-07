#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/component/event.hpp>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>
#include <map>

using namespace ftxui;

// Contact data structure
struct Contact {
  std::string id;
  std::string name;
  std::string address;
  std::string type; // "ens", "base", "multisig", "contract"
  std::string ensName;
  std::string baseName;
  std::vector<std::string> multisigOwners;
  int threshold = 0;
};

// Address book for autocomplete
struct AddressEntry {
  std::string address;
  std::string name;
  std::string type;
};

// Comprehensive TUI Application
int RunSimpleTransaction() {
  // Application state
  enum class Screen { CONNECT_WALLET, USB_CONTACTS, TRANSACTION_INPUT, CONFIRMATION, RESULT };
  Screen current_screen = Screen::CONNECT_WALLET;
  std::vector<Screen> navigation_history = {Screen::CONNECT_WALLET};
  
  // UI state
  int focused_element = 0;
  bool show_help = false;
  bool show_confirm_dialog = false;
  std::string confirm_dialog_message = "";
  std::string command_buffer = "";
  bool command_mode = false;
  
  // Transaction form data
  std::map<std::string, std::string> form_data = {
    {"toAddress", ""},
    {"amount", ""},
    {"nonce", ""},
    {"gasPrice", ""},
    {"gasLimit", ""}
  };
  
  // USB contacts state
  std::vector<Contact> contacts;
  int selected_contact_index = -1;
  bool is_scanning = false;
  
  // Autocomplete state
  std::vector<AddressEntry> address_book = {
    {"0x742d35Cc6634C0532925a3b8D4C9db96590c6C87", "Alice.eth", "ens"},
    {"0x8ba1f109551bD432803012645Hac136c22C177ec", "Bob.base.eth", "base"},
    {"0x1234567890123456789012345678901234567890", "Treasury Safe", "multisig"},
    {"0x9876543210987654321098765432109876543210", "Charlie.eth", "ens"},
    {"0xabcdefabcdefabcdefabcdefabcdefabcdefabcd", "Dev Multisig", "multisig"},
    {"0xd8dA6BF26964aF9D7eEd9e03E53415D37aA96045", "Vitalik.eth", "ens"},
    {"0x3fC91A3afd70395Cd496C647d5a6CC9D4B2b7FAD", "Uniswap V3", "contract"},
    {"0x7a250d5630B4cF539739dF2C5dAcb4c659F2488D", "Uniswap V2 Router", "contract"}
  };
  
  std::vector<AddressEntry> autocomplete_results;
  bool show_autocomplete = false;
  int autocomplete_index = 0;
  
  // Transaction state
  bool is_signing = false;
  std::string tx_hash = "";
  std::string status_message = "Welcome to Offline Signer";
  
  // Mock contacts data
  auto load_mock_contacts = [&]() {
    contacts = {
      {"1", "Alice", "0x742d35Cc6634C0532925a3b8D4C9db96590c6C87", "ens", "alice.eth", "", {}, 0},
      {"2", "Bob Base", "0x8ba1f109551bD432803012645Hac136c22C177ec", "base", "", "bob.base.eth", {}, 0},
      {"3", "Treasury Safe", "0x1234567890123456789012345678901234567890", "multisig", "", "", {"0x111...", "0x222...", "0x333..."}, 2},
      {"4", "Charlie", "0x9876543210987654321098765432109876543210", "ens", "charlie.eth", "", {}, 0},
      {"5", "Dev Multisig", "0xabcdefabcdefabcdefabcdefabcdefabcdefabcd", "multisig", "", "", {"0xaaa...", "0xbbb...", "0xccc...", "0xddd..."}, 3}
    };
    selected_contact_index = 0;
  };
  
  // Navigation functions
  auto navigate_to_screen = [&](Screen screen) {
    if (screen != current_screen) {
      navigation_history.push_back(screen);
      current_screen = screen;
      focused_element = 0;
    }
  };
  
  auto go_back = [&]() {
    if (navigation_history.size() > 1) {
      navigation_history.pop_back();
      current_screen = navigation_history.back();
      focused_element = 0;
    }
  };
  
  // Autocomplete filtering
  auto filter_addresses = [&](const std::string& input) {
    if (input.length() < 2) {
      show_autocomplete = false;
      autocomplete_results.clear();
      return;
    }
    
    autocomplete_results.clear();
    for (const auto& entry : address_book) {
      std::string lower_input = input;
      std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);
      std::string lower_address = entry.address;
      std::transform(lower_address.begin(), lower_address.end(), lower_address.begin(), ::tolower);
      std::string lower_name = entry.name;
      std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
      
      if (lower_address.find(lower_input) != std::string::npos || 
          lower_name.find(lower_input) != std::string::npos) {
        autocomplete_results.push_back(entry);
      }
    }
    
    show_autocomplete = !autocomplete_results.empty();
    autocomplete_index = 0;
  };
  
  // USB scanning simulation
  auto simulate_usb_scan = [&]() {
    is_scanning = true;
    std::thread([&]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(2000));
      load_mock_contacts();
      is_scanning = false;
    }).detach();
  };
  
  // Transaction signing simulation
  auto simulate_signing = [&]() {
    is_signing = true;
    std::thread([&]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(3000));
      tx_hash = "0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
      is_signing = false;
      navigate_to_screen(Screen::RESULT);
    }).detach();
  };
  
  // Input components
  auto to_address_input = Input(&form_data["toAddress"], "0x... (start typing for suggestions)");
  auto amount_input = Input(&form_data["amount"], "0.0");
  auto nonce_input = Input(&form_data["nonce"], "0");
  auto gas_price_input = Input(&form_data["gasPrice"], "20");
  auto gas_limit_input = Input(&form_data["gasLimit"], "21000");
  
  std::vector<Component> form_inputs = {
    to_address_input, amount_input, nonce_input, gas_price_input, gas_limit_input
  };
  
  // Event handling for comprehensive keyboard navigation
  auto main_component = CatchEvent(Container::Vertical({}), [&](Event event) {
    // Handle confirmation dialog
    if (show_confirm_dialog) {
      if (event == Event::Character('y') || event == Event::Character('Y')) {
        if (confirm_dialog_message.find("quit") != std::string::npos) {
          std::exit(0);
        } else if (confirm_dialog_message.find("clear") != std::string::npos) {
          for (auto& pair : form_data) {
            pair.second = "";
          }
        }
        show_confirm_dialog = false;
        return true;
      }
      if (event == Event::Character('n') || event == Event::Character('N') || event == Event::Escape) {
        show_confirm_dialog = false;
        return true;
      }
      return true;
    }
    
    // Handle command mode
    if (command_mode) {
      if (event == Event::Escape) {
        command_mode = false;
        command_buffer = "";
        return true;
      }
      if (event == Event::Return) {
        // Execute command
        if (command_buffer == "help" || command_buffer == "h") {
          show_help = !show_help;
        } else if (command_buffer == "quit" || command_buffer == "q") {
          show_confirm_dialog = true;
          confirm_dialog_message = "Are you sure you want to quit?";
        } else if (command_buffer == "next" || command_buffer == "n") {
          if (current_screen != Screen::RESULT) {
            navigate_to_screen(static_cast<Screen>(static_cast<int>(current_screen) + 1));
          }
        } else if (command_buffer == "back" || command_buffer == "b") {
          go_back();
        } else if (command_buffer == "scan" || command_buffer == "usb") {
          if (current_screen == Screen::USB_CONTACTS) {
            simulate_usb_scan();
          }
        }
        command_mode = false;
        command_buffer = "";
        return true;
      }
      if (event == Event::Backspace) {
        if (!command_buffer.empty()) {
          command_buffer.pop_back();
        }
        return true;
      }
      if (event.is_character()) {
        command_buffer += event.character();
        return true;
      }
      return true;
    }
    
    // Handle autocomplete navigation
    if (show_autocomplete && current_screen == Screen::TRANSACTION_INPUT && focused_element == 0) {
      if (event == Event::ArrowDown || event == Event::Character('j')) {
        autocomplete_index = (autocomplete_index + 1) % autocomplete_results.size();
        return true;
      }
      if (event == Event::ArrowUp || event == Event::Character('k')) {
        autocomplete_index = (autocomplete_index - 1 + autocomplete_results.size()) % autocomplete_results.size();
        return true;
      }
      if (event == Event::Tab) {
        if (!autocomplete_results.empty()) {
          form_data["toAddress"] = autocomplete_results[autocomplete_index].address;
          show_autocomplete = false;
        }
        return true;
      }
      if (event == Event::Escape) {
        show_autocomplete = false;
        return true;
      }
    }
    
    // Global navigation keys
    if (event == Event::Character('?') || event.input() == "F1") {
      show_help = !show_help;
      return true;
    }
    
    if (event == Event::Character(':')) {
      command_mode = true;
      command_buffer = "";
      return true;
    }
    
    if (event == Event::Character('q')) {
      show_confirm_dialog = true;
      confirm_dialog_message = "Are you sure you want to quit?";
      return true;
    }
    
    // Screen-specific navigation
    if (event == Event::Return) {
      switch (current_screen) {
        case Screen::CONNECT_WALLET:
          navigate_to_screen(Screen::USB_CONTACTS);
          return true;
        case Screen::USB_CONTACTS:
          if (!contacts.empty() && selected_contact_index >= 0) {
            form_data["toAddress"] = contacts[selected_contact_index].address;
            navigate_to_screen(Screen::TRANSACTION_INPUT);
          } else if (!is_scanning) {
            simulate_usb_scan();
          }
          return true;
        case Screen::TRANSACTION_INPUT:
          navigate_to_screen(Screen::CONFIRMATION);
          return true;
        case Screen::CONFIRMATION:
          if (!is_signing) {
            simulate_signing();
          }
          return true;
        case Screen::RESULT:
          // Reset and start over
          for (auto& pair : form_data) {
            pair.second = "";
          }
          contacts.clear();
          selected_contact_index = -1;
          tx_hash = "";
          navigation_history = {Screen::CONNECT_WALLET};
          navigate_to_screen(Screen::CONNECT_WALLET);
          return true;
      }
    }
    
    if (event == Event::Escape || event == Event::Character('b') || event == Event::Backspace) {
      go_back();
      return true;
    }
    
    // Screen navigation shortcuts
    if (event == Event::Character('1')) { navigate_to_screen(Screen::CONNECT_WALLET); return true; }
    if (event == Event::Character('2')) { navigate_to_screen(Screen::USB_CONTACTS); return true; }
    if (event == Event::Character('3')) { navigate_to_screen(Screen::TRANSACTION_INPUT); return true; }
    if (event == Event::Character('4')) { navigate_to_screen(Screen::CONFIRMATION); return true; }
    if (event == Event::Character('5')) { navigate_to_screen(Screen::RESULT); return true; }
    
    // USB contacts navigation
    if (current_screen == Screen::USB_CONTACTS && !contacts.empty()) {
      if (event == Event::ArrowDown || event == Event::Character('j')) {
        selected_contact_index = (selected_contact_index + 1) % contacts.size();
        return true;
      }
      if (event == Event::ArrowUp || event == Event::Character('k')) {
        selected_contact_index = (selected_contact_index - 1 + contacts.size()) % contacts.size();
        return true;
      }
    }
    
    // Transaction input form navigation
    if (current_screen == Screen::TRANSACTION_INPUT) {
      if (event == Event::Tab) {
        focused_element = (focused_element + 1) % 5;
        return true;
      }
      if (event == Event::ArrowDown || event == Event::Character('j')) {
        focused_element = (focused_element + 1) % 5;
        return true;
      }
      if (event == Event::ArrowUp || event == Event::Character('k')) {
        focused_element = (focused_element - 1 + 5) % 5;
        return true;
      }
    }
    
    // USB scan shortcut
    if (event == Event::Character('u') && current_screen == Screen::USB_CONTACTS) {
      simulate_usb_scan();
      return true;
    }
    
    return false;
  });
  
  // Monitor address input for autocomplete
  to_address_input = to_address_input | CatchEvent([&](Event event) {
    if (event.is_character() || event == Event::Backspace) {
      // Delay filtering to allow input to update
      std::thread([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        filter_addresses(form_data["toAddress"]);
      }).detach();
    }
    return false;
  });
  
  auto renderer = Renderer(main_component, [&] {
    // Build screen content based on current screen
    Element content;
    
    switch (current_screen) {
      case Screen::CONNECT_WALLET:
        content = vbox({
          text("") | center,
          text("[WALLET]") | center | size(HEIGHT, EQUAL, 3),
          text("Welcome to Offline Signer") | bold | center | color(Color::Green),
          text("") | center,
          text("Please connect your hardware wallet and ensure it's unlocked.") | center | color(Color::GreenLight),
          text("") | center,
          vbox({
            text("┌─ Hardware Wallet Status ─┐") | center,
            text("│ Status: Detecting...      │") | center,
            text("│ Device: Not Connected     │") | center,
            text("└───────────────────────────┘") | center
          }) | border | center,
          text("") | center,
          text("[Enter/l/→] Continue") | center | color(Color::Yellow)
        });
        break;
        
      case Screen::USB_CONTACTS:
        if (is_scanning) {
          content = vbox({
            text("") | center,
            text("[SCAN]") | center | size(HEIGHT, EQUAL, 3),
            text("Scanning USB Devices...") | bold | center | color(Color::Yellow),
            text("") | center,
            text("Looking for contacts.json files on connected USB drives.") | center | color(Color::GreenLight),
            text("") | center,
            text("Checking mounted drives...") | center | dim
          });
        } else if (contacts.empty()) {
          content = vbox({
            text("") | center,
            text("[USB]") | center | size(HEIGHT, EQUAL, 3),
            text("USB Contacts Manager") | bold | center | color(Color::Blue),
            text("") | center,
            text("No contacts loaded. Insert USB drive and scan for contacts.json") | center | color(Color::GreenLight),
            text("") | center,
            text("[Enter/u] Scan USB Drives") | center | color(Color::Yellow),
            text("[Esc/b/⌫] Back") | center | color(Color::Yellow)
          });
        } else {
          Elements contact_list;
          for (size_t i = 0; i < contacts.size(); i++) {
            const auto& contact = contacts[i];
            bool is_selected = (int)i == selected_contact_index;
            
            std::string type_indicator = contact.type == "ens" ? "ENS" :
                                       contact.type == "base" ? "BASE" :
                                       contact.type == "multisig" ? "MULTI" : "CONT";
            
            auto contact_element = hbox({
              text(is_selected ? "► " : "  "),
              text("[" + type_indicator + "] ") | (contact.type == "ens" ? color(Color::Blue) :
                                                  contact.type == "base" ? color(Color::Magenta) :
                                                  contact.type == "multisig" ? color(Color::Yellow) : color(Color::Cyan)),
              text(contact.name) | bold,
              filler(),
              text(contact.address.substr(0, 10) + "...") | dim
            });
            
            if (is_selected) {
              contact_element = contact_element | bgcolor(Color::Green) | color(Color::Black);
            }
            
            contact_list.push_back(contact_element);
          }
          
          content = vbox({
            text("[CONTACTS] USB Contacts Manager") | bold | center | color(Color::Blue),
            separator(),
            text(""),
            text("Found " + std::to_string(contacts.size()) + " contacts • Use j/k or ↓↑ to navigate • Enter to select") | center | dim,
            text(""),
            vbox(contact_list) | border | size(HEIGHT, LESS_THAN, 15),
            text(""),
            hbox({
              text("[Esc/b/⌫] Back") | color(Color::Yellow),
              text("  "),
              text("[u] Rescan USB") | color(Color::Blue),
              text("  "),
              text("[Enter] Continue") | color(Color::Green)
            }) | center
          });
        }
        break;
        
      case Screen::TRANSACTION_INPUT: {
        std::vector<std::string> field_labels = {
          "To Address:", "Amount (ETH):", "Nonce:", "Gas Price (Gwei):", "Gas Limit:"
        };
        std::vector<std::string> field_keys = {
          "toAddress", "amount", "nonce", "gasPrice", "gasLimit"
        };
        
        Elements form_elements;
        for (size_t i = 0; i < field_labels.size(); i++) {
          bool is_focused = focused_element == (int)i;
          
          auto field_element = hbox({
            text(is_focused ? "► " : "  "),
            text("[" + std::to_string(i+1) + "] " + field_labels[i]) | size(WIDTH, EQUAL, 20) | 
              (is_focused ? color(Color::GreenLight) | bold : color(Color::Green)),
            form_inputs[i]->Render() | (is_focused ? bgcolor(Color::Green) | color(Color::Black) : color(Color::GreenLight))
          });
          
          form_elements.push_back(field_element);
          if (i < field_labels.size() - 1) {
            form_elements.push_back(text(""));
          }
        }
        
        // Add autocomplete dropdown
        if (show_autocomplete && focused_element == 0 && !autocomplete_results.empty()) {
          Elements autocomplete_elements;
          autocomplete_elements.push_back(text("Suggestions (" + std::to_string(autocomplete_results.size()) + " matches):") | 
                                        color(Color::Yellow) | bold);
          
          for (size_t i = 0; i < std::min((size_t)5, autocomplete_results.size()); i++) {
            const auto& result = autocomplete_results[i];
            bool is_highlighted = (int)i == autocomplete_index;
            
            auto suggestion_element = hbox({
              text(is_highlighted ? "► " : "  "),
              text("[" + result.type + "] ") | (result.type == "ens" ? color(Color::Blue) :
                                              result.type == "base" ? color(Color::Magenta) :
                                              result.type == "multisig" ? color(Color::Yellow) : color(Color::Cyan)),
              text(result.name) | bold,
              filler(),
              text(result.address.substr(0, 12) + "...") | dim
            });
            
            if (is_highlighted) {
              suggestion_element = suggestion_element | bgcolor(Color::Yellow) | color(Color::Black);
            }
            
            autocomplete_elements.push_back(suggestion_element);
          }
          
          form_elements.push_back(text(""));
          form_elements.push_back(vbox(autocomplete_elements) | border | color(Color::Yellow));
          form_elements.push_back(text("Use j/k or ↓↑ to navigate • Tab to select • Esc to close") | center | dim | color(Color::Yellow));
        }
        
        content = vbox({
          text("[FORM] Transaction Details") | bold | center | color(Color::Green),
          separator(),
          text(""),
          vbox(form_elements),
          text(""),
          hbox({
            text("[Esc/b/⌫] Back") | color(Color::Yellow),
            text("  "),
            text("[Tab/j/k] Navigate") | color(Color::Blue),
            text("  "),
            text("[Enter] Continue") | color(Color::Green)
          }) | center
        });
        break;
      }
        
      case Screen::CONFIRMATION:
        if (is_signing) {
          content = vbox({
            text("") | center,
            text("[SIGN]") | center | size(HEIGHT, EQUAL, 3),
            text("Signing Transaction...") | bold | center | color(Color::Magenta),
            text("") | center,
            text("[WAIT] Processing your transaction...") | center | color(Color::Yellow),
            text("[SECURE] Secure signing in progress") | center | color(Color::Green),
            text("") | center,
            text("Please wait, do not close the application") | center | dim,
            text("") | center,
            text("Awaiting confirmation on device...") | center | dim
          });
        } else {
          // Calculate total cost
          double amount = 0.0, gas_price = 0.0, gas_limit = 0.0;
          try {
            amount = std::stod(form_data["amount"]);
            gas_price = std::stod(form_data["gasPrice"]);
            gas_limit = std::stod(form_data["gasLimit"]);
          } catch (...) {}
          
          double total_cost = amount + (gas_price * gas_limit / 1e9);
          
          content = vbox({
            text("[REVIEW] Review Transaction") | bold | center | color(Color::Blue),
            separator(),
            text(""),
            text("Transaction Summary") | center | bold,
            text(""),
            hbox({
              text("To: ") | size(WIDTH, EQUAL, 15) | bold,
              text(form_data["toAddress"].empty() ? "Not specified" : form_data["toAddress"]) | color(Color::Cyan)
            }),
            text(""),
            hbox({
              text("Amount: ") | size(WIDTH, EQUAL, 15) | bold,
              text((form_data["amount"].empty() ? "0" : form_data["amount"]) + " ETH") | color(Color::Yellow)
            }),
            text(""),
            hbox({
              text("Nonce: ") | size(WIDTH, EQUAL, 15) | bold,
              text(form_data["nonce"].empty() ? "0" : form_data["nonce"]) | color(Color::Green)
            }),
            text(""),
            hbox({
              text("Gas Price: ") | size(WIDTH, EQUAL, 15) | bold,
              text((form_data["gasPrice"].empty() ? "0" : form_data["gasPrice"]) + " Gwei") | color(Color::Green)
            }),
            text(""),
            hbox({
              text("Gas Limit: ") | size(WIDTH, EQUAL, 15) | bold,
              text(form_data["gasLimit"].empty() ? "21000" : form_data["gasLimit"]) | color(Color::Green)
            }),
            separator(),
            hbox({
              text("Total Cost: ") | size(WIDTH, EQUAL, 15) | bold,
              text(std::to_string(total_cost).substr(0, 8) + " ETH") | color(Color::Red) | bold
            }),
            text(""),
            text("[WARNING] Please verify all details before signing") | center | color(Color::Yellow),
            text(""),
            hbox({
              text("[Esc/h/←] Back") | color(Color::Yellow),
              text("  "),
              text("[Enter/s] Sign Transaction") | color(Color::Green)
            }) | center
          });
        }
        break;
        
      case Screen::RESULT: {
        // Mock QR code representation
        Elements qr_lines;
        for (int i = 0; i < 8; i++) {
          std::string qr_line = "";
          for (int j = 0; j < 16; j++) {
            qr_line += ((i + j) % 3 == 0) ? "██" : "  ";
          }
          qr_lines.push_back(text(qr_line) | bgcolor(Color::Green) | color(Color::Black));
        }
        
        std::string mock_signed_tx = "0xf86c0a8504a817c8008252089435353535353535353535353535353535880de0b6b3a76400008025a04f4c17305743700648bc4f6cd3038ec6f6af0df73e31757d8b9f8dc5c4c0c93739a06b6b6974e48386f05e5fcb2a13b61b5b4680a2b17b87b7101";
        
        content = vbox({
          text("[SUCCESS] Transaction Signed Successfully") | bold | center | color(Color::Green),
          separator(),
          text(""),
          text("QR Code") | center | bold,
          vbox(qr_lines) | center | border,
          text(""),
          text("Signed Transaction (Hex)") | center | bold,
          text(mock_signed_tx.substr(0, 60) + "...") | center | color(Color::Cyan) | dim,
          text(""),
          text("Scan the QR code with an internet-connected device to broadcast,") | center | color(Color::GreenLight),
          text("or copy the hex string above.") | center | color(Color::GreenLight),
          text(""),
          text("[Enter/q/r] Sign Another Transaction") | center | color(Color::Yellow)
        });
        break;
      }
    }
    
    // Build the full UI
    Elements ui_elements;
    
    // Header
    ui_elements.push_back(vbox({
      text("╔══════════════════════════════════════════════════════════════╗") | center,
      text("║                    OFFLINE SIGNER v1.0                      ║") | center | bold | color(Color::Green),
      text("╚══════════════════════════════════════════════════════════════╝") | center
    }));
    
    // Navigation history
    if (navigation_history.size() > 1) {
      std::string history_text = "Navigation: ";
      std::vector<std::string> screen_names = {"Connect Wallet", "USB Contacts", "Transaction Input", "Confirmation", "Result"};
      for (size_t i = 0; i < navigation_history.size(); i++) {
        history_text += screen_names[(int)navigation_history[i]];
        if (i < navigation_history.size() - 1) history_text += " → ";
      }
      ui_elements.push_back(text(history_text) | center | color(Color::Blue) | dim);
    }
    
    // Command mode indicator
    if (command_mode) {
      ui_elements.push_back(text("Command Mode: :" + command_buffer + "_") | center | color(Color::Yellow) | bgcolor(Color::Blue));
    }
    
    // Screen tabs
    Elements tabs;
    std::vector<std::string> screen_names = {"Connect Wallet", "USB Contacts", "Transaction Input", "Confirmation", "Result"};
    for (int i = 0; i < 5; i++) {
      auto tab_element = text("[" + std::to_string(i+1) + "] " + screen_names[i]);
      if (i == (int)current_screen) {
        tab_element = tab_element | bgcolor(Color::Green) | color(Color::Black) | bold;
      } else if (i < (int)current_screen) {
        tab_element = tab_element | color(Color::GreenLight);
      } else {
        tab_element = tab_element | color(Color::Green) | dim;
      }
      tabs.push_back(tab_element);
    }
    ui_elements.push_back(hbox(tabs) | center | border);
    
    // Main content
    ui_elements.push_back(content | border | size(HEIGHT, GREATER_THAN, 20));
    
    // Help system
    if (show_help) {
      ui_elements.push_back(vbox({
        text("Keyboard Shortcuts:") | bold | color(Color::Blue),
        hbox({
          vbox({
            text("h/← : Previous screen"),
            text("l/→ : Next screen"),
            text("j/↓ : Move down"),
            text("k/↑ : Move up"),
            text("Enter : Select/Confirm"),
            text("Esc/b/⌫ : Back")
          }),
          text("  "),
          vbox({
            text("Tab : Navigate fields"),
            text("1-5 : Jump to screen"),
            text("g : Go to first screen"),
            text("q/r : Quit/Restart"),
            text(": : Command mode"),
            text("?/F1 : Toggle help")
          })
        }),
        text("Commands: help, quit, next, prev/back, clear, sign, home, 1-5") | dim
      }) | border | color(Color::Blue));
    }
    
    // Confirmation dialog
    if (show_confirm_dialog) {
      ui_elements.push_back(vbox({
        text(confirm_dialog_message) | center | color(Color::Yellow),
        text(""),
        hbox({
          text("[Y] Yes") | color(Color::Red),
          text("  "),
          text("[N] No") | color(Color::Green)
        }) | center,
        text("Press Y to confirm, N or Esc to cancel") | center | dim
      }) | border | bgcolor(Color::Black) | color(Color::Yellow));
    }
    
    // Footer help
    std::string footer_text = "Navigation: h/j/k/l or ←↓↑→ • Enter: select • Esc/b/⌫: back • Tab: cycle • 1-5: screens • q: quit • ?: help";
    if (command_mode) {
      footer_text = "Command Mode Active - Type command and press Enter";
    }
    ui_elements.push_back(text(footer_text) | center | dim | color(Color::Green));
    
    return vbox(ui_elements) | size(WIDTH, EQUAL, Terminal::Size().dimx) | size(HEIGHT, EQUAL, Terminal::Size().dimy);
  });
  
  auto screen = ScreenInteractive::Fullscreen();
  screen.Loop(renderer);
  
  return 0;
}
