#pragma once
#include <ftxui/component/component.hpp>
#include <functional>
#include "state.hpp"

namespace app {

// Navigation button configuration
struct NavigationButton {
  std::string label;
  std::function<void()> action;
  bool enabled = true;
  bool visible = true;
};

// Navigation bar configuration
struct NavigationConfig {
  NavigationButton back_button;
  NavigationButton continue_button;
  NavigationButton skip_button;
  bool show_back = true;
  bool show_continue = true;
  bool show_skip = false;
  bool show_help_text = true;
};

// Factory functions for common navigation patterns
class NavigationFactory {
public:
  // Create a standard navigation bar with Continue/Back/Skip buttons
  static ftxui::Component CreateNavigationBar(const NavigationConfig& config);
  
  // Create navigation for specific routes
  static NavigationConfig ForConnectWallet(AppState& state);
  static NavigationConfig ForUSBContacts(AppState& state);
  static NavigationConfig ForTransactionInput(AppState& state);
  static NavigationConfig ForConfirmation(AppState& state);
  static NavigationConfig ForSigning(AppState& state);
  static NavigationConfig ForResult(AppState& state);
  static NavigationConfig ForSettings(AppState& state);
  static NavigationConfig ForHelp(AppState& state);
  
  // Helper to create navigation for any route
  static NavigationConfig ForRoute(Route route, AppState& state);
};

// Navigation component that handles standard flow
class NavigationBar : public ftxui::ComponentBase {
public:
  explicit NavigationBar(const NavigationConfig& config);
  
  ftxui::Element OnRender() override;
  bool OnEvent(ftxui::Event event) override;
  
  void UpdateConfig(const NavigationConfig& config);
  
private:
  NavigationConfig config_;
  ftxui::Component back_btn_;
  ftxui::Component continue_btn_;
  ftxui::Component skip_btn_;
  ftxui::Component container_;
  
  void CreateButtons();
};

// Factory function to create NavigationBar component
ftxui::Component MakeNavigationBar(const NavigationConfig& config);

} // namespace app
