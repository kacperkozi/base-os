#include "app/navigation.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

namespace app {

using namespace ftxui;

// Theme colors for navigation (consistent with views.cpp)
namespace nav_theme {
  const auto primary = Color::Green;
}

NavigationBar::NavigationBar(const NavigationConfig& config) : config_(config) {
  CreateButtons();
}

void NavigationBar::CreateButtons() {
  // Create buttons based on configuration
  std::vector<Component> buttons;
  
  if (config_.show_back && config_.back_button.visible) {
    auto btn = Button(config_.back_button.label, [this] {
      if (config_.back_button.enabled && config_.back_button.action) {
        config_.back_button.action();
      }
    });
    
    // Wrap button to also respond to Enter key (Space already works)
    back_btn_ = CatchEvent(btn, [this](Event event) {
      if (event == Event::Return) {
        if (config_.back_button.enabled && config_.back_button.action) {
          config_.back_button.action();
          return true;
        }
      }
      return false;
    });
    
    buttons.push_back(back_btn_);
  }
  
  if (config_.show_continue && config_.continue_button.visible) {
    auto btn = Button(config_.continue_button.label, [this] {
      if (config_.continue_button.enabled && config_.continue_button.action) {
        config_.continue_button.action();
      }
    });
    
    // Wrap button to also respond to Enter key (Space already works)
    continue_btn_ = CatchEvent(btn, [this](Event event) {
      if (event == Event::Return) {
        if (config_.continue_button.enabled && config_.continue_button.action) {
          config_.continue_button.action();
          return true;
        }
      }
      return false;
    });
    
    buttons.push_back(continue_btn_);
  }
  
  if (config_.show_skip && config_.skip_button.visible) {
    skip_btn_ = Button(config_.skip_button.label, [this] {
      if (config_.skip_button.enabled && config_.skip_button.action) {
        config_.skip_button.action();
      }
    });
    buttons.push_back(skip_btn_);
  }
  
  if (!buttons.empty()) {
    container_ = Container::Horizontal(buttons);
    Add(container_);
  }
}

Element NavigationBar::OnRender() {
  Elements elements;
  
  // Navigation buttons
  if (container_) {
    Elements button_elements;
    
    if (config_.show_back && config_.back_button.visible && back_btn_) {
      auto btn_element = back_btn_->Render();
      if (!config_.back_button.enabled) {
        btn_element = btn_element | dim;
      }
      button_elements.push_back(btn_element);
    }
    
    if (config_.show_continue && config_.continue_button.visible && continue_btn_) {
      if (!button_elements.empty()) {
        button_elements.push_back(text("  ")); // Spacer
      }
      auto btn_element = continue_btn_->Render();
      if (!config_.continue_button.enabled) {
        btn_element = btn_element | dim;
      } else {
        btn_element = btn_element | color(nav_theme::primary);
      }
      button_elements.push_back(btn_element);
    }
    
    if (config_.show_skip && config_.skip_button.visible && skip_btn_) {
      if (!button_elements.empty()) {
        button_elements.push_back(text("  ")); // Spacer
      }
      auto btn_element = skip_btn_->Render();
      if (!config_.skip_button.enabled) {
        btn_element = btn_element | dim;
      }
      button_elements.push_back(btn_element);
    }
    
    if (!button_elements.empty()) {
      elements.push_back(hbox(button_elements) | center);
    }
  }
  
  // Help text
  if (config_.show_help_text) {
    elements.push_back(text(""));
    elements.push_back(
      text("Tab/Arrows: navigate buttons • Enter: activate • Escape: back • e: edit mode") 
      | center | dim
    );
  }
  
  return vbox(elements);
}

bool NavigationBar::OnEvent(Event event) {
  // Let the container handle button events first
  if (container_ && container_->OnEvent(event)) {
    return true;
  }
  
  // Handle keyboard shortcuts
  if (event == Event::Escape && config_.show_back && config_.back_button.enabled) {
    if (config_.back_button.action) {
      config_.back_button.action();
      return true;
    }
  }
  
  if (event == Event::Return && config_.show_continue && config_.continue_button.enabled) {
    if (config_.continue_button.action) {
      config_.continue_button.action();
      return true;
    }
  }
  
  return false;
}

void NavigationBar::UpdateConfig(const NavigationConfig& config) {
  config_ = config;
  CreateButtons();
}

// Factory function
Component MakeNavigationBar(const NavigationConfig& config) {
  return std::make_shared<NavigationBar>(config);
}

// Navigation configurations for different routes
NavigationConfig NavigationFactory::ForConnectWallet(AppState& state) {
  NavigationConfig config;
  
  config.back_button = {
    "Exit",
    [&state] { state.requestShutdown(); },
    true,
    true
  };
  
  config.continue_button = {
    "Continue",
    [&state] { 
      auto device_state = state.getDeviceState();
      if (device_state.wallet_connected) {
        state.setRoute(Route::USBContacts);
      }
    },
    false, // Will be enabled when wallet is connected
    true
  };
  
  config.skip_button = {
    "Skip to USB",
    [&state] { state.setRoute(Route::USBContacts); },
    true,
    true
  };
  
  config.show_back = true;
  config.show_continue = true;
  config.show_skip = true;
  
  // Update continue button state based on wallet connection
  auto device_state = state.getDeviceState();
  config.continue_button.enabled = device_state.wallet_connected;
  
  return config;
}

NavigationConfig NavigationFactory::ForUSBContacts(AppState& state) {
  NavigationConfig config;
  
  config.back_button = {
    "Back",
    [&state] { state.setRoute(Route::ConnectWallet); },
    true,
    true
  };
  
  config.continue_button = {
    "Continue",
    [&state] { state.setRoute(Route::TransactionInput); },
    true,
    true
  };
  
  config.skip_button = {
    "Skip",
    [&state] { state.setRoute(Route::TransactionInput); },
    true,
    true
  };
  
  config.show_back = true;
  config.show_continue = true;
  config.show_skip = true;
  
  return config;
}

NavigationConfig NavigationFactory::ForTransactionInput(AppState& state) {
  NavigationConfig config;
  
  config.back_button = {
    "Back",
    [&state] { state.setRoute(Route::USBContacts); },
    true,
    true
  };
  
  config.continue_button = {
    "Continue",
    [&state] { 
      // Validate transaction before continuing
      auto tx = state.getUnsignedTx();
      if (!tx.to.empty() && !tx.value.empty()) {
        state.setRoute(Route::Confirmation);
      } else {
        state.setError("Please fill in required transaction fields");
      }
    },
    false, // Will be enabled based on form validation
    true
  };
  
  config.skip_button = {
    "Skip",
    [&state] { state.setRoute(Route::Confirmation); },
    true,
    true
  };
  
  config.show_back = true;
  config.show_continue = true;
  config.show_skip = true;
  
  // Update continue button state based on transaction data
  auto tx = state.getUnsignedTx();
  config.continue_button.enabled = !tx.to.empty() && !tx.value.empty();
  
  return config;
}

NavigationConfig NavigationFactory::ForConfirmation(AppState& state) {
  NavigationConfig config;
  
  config.back_button = {
    "Back to Edit",
    [&state] { state.setRoute(Route::TransactionInput); },
    true,
    true
  };
  
  config.continue_button = {
    "Confirm & Sign",
    [&state] { state.setRoute(Route::Signing); },
    true,
    true
  };
  
  config.show_back = true;
  config.show_continue = true;
  config.show_skip = false;
  
  return config;
}

NavigationConfig NavigationFactory::ForSigning(AppState& state) {
  NavigationConfig config;
  
  config.back_button = {
    "Back",
    [&state] { state.setRoute(Route::Confirmation); },
    true,
    true
  };
  
  config.continue_button = {
    "View Result",
    [&state] { 
      if (state.hasSignedTx()) {
        state.setRoute(Route::Result);
      }
    },
    false, // Only enabled when transaction is signed
    true
  };
  
  config.show_back = true;
  config.show_continue = true;
  config.show_skip = false;
  
  // Update continue button based on signing state
  config.continue_button.enabled = state.hasSignedTx();
  
  return config;
}

NavigationConfig NavigationFactory::ForResult(AppState& state) {
  NavigationConfig config;
  
  config.back_button = {
    "New Transaction",
    [&state] { 
      state.clearTransaction();
      state.setRoute(Route::TransactionInput);
    },
    true,
    true
  };
  
  config.continue_button = {
    "Exit",
    [&state] { state.requestShutdown(); },
    true,
    true
  };
  
  config.show_back = true;
  config.show_continue = true;
  config.show_skip = false;
  
  return config;
}

NavigationConfig NavigationFactory::ForSettings(AppState& state) {
  NavigationConfig config;
  
  config.back_button = {
    "Back",
    [&state] { 
      auto ui_state = state.getUIState();
      state.setRoute(ui_state.previous_route);
    },
    true,
    true
  };
  
  config.continue_button = {
    "Save & Exit",
    [&state] { 
      auto ui_state = state.getUIState();
      state.setEditMode(false);
      state.setRoute(ui_state.previous_route);
    },
    true,
    true
  };
  
  config.show_back = true;
  config.show_continue = true;
  config.show_skip = false;
  
  return config;
}

NavigationConfig NavigationFactory::ForHelp(AppState& state) {
  NavigationConfig config;
  
  config.back_button = {
    "Back",
    [&state] { 
      auto ui_state = state.getUIState();
      state.setRoute(ui_state.previous_route);
    },
    true,
    true
  };
  
  config.continue_button = {
    "Close Help",
    [&state] { 
      auto ui_state = state.getUIState();
      state.setRoute(ui_state.previous_route);
    },
    true,
    true
  };
  
  config.show_back = true;
  config.show_continue = true;
  config.show_skip = false;
  
  return config;
}

NavigationConfig NavigationFactory::ForRoute(Route route, AppState& state) {
  switch (route) {
    case Route::ConnectWallet:
      return ForConnectWallet(state);
    case Route::USBContacts:
      return ForUSBContacts(state);
    case Route::TransactionInput:
      return ForTransactionInput(state);
    case Route::Confirmation:
      return ForConfirmation(state);
    case Route::Signing:
      return ForSigning(state);
    case Route::Result:
      return ForResult(state);
    case Route::Settings:
      return ForSettings(state);
    case Route::Help:
      return ForHelp(state);
    default:
      // Default configuration
      NavigationConfig config;
      config.back_button = {"Back", [&state] { 
        auto ui_state = state.getUIState();
        state.setRoute(ui_state.previous_route);
      }, true, true};
      config.continue_button = {"Continue", nullptr, false, false};
      config.show_back = true;
      config.show_continue = false;
      config.show_skip = false;
      return config;
  }
}

Component NavigationFactory::CreateNavigationBar(const NavigationConfig& config) {
  return MakeNavigationBar(config);
}

} // namespace app
