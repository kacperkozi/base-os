#pragma once
#include <ftxui/component/component.hpp>
#include "state.hpp"

namespace app {

// View components for each screen
ftxui::Component ConnectWalletView(AppState& state);
ftxui::Component USBContactsView(AppState& state);
ftxui::Component TransactionInputView(AppState& state);
ftxui::Component ConfirmationView(AppState& state);
ftxui::Component SigningView(AppState& state);
ftxui::Component ResultView(AppState& state);
ftxui::Component SettingsView(AppState& state);
ftxui::Component HelpView(AppState& state);
ftxui::Component ErrorView(AppState& state);

} // namespace app