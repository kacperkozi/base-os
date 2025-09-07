#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <string>
#include <thread>
#include <chrono>

using namespace ftxui;

// Enhanced transaction app with proper flow
int RunSimpleTransaction() {
  // Application state
  enum class Screen { INPUT, REVIEW, SIGNING, COMPLETE };
  Screen current_screen = Screen::INPUT;
  
  // Transaction data
  std::string to_address;
  std::string amount_wei;
  std::string status = "Enter transaction details";
  std::string tx_hash = "";
  
  // Input screen components
  auto input_to = Input(&to_address, "0x...");
  auto input_amount = Input(&amount_wei, "Amount in Wei");
  
  // Validation function
  auto validate_inputs = [&]() -> std::string {
    if (to_address.length() != 42) {
      return "Invalid address length (must be 42 characters)";
    }
    if (to_address.substr(0, 2) != "0x") {
      return "Address must start with 0x";
    }
    if (amount_wei.empty()) {
      return "Amount is required";
    }
    try {
      std::stoull(amount_wei);
    } catch (...) {
      return "Amount must be a valid number";
    }
    return "";
  };
  
  // Button actions
  auto submit_btn = Button("Review Transaction", [&] {
    std::string error = validate_inputs();
    if (!error.empty()) {
      status = error;
    } else {
      current_screen = Screen::REVIEW;
      status = "Review your transaction details";
    }
  });
  
  auto confirm_btn = Button("Confirm & Sign", [&] {
    current_screen = Screen::SIGNING;
    status = "Signing transaction...";
  });
  
  auto back_btn = Button("← Back", [&] {
    current_screen = Screen::INPUT;
    status = "Enter transaction details";
  });
  
  auto sign_another_btn = Button("New Transaction", [&] {
    current_screen = Screen::INPUT;
    to_address.clear();
    amount_wei.clear();
    tx_hash.clear();
    status = "Enter transaction details";
  });
  
  auto quit_btn = Button("Quit", [&] { 
    std::exit(0); 
  });
  
  // Simulate signing process
  auto simulate_signing = [&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    tx_hash = "0x" + std::string(64, 'a'); // Mock transaction hash
    current_screen = Screen::COMPLETE;
    status = "Transaction signed successfully!";
  };
  
  // Layout containers for different screens
  auto input_layout = Container::Vertical({
    input_to,
    input_amount,
    Container::Horizontal({submit_btn, quit_btn})
  });
  
  auto review_layout = Container::Vertical({
    Container::Horizontal({back_btn, confirm_btn, quit_btn})
  });
  
  auto complete_layout = Container::Vertical({
    Container::Horizontal({sign_another_btn, quit_btn})
  });
  
  // Main container that switches between screens
  auto main_container = Container::Tab({
    input_layout,
    review_layout,
    review_layout, // SIGNING uses same layout as review
    complete_layout
  }, (int*)&current_screen);
  
  auto renderer = Renderer(main_container, [&] {
    switch (current_screen) {
      case Screen::INPUT:
        return vbox({
          text("🔐 Base OS - Simple Transaction") | bold | center | color(Color::Green),
          separator(),
          text(""),
          hbox({
            text("To Address: ") | size(WIDTH, EQUAL, 15),
            input_to->Render()
          }),
          text(""),
          hbox({
            text("Amount (Wei): ") | size(WIDTH, EQUAL, 15),
            input_amount->Render()
          }),
          text(""),
          hbox({
            filler(),
            submit_btn->Render(),
            text("  "),
            quit_btn->Render(),
            filler()
          }),
          text(""),
          text(status) | center | (status.find("Invalid") != std::string::npos || 
                                   status.find("required") != std::string::npos ||
                                   status.find("must") != std::string::npos ? 
                                   color(Color::Red) : color(Color::Yellow)),
          text(""),
          text("Tab: Navigate • Enter: Continue • Ctrl+C: Quit") | center | dim
        }) | border | center;
        
      case Screen::REVIEW:
        return vbox({
          text("🔍 Review Transaction") | bold | center | color(Color::Blue),
          separator(),
          text(""),
          hbox({
            text("To Address: ") | size(WIDTH, EQUAL, 15) | bold,
            text(to_address) | color(Color::Cyan)
          }),
          text(""),
          hbox({
            text("Amount: ") | size(WIDTH, EQUAL, 15) | bold,
            text(amount_wei + " Wei") | color(Color::Yellow)
          }),
          text(""),
          hbox({
            text("Network: ") | size(WIDTH, EQUAL, 15) | bold,
            text("Ethereum Mainnet") | color(Color::Green)
          }),
          text(""),
          text("⚠️  Please verify all details before signing") | center | color(Color::Yellow),
          text(""),
          hbox({
            filler(),
            back_btn->Render(),
            text("  "),
            confirm_btn->Render(),
            text("  "),
            quit_btn->Render(),
            filler()
          }),
          text(""),
          text(status) | center | color(Color::Green),
          text(""),
          text("Tab: Navigate • Enter: Continue • Escape: Back") | center | dim
        }) | border | center;
        
      case Screen::SIGNING:
        // Start signing simulation in background
        static bool signing_started = false;
        if (!signing_started) {
          signing_started = true;
          std::thread(simulate_signing).detach();
        }
        
        return vbox({
          text("🔐 Signing Transaction") | bold | center | color(Color::Magenta),
          separator(),
          text(""),
          text("") | center,
          text("⏳ Processing your transaction...") | center | color(Color::Yellow),
          text("") | center,
          text("🔒 Secure signing in progress") | center | color(Color::Green),
          text("") | center,
          text("Please wait, do not close the application") | center | dim,
          text(""),
          text(status) | center | color(Color::Blue),
          text(""),
          text("This may take a few moments...") | center | dim
        }) | border | center;
        
      case Screen::COMPLETE:
        return vbox({
          text("✅ Transaction Complete") | bold | center | color(Color::Green),
          separator(),
          text(""),
          hbox({
            text("Status: ") | size(WIDTH, EQUAL, 15) | bold,
            text("Successfully Signed") | color(Color::Green)
          }),
          text(""),
          hbox({
            text("To Address: ") | size(WIDTH, EQUAL, 15) | bold,
            text(to_address) | color(Color::Cyan)
          }),
          text(""),
          hbox({
            text("Amount: ") | size(WIDTH, EQUAL, 15) | bold,
            text(amount_wei + " Wei") | color(Color::Yellow)
          }),
          text(""),
          hbox({
            text("TX Hash: ") | size(WIDTH, EQUAL, 15) | bold,
            text(tx_hash.substr(0, 20) + "...") | color(Color::Magenta)
          }),
          text(""),
          text("🎉 Your transaction has been signed successfully!") | center | color(Color::Green),
          text(""),
          hbox({
            filler(),
            sign_another_btn->Render(),
            text("  "),
            quit_btn->Render(),
            filler()
          }),
          text(""),
          text(status) | center | color(Color::Green),
          text(""),
          text("Tab: Navigate • Enter: Continue • Ctrl+C: Quit") | center | dim
        }) | border | center;
    }
    
    return text("Error: Unknown screen") | center;
  });
  
  auto screen = ScreenInteractive::FitComponent();
  screen.Loop(renderer);
  
  return 0;
}
