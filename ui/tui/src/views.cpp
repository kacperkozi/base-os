#include "app/views.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <thread>
#include <chrono>
#include <memory>
#include "app/wallet_detector.hpp"
#include "app/qr_generator.hpp"
#include "app/qr_viewer.hpp"
#include "app/validation.hpp"
#include "app/navigation.hpp"

namespace app {

using namespace ftxui;

// FTXUI Color Theme for consistent styling
namespace theme {
  const auto secondary = Color::Blue;
  const auto accent = Color::Yellow;
  const auto success = Color::Green;
  const auto warning = Color::Yellow;
  const auto error = Color::Red;
  const auto info = Color::Cyan;
  
  // Semantic colors for different states
  const auto wallet_connected = success;
  const auto edit_mode = success;
  const auto navigation_mode = secondary;
}

Component ConnectWalletView(AppState& state) {
  // Helper function for wallet detection (shared with global 'd' key)
  auto triggerWalletDetection = [&state]() {
    auto device_state = state.getDeviceState();
    auto ui_state = state.getUIState();
    if (!ui_state.is_detecting_wallet && !device_state.wallet_connected) {
      state.setDetectingWallet(true);
      // Use async task with PostEvent for better integration
      auto detection_task = [&state]() {
        for (int i = 0; i < 20; i++) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          state.incrementAnimationFrame();
        }
        state.setDetectingWallet(false);
        state.setWalletConnected(true);
        state.setStatus("Wallet connected, navigating...");
        // Automatically navigate on success
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        state.setRoute(Route::USBContacts);
      };
      std::thread(detection_task).detach();
    }
  };

  // Create interactive buttons
  auto detect_button = Button("Detect Wallet", triggerWalletDetection);
  
  // Create navigation bar
  auto nav_config = NavigationFactory::ForConnectWallet(state);
  auto navigation_bar = MakeNavigationBar(nav_config);
  
  // Container for detect button and navigation
  auto button_container = Container::Horizontal({detect_button});
  auto main_container = Container::Vertical({button_container, navigation_bar});

  return Renderer(main_container, [&state, detect_button, button_container, navigation_bar] {
    auto ui_state = state.getUIState();
    auto device_state = state.getDeviceState();

    Elements elements;
    elements.push_back(text("Connect Hardware Wallet") | bold | center);
    elements.push_back(separator());

    if (ui_state.dev_mode) {
        elements.push_back(text("DEV MODE ENABLED") | color(theme::warning) | center);
        elements.push_back(text("Mock Wallet Connected") | color(theme::wallet_connected) | center);
        elements.push_back(text(""));
    } else if (ui_state.is_detecting_wallet) {
      elements.push_back(text("Detecting wallet...") | center);
      // Use FTXUI's built-in spinner with animation frame
      elements.push_back(spinner(21, ui_state.animation_frame) | center);
    } else if (device_state.wallet_connected) {
      elements.push_back(text("Wallet Connected") | color(theme::wallet_connected) | center);
      elements.push_back(text(""));
      elements.push_back(text("Ready to continue to next step") | center);
    } else {
      elements.push_back(text("Please connect your hardware wallet") | center);
      elements.push_back(text(""));
      elements.push_back(button_container->Render() | center);
      elements.push_back(text(""));
      elements.push_back(text("Press 'd' to detect wallet") | dim | center);
    }
    
    // Add navigation bar
    elements.push_back(text(""));
    elements.push_back(separator());
    elements.push_back(navigation_bar->Render());
    
    return vbox(elements) | border;
  });
}

Component USBContactsView(AppState& state) {
  // Create Menu entries for contacts (will be populated dynamically)
  auto contact_entries = std::make_shared<std::vector<std::string>>();
  auto selected_contact = std::make_shared<int>(0);
  auto contact_menu = Menu(contact_entries.get(), selected_contact.get());
  
  // Container for the menu and scan button handling
  auto main_container = Container::Vertical({contact_menu});

  return Renderer(main_container, [&state, contact_entries, selected_contact, contact_menu, main_container] {
    auto ui_state = state.getUIState();
    auto device_state = state.getDeviceState();

    Elements elements;
    elements.push_back(text("USB Contacts") | bold | center);
    elements.push_back(separator());
    
    // Handle scanning state with FTXUI spinner
    if (ui_state.is_scanning_usb) {
      elements.push_back(text("Scanning USB for contacts...") | center);
      // Use different spinner style for variety
      elements.push_back(spinner(17, ui_state.animation_frame) | center);
      return vbox(elements) | border;
    }
    
    // Handle empty state (unchanged)
    if (device_state.usb_contacts.empty()) {
      elements.push_back(text("No contacts found") | dim | center);
      elements.push_back(text("Press 's' to scan USB") | dim | center);
      return vbox(elements) | border;
    }
    
    // Handle contacts state - use FTXUI Menu
    elements.push_back(text("Found " + std::to_string(device_state.usb_contacts.size()) + " contacts:"));
    
    // Update Menu entries if contacts changed
    contact_entries->clear();
    for (const auto& contact : device_state.usb_contacts) {
      contact_entries->push_back(contact.name + " - " + contact.address.substr(0, 10) + "...");
    }
    
    // Sync Menu selection with AppState
    if (*selected_contact != ui_state.selected_contact && 
        ui_state.selected_contact >= 0 && 
        ui_state.selected_contact < (int)contact_entries->size()) {
      *selected_contact = ui_state.selected_contact;
    }
    
    // Update AppState when Menu selection changes
    if (*selected_contact != ui_state.selected_contact) {
      state.setSelectedContact(*selected_contact);
    }
    
    elements.push_back(contact_menu->Render());
    elements.push_back(text(""));
    elements.push_back(text("Press 's' to scan USB") | dim | center);
    
    return vbox(elements) | border;
  }) | CatchEvent([&state](Event event) {
    // Handle 's' key for USB scanning
    if (event.is_character() && event.character() == "s") {
      state.setScanningUsb(true);
      return true;
    }
    return false;
  });
}

Component TransactionInputView(AppState& state) {
  struct TransactionData {
    std::string to;
    std::string value;
    std::string gas_limit;
    bool initialized = false;
    bool focus_set = false;  // Track if we've set initial focus
  };
  auto temp_tx_data = std::make_shared<TransactionData>();

  // Create inputs with FTXUI validation decorators
  Component to_input = Input(&temp_tx_data->to, "To address...") | CatchEvent([temp_tx_data, &state](Event event) {
    if (event == Event::Character(' ') || event == Event::Tab) {
      // Validate address input in real-time
      auto result = InputValidators::validateAddressInput(temp_tx_data->to);
      if (!result.is_valid && !temp_tx_data->to.empty()) {
        state.setError("Address: " + result.error_message);
      } else {
        state.clearError();
      }
      return false; // Let the input handle the event
    }
    return false;
  });
  
  Component value_input = Input(&temp_tx_data->value, "Amount in Wei...") | CatchEvent([temp_tx_data, &state](Event event) {
    if (event == Event::Character(' ') || event == Event::Tab) {
      // Validate amount input in real-time
      auto result = InputValidators::validateAmountInput(temp_tx_data->value, true);
      if (!result.is_valid && !temp_tx_data->value.empty()) {
        state.setError("Amount: " + result.error_message);
      } else {
        state.clearError();
        if (!result.suggestion.empty()) {
          state.setInfo(result.suggestion);
        }
      }
      return false;
    }
    return false;
  });
  
  Component gas_input = Input(&temp_tx_data->gas_limit, "Gas limit...") | CatchEvent([temp_tx_data, &state](Event event) {
    if (event == Event::Character(' ') || event == Event::Tab) {
      // Validate gas input in real-time
      auto result = InputValidators::validateGasInput(temp_tx_data->gas_limit, "Gas Limit");
      if (!result.is_valid && !temp_tx_data->gas_limit.empty()) {
        state.setError("Gas: " + result.error_message);
      } else {
        state.clearError();
      }
      return false;
    }
    return false;
  });
  // Save transaction data function
  auto saveTransactionData = [&state, temp_tx_data]() {
    UnsignedTx new_tx = state.getUnsignedTx();
    new_tx.to = temp_tx_data->to;
    new_tx.value = temp_tx_data->value;
    new_tx.gas_limit = temp_tx_data->gas_limit;
    state.setUnsignedTx(new_tx);
  };

  auto input_container = Container::Vertical({
      to_input,
      value_input,
      gas_input,
  });
  
  // Create navigation bar
  auto nav_config = NavigationFactory::ForTransactionInput(state);
  auto navigation_bar = MakeNavigationBar(nav_config);
  
  // Main container with inputs and navigation
  auto main_container = Container::Vertical({
      input_container,
      navigation_bar,
  });

  // Enhanced paste functionality with FTXUI event handling
  auto paste_handler = CatchEvent(main_container, [&state, temp_tx_data](Event event) {
    // Handle common paste shortcuts
    if (event == Event::CtrlV) {
      // FTXUI Input components handle paste automatically
      // We just need to trigger validation after paste
      state.setInfo("Paste detected - validating input...");
      return false; // Let input handle the actual paste
    }
    
    // Handle other terminal paste events
    if (event == Event::Insert || 
        (event.is_character() && (event.character() == "\x16" || event.character() == "\x1b[200~"))) {
      state.setInfo("Data pasted - please verify the input");
      return false;
    }
    
    return false; // Don't consume other events
  });

  return Renderer(paste_handler, [&, temp_tx_data, input_container, to_input, navigation_bar] {
    auto ui_state = state.getUIState();
    
    // CRITICAL FIX: Only initialize data ONCE, never overwrite user input
    if (!temp_tx_data->initialized) {
      auto tx = state.getUnsignedTx();
      temp_tx_data->to = tx.to;
      temp_tx_data->value = tx.value;
      temp_tx_data->gas_limit = tx.gas_limit.empty() ? "21000" : tx.gas_limit;
      temp_tx_data->initialized = true;
    }

    // UX Enhancement: Auto-focus the first input field when entering edit mode
    if (ui_state.edit_mode && !temp_tx_data->focus_set) {
      input_container->SetActiveChild(to_input.get());
      temp_tx_data->focus_set = true;
    } else if (!ui_state.edit_mode) {
      // Reset focus state when exiting edit mode
      temp_tx_data->focus_set = false;
    }

    // Always save transaction data when rendering
    saveTransactionData();
    
    if (ui_state.edit_mode) {
      // In edit mode: let the container handle event delegation and rendering
      return vbox({
        text("Transaction Input") | bold | center,
        separator(),
        text("EDIT MODE") | color(theme::edit_mode) | center,
        text("Focus on input fields to enter data. Numbers work normally in edit mode.") | dim | center,
        separator(),
        // ✅ CORRECT: Let container render its components with proper event flow
        input_container->Render() | flex,
        separator(),
        navigation_bar->Render(),
      }) | border;
    } else {
      auto tx = state.getUnsignedTx();
      return vbox({
        text("Transaction Input") | bold | center,
        separator(),
        text("NAVIGATION MODE") | color(theme::navigation_mode) | center,
        text("e: enter edit mode | 1-5: navigate screens | q: quit") | dim | center,
        separator(),
        text("Current Values:") | center,
        text(""),
        // Use FTXUI's gridbox for better field layout
        gridbox({
          {text("To Address:"), text(tx.to.empty() ? "[Not Set]" : tx.to) | dim},
          {text("Amount:"), text(tx.value.empty() ? "[Not Set]" : tx.value + " ETH") | dim},
          {text("Gas Limit:"), text(tx.gas_limit.empty() ? "21000" : tx.gas_limit) | dim}
        }),
        text(""),
        separator(),
        navigation_bar->Render(),
      }) | border;
    }
  });
}

Component ConfirmationView(AppState& state) {
  // Create navigation bar
  auto nav_config = NavigationFactory::ForConfirmation(state);
  auto navigation_bar = MakeNavigationBar(nav_config);
  
  // Create a container that includes the navigation bar
  auto container = Container::Vertical({navigation_bar});

  return Renderer(container, [&state, navigation_bar] {
    auto tx = state.getUnsignedTx();

    return vbox({
        text("Review Transaction") | bold | center,
        separator(),
        text(""),
        // Use gridbox for consistent confirmation layout
        gridbox({
          {text("To:"), text(tx.to.empty() ? "[Not Set]" : tx.to)},
          {text("Value:"), text(tx.value.empty() ? "[Not Set]" : tx.value + " ETH")},
          {text("Gas Limit:"), text(tx.gas_limit.empty() ? "21000" : tx.gas_limit)}
        }),
        text(""),
        separator(),
        navigation_bar->Render(),
    }) | border;
  });
}

Component SigningView(AppState& state) {
  // Create signing content
  auto signing_content = Renderer([&state] {
    auto ui_state = state.getUIState();
    Elements elements;
    elements.push_back(text("Signing Transaction") | bold | center);
    elements.push_back(separator());
    
    if (ui_state.is_signing) {
      elements.push_back(text("Please confirm on your hardware wallet") | center);
      // Use a more appropriate spinner for signing process
      elements.push_back(spinner(6, ui_state.animation_frame) | center);
    } else if (state.hasSignedTx()) {
      elements.push_back(text("Transaction Signed") | color(theme::success) | center);
      elements.push_back(text("Press Enter to view result") | dim | center);
    } else {
      elements.push_back(text("Signing failed") | color(theme::error) | center);
      elements.push_back(text(ui_state.error) | dim | center);
    }
    
    return vbox(elements) | border;
  });
  
  // Return the signing content directly (Modal is a decorator, not a component)
  return signing_content;
}

Component ResultView(AppState& state) {
  // This struct holds the state for this view instance, allowing us to track
  // what data the QR viewer is currently displaying.
  struct ViewState {
    std::string current_signed_hex;
  };
  auto view_state = std::make_shared<ViewState>();

  // Create the viewer component once.
  auto qr_viewer = MakeQrViewer({});
  auto main_container = Container::Vertical({qr_viewer});
  
  // Ensure the container is focused to receive navigation events
  main_container->TakeFocus();

  // The main renderer for the ResultView.
  auto component = Renderer(main_container, [&, view_state, qr_viewer] {
    auto tx_state = state.getTransactionState();
    auto ui_state = state.getUIState();

    // Only update the QR codes in the viewer if the signed transaction has changed.
    // This is the critical fix to prevent the viewer's state (like the current index)
    // from being reset on every single frame.
    if (tx_state.signed_hex != view_state->current_signed_hex) {
      view_state->current_signed_hex = tx_state.signed_hex;
      auto viewer_ptr = std::static_pointer_cast<QrViewer>(qr_viewer);
      viewer_ptr->SetQRCodes(tx_state.qr_codes);
      // Set ASCII preference based on dev mode
      viewer_ptr->SetPreferASCII(ui_state.dev_mode);
    }

    // Display an error if there's no transaction.
    if (!state.hasSignedTx()) {
        return vbox({
            text("No signed transaction available") | color(theme::error) | center,
            separator(),
            text("Press Escape to go back") | dim | center
        }) | border;
    }

    // Render the header and the QR viewer.
    auto header = vbox({
        text("Transaction Signed Successfully") | bold | color(theme::success) | center,
        separator(),
        text("Signed Transaction:") | bold,
        text(tx_state.signed_hex.substr(0, 66) + "...") | dim
    });

    return vbox({
        header,
        qr_viewer->Render() | flex,
    }) | border;
  });

  return component;
}

Component SettingsView(AppState& state) {
  // Component state must be stored outside the renderer lambda.
  struct ViewState {
    bool use_eip1559;
    bool show_wei;
    bool initialized = false;
  };
  auto view_state = std::make_shared<ViewState>();

  // Create the interactive form components
  auto settings_form = Container::Vertical({
      Checkbox("Use EIP-1559 transactions", &view_state->use_eip1559),
      Checkbox("Show amounts in Wei", &view_state->show_wei),
      Button("Save Settings", [&state, view_state] {
        state.setUseEIP1559(view_state->use_eip1559);
        state.setShowWei(view_state->show_wei);
        state.setStatus("Settings saved successfully!");
        state.setEditMode(false);
      }),
  });

  // Use the same pattern as successful views: Renderer(component, lambda)
  // In the lambda we manually call ->Render() on the component parts
  auto main_container = Container::Vertical({settings_form});

  return Renderer(main_container, [&state, view_state, settings_form] {
    auto app_ui_state = state.getUIState();
    auto app_tx_state = state.getTransactionState();
    
    // CRITICAL FIX: Only initialize data ONCE, never overwrite user selections
    if (!view_state->initialized) {
      view_state->use_eip1559 = app_tx_state.use_eip1559;
      view_state->show_wei = app_ui_state.show_wei;
      view_state->initialized = true;
    }

    // Render either the interactive form or a read-only view
    if (app_ui_state.edit_mode) {
      // In edit mode: show the interactive form with headers
      return vbox({
          text("Settings") | bold | center,
          separator(),
          text("SETTINGS EDIT MODE") | color(theme::edit_mode) | center,
          text("Tab/Arrows: move | Space: toggle | e: exit") | dim | center,
          separator(),
          // Call ->Render() on the form component (this is the correct pattern)
          settings_form->Render(),
      }) | border;
    } else {
      // In navigation mode: show read-only view
      return vbox({
          text("Settings") | bold | center,
          separator(),
          text("NAVIGATION MODE") | color(theme::navigation_mode) | center,
          text("e: enter edit mode | 1-5: navigate | q: quit") | dim | center,
          separator(),
          text("Current Settings:") | center,
          text(""),
          // Use gridbox for settings display
          gridbox({
            {text("Transaction Type:"), text(app_tx_state.use_eip1559 ? "EIP-1559" : "Legacy") | color(app_tx_state.use_eip1559 ? theme::success : theme::warning)},
            {text("Amount Display:"), text(app_ui_state.show_wei ? "Wei" : "ETH") | dim}
          }),
          text(""),
          separator(),
          text(app_ui_state.status) | center | color(theme::success),
          text("Press 'e' to modify settings") | color(theme::accent) | center,
      }) | border;
    }
  });
}

Component HelpView(AppState& state) {
  // Help section topics
  auto help_topics = std::make_shared<std::vector<std::string>>();
  help_topics->push_back("Navigation Controls");
  help_topics->push_back("Edit Mode Controls");
  help_topics->push_back("Tips & Tricks");
  help_topics->push_back("Quick Actions");
  
  auto selected_topic = std::make_shared<int>(0);
  auto help_menu = Menu(help_topics.get(), selected_topic.get());
  
  // Quick action buttons
  auto go_to_transaction = Button("Go to Transaction Input", [&state]() {
    state.setRoute(Route::TransactionInput);
  });
  auto go_to_settings = Button("Go to Settings", [&state]() {
    state.setRoute(Route::Settings);
  });
  
  // Use FTXUI's Tab container for better focus management
  auto menu_container = Container::Vertical({help_menu});
  auto quick_actions_container = Container::Vertical({go_to_transaction, go_to_settings});
  
  // Use Tab container for proper focus management
  auto main_container = Container::Tab({
    menu_container,
    quick_actions_container
  }, selected_topic.get());

  return Renderer(main_container, [help_topics, selected_topic, help_menu, menu_container, quick_actions_container] {
    // Tab container automatically manages focus based on selected_topic
    Elements left_panel;
    left_panel.push_back(text("Help Topics") | bold | center);
    left_panel.push_back(separator());
    left_panel.push_back(help_menu->Render());
    
    Elements right_panel;
    right_panel.push_back(text("Base OS TUI - Help") | bold | center);
    right_panel.push_back(separator());
    
    // Display content based on selected topic
    switch (*selected_topic) {
      case 0: // Navigation Controls
        right_panel.push_back(text("NAVIGATION CONTROLS") | bold | color(theme::navigation_mode));
        right_panel.push_back(text(""));
        right_panel.push_back(text("  1: Connect Wallet screen"));
        right_panel.push_back(text("  2: USB Contacts screen"));
        right_panel.push_back(text("  3: Transaction Input screen"));
        right_panel.push_back(text("  4: Confirmation screen"));
        right_panel.push_back(text("  5: Result/QR Code screen"));
        right_panel.push_back(text("  h: This Help screen"));
        right_panel.push_back(text("  s: Settings screen"));
        right_panel.push_back(text("  q: Quit application"));
        right_panel.push_back(text("  Escape: Go back to previous screen"));
        break;
        
      case 1: // Edit Mode Controls
        right_panel.push_back(text("EDIT MODE CONTROLS") | bold | color(theme::edit_mode));
        right_panel.push_back(text(""));
        right_panel.push_back(text("  e: Toggle between Navigation and Edit modes"));
        right_panel.push_back(text("  Tab / Arrow Keys: Move between input fields/buttons"));
        right_panel.push_back(text("  Space: Toggle checkboxes"));
        right_panel.push_back(text("  Enter: Press buttons"));
        right_panel.push_back(text("  4/5: Preview screens even while editing"));
        break;
        
      case 2: // Tips & Tricks
        right_panel.push_back(text("TIPS & TRICKS") | bold | color(theme::accent));
        right_panel.push_back(text(""));
        right_panel.push_back(text("  On any screen with a form, press 'e' to start editing."));
        right_panel.push_back(text("  The focused item will be highlighted in yellow."));
        right_panel.push_back(text("  You can preview (4/5) with incomplete data."));
        right_panel.push_back(text("  All values persist when navigating between screens."));
        right_panel.push_back(text("  Use arrow keys or h/l to navigate QR codes."));
        right_panel.push_back(text("  In dev mode, wallet detection is automatic."));
        break;
        
      case 3: // Quick Actions
        right_panel.push_back(text("QUICK ACTIONS") | bold | color(theme::info));
        right_panel.push_back(text(""));
        right_panel.push_back(text("Use Tab/Enter to select buttons below:"));
        right_panel.push_back(text(""));
        // Show the actual interactive buttons via the quick_actions_container
        right_panel.push_back(quick_actions_container->Render() | center);
        right_panel.push_back(text(""));
        right_panel.push_back(text("Or use number keys (1-5) from any screen."));
        break;
    }
    
    return hbox({
      vbox(left_panel) | flex_shrink | size(WIDTH, EQUAL, 25),
      separator(),
      vbox(right_panel) | flex
    }) | border;
  });
}

Component ErrorView(AppState& state) {
  // Use FTXUI's Modal system for better error display
  auto error_content = Renderer([&state] {
    return vbox({
      text("Error") | bold | color(theme::error) | center,
      separator(),
      text(state.getUIState().error) | center,
      separator(),
      text("Press Escape to go back") | dim | center
    }) | border | center;
  });
  
  // Return the error content directly
  return error_content;
}

} // namespace app
