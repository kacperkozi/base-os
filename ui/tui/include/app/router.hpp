#pragma once
#include <ftxui/component/component.hpp>
#include "state.hpp"

namespace app {

// Central UI Router that manages navigation between views
class Router : public ftxui::ComponentBase {
public:
  explicit Router(AppState& state);
  
  // Returns the current view component based on the route
  ftxui::Component GetCurrentView();
  
  // Navigation methods
  void NavigateTo(Route route);
  void NavigateBack();
  void NavigateNext();
  
  // ComponentBase overrides
  ftxui::Element OnRender() override;
  bool OnEvent(ftxui::Event event) override;
  
private:
  AppState& state_;
  
  // Cached view components
  ftxui::Component connect_wallet_view_;
  ftxui::Component usb_contacts_view_;
  ftxui::Component transaction_input_view_;
  ftxui::Component confirmation_view_;
  ftxui::Component signing_view_;
  ftxui::Component result_view_;
  ftxui::Component settings_view_;
  ftxui::Component help_view_;
  ftxui::Component error_view_;
  
  // Initialize all view components
  void InitializeViews();
};

// Factory function for creating the router component
ftxui::Component MakeRouter(AppState& state);

} // namespace app