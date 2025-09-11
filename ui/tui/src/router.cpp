#include "app/router.hpp"
#include "app/views.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

namespace app {

using namespace ftxui;

Router::Router(AppState& state) : state_(state) {
  InitializeViews();
}

void Router::InitializeViews() {
  // Initialize all view components
  connect_wallet_view_ = ConnectWalletView(state_);
  usb_contacts_view_ = USBContactsView(state_);
  transaction_input_view_ = TransactionInputView(state_);
  confirmation_view_ = ConfirmationView(state_);
  signing_view_ = SigningView(state_);
  result_view_ = ResultView(state_);
  settings_view_ = SettingsView(state_);
  help_view_ = HelpView(state_);
  error_view_ = ErrorView(state_);
}

Component Router::GetCurrentView() {
  switch(state_.getRoute()) {
    case Route::ConnectWallet:
      return connect_wallet_view_;
    case Route::USBContacts:
      return usb_contacts_view_;
    case Route::TransactionInput:
      return transaction_input_view_;
    case Route::Confirmation:
      return confirmation_view_;
    case Route::Signing:
      return signing_view_;
    case Route::Result:
      return result_view_;
    case Route::Settings:
      return settings_view_;
    case Route::Help:
      return help_view_;
    case Route::Error:
      return error_view_;
    default:
      return connect_wallet_view_;
  }
}

void Router::NavigateTo(Route route) {
  state_.setRoute(route);
}

void Router::NavigateBack() {
  auto ui_state = state_.getUIState();
  NavigateTo(ui_state.previous_route);
}

void Router::NavigateNext() {
  // Define the navigation flow
  switch(state_.getRoute()) {
    case Route::ConnectWallet:
      NavigateTo(Route::USBContacts);
      break;
    case Route::USBContacts:
      NavigateTo(Route::TransactionInput);
      break;
    case Route::TransactionInput:
      NavigateTo(Route::Confirmation);
      break;
    case Route::Confirmation:
      NavigateTo(Route::Signing);
      break;
    case Route::Signing:
      NavigateTo(Route::Result);
      break;
    default:
      break;
  }
}

Element Router::OnRender() {
  auto current_view = GetCurrentView();
  
  // Use flexbox for better header layout
  auto header = vbox({
    text("╔══════════════════════════════════════════════════════════════╗") | center,
    text("║                    BASE OS TUI v1.0                          ║") | center | bold | color(Color::Green),
    text("╚══════════════════════════════════════════════════════════════╝") | center,
  });
  
  // Current route indicator with flexbox
  auto route_indicator = flexbox({
    text("Current: "),
    text([this] {
      switch(state_.getRoute()) {
        case Route::ConnectWallet: return "Connect Wallet";
        case Route::USBContacts: return "USB Contacts";
        case Route::TransactionInput: return "Transaction Input";
        case Route::Confirmation: return "Confirmation";
        case Route::Signing: return "Signing";
        case Route::Result: return "Result";
        case Route::Settings: return "Settings";
        case Route::Help: return "Help";
        case Route::Error: return "Error";
        default: return "Unknown";
      }
    }()) | bold
  }) | center;
  
  return vbox({
    header,
    route_indicator,
    
    separator(),
    
    // Main content from current view
    current_view->Render() | flex,
    
    separator(),
    
    // Footer with flexbox layout for better control display
    flexbox({
      text("Controls: "),
      text("1-5 navigate") | color(Color::Yellow),
      text(" • "),
      text("h help") | color(Color::Cyan),
      text(" • "),
      text("s settings") | color(Color::Magenta),
      text(" • "),
      text("e edit") | color(Color::Green),
      text(" • "),
      text("q quit") | color(Color::Red)
    }) | center | dim
  });
}

bool Router::OnEvent(Event event) {
  // Delegate all events to the current view component
  auto current_view = GetCurrentView();
  return current_view->OnEvent(event);
}

// Factory function
Component MakeRouter(AppState& state) {
  return std::make_shared<Router>(state);
}

} // namespace app