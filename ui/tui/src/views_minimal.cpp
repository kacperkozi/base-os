#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <string>
#include <functional>

using namespace ftxui;

// Minimal state for MVP - no complex threading or validation
struct SimpleState {
  std::string to_address;
  std::string amount_wei;
  std::string gas_limit = "21000";
  std::string nonce = "0";
  std::string status_message;
  bool transaction_ready = false;
  int current_screen = 0; // 0=input, 1=review, 2=result
};

namespace {

// Simple input validation
bool isValidAddress(const std::string& addr) {
  return addr.length() == 42 && addr.substr(0, 2) == "0x";
}

bool isValidNumber(const std::string& num) {
  if (num.empty()) return false;
  for (char c : num) {
    if (c < '0' || c > '9') return false;
  }
  return true;
}

// Screen 1: Simple Transaction Input
Component SimpleTransactionInput(SimpleState& state) {
  auto to_input = Input(&state.to_address, "0x...");
  auto amount_input = Input(&state.amount_wei, "Amount in Wei");
  auto gas_input = Input(&state.gas_limit, "21000");
  auto nonce_input = Input(&state.nonce, "0");
  
  auto review_btn = Button("Review", [&]{
    // Simple validation
    if (!isValidAddress(state.to_address)) {
      state.status_message = "Invalid address format";
      return;
    }
    if (!isValidNumber(state.amount_wei)) {
      state.status_message = "Invalid amount";
      return;
    }
    if (!isValidNumber(state.gas_limit)) {
      state.status_message = "Invalid gas limit";
      return;
    }
    if (!isValidNumber(state.nonce)) {
      state.status_message = "Invalid nonce";
      return;
    }
    
    state.transaction_ready = true;
    state.status_message = "Transaction ready for review";
    state.current_screen = 1;
  });
  
  auto layout = Container::Vertical({
    to_input,
    amount_input,
    gas_input,
    nonce_input,
    review_btn
  });
  
  return Renderer(layout, [&]{
    return vbox({
      text("Base OS - Simple Transaction Input") | bold | center,
      separator(),
      text(""),
      
      hbox({
        text("To Address: ") | size(WIDTH, EQUAL, 15),
        to_input->Render()
      }),
      text(""),
      
      hbox({
        text("Amount (Wei): ") | size(WIDTH, EQUAL, 15),
        amount_input->Render()
      }),
      text(""),
      
      hbox({
        text("Gas Limit: ") | size(WIDTH, EQUAL, 15),
        gas_input->Render()
      }),
      text(""),
      
      hbox({
        text("Nonce: ") | size(WIDTH, EQUAL, 15),
        nonce_input->Render()
      }),
      text(""),
      
      hbox({
        filler(),
        review_btn->Render(),
        filler()
      }),
      text(""),
      
      // Status message
      state.status_message.empty() ? text("") : 
        text(state.status_message) | center | 
        (state.status_message.find("ready") != std::string::npos ? 
         color(Color::Green) : color(Color::Red))
    }) | border | size(WIDTH, EQUAL, 80) | center;
  });
}

// Screen 2: Simple Review
Component SimpleReview(SimpleState& state) {
  auto sign_btn = Button("Sign Transaction", [&]{
    // Mock signing - just create a simple hex string
    state.status_message = "Transaction signed! (Mock)";
    state.current_screen = 2;
  });
  
  auto back_btn = Button("Back", [&]{
    state.current_screen = 0;
  });
  
  auto layout = Container::Horizontal({sign_btn, back_btn});
  
  return Renderer(layout, [&]{
    return vbox({
      text("Review Transaction") | bold | center,
      separator(),
      text(""),
      
      hbox({
        text("To: ") | bold,
        text(state.to_address) | color(Color::Green)
      }),
      text(""),
      
      hbox({
        text("Amount: ") | bold,
        text(state.amount_wei + " Wei") | color(Color::Green)
      }),
      text(""),
      
      hbox({
        text("Gas Limit: ") | bold,
        text(state.gas_limit) | color(Color::Green)
      }),
      text(""),
      
      hbox({
        text("Nonce: ") | bold,
        text(state.nonce) | color(Color::Green)
      }),
      text(""),
      
      separator(),
      text(""),
      
      hbox({
        filler(),
        sign_btn->Render(),
        text("  "),
        back_btn->Render(),
        filler()
      })
    }) | border | size(WIDTH, EQUAL, 80) | center;
  });
}

// Screen 3: Simple Result
Component SimpleResult(SimpleState& state) {
  auto new_tx_btn = Button("New Transaction", [&]{
    // Reset for new transaction
    state.to_address.clear();
    state.amount_wei.clear();
    state.gas_limit = "21000";
    state.nonce = "0";
    state.status_message.clear();
    state.transaction_ready = false;
    state.current_screen = 0;
  });
  
  auto quit_btn = Button("Quit", [&]{
    std::exit(0);
  });
  
  auto layout = Container::Horizontal({new_tx_btn, quit_btn});
  
  return Renderer(layout, [&]{
    return vbox({
      text("Transaction Complete") | bold | center | color(Color::Green),
      separator(),
      text(""),
      
      text("✓ Transaction has been signed") | center | color(Color::Green),
      text(""),
      text("In a real implementation, this would show:") | center | dim,
      text("• Signed transaction hex") | center | dim,
      text("• QR code for broadcasting") | center | dim,
      text(""),
      
      separator(),
      text(""),
      
      hbox({
        filler(),
        new_tx_btn->Render(),
        text("  "),
        quit_btn->Render(),
        filler()
      })
    }) | border | size(WIDTH, EQUAL, 80) | center;
  });
}

} // namespace

// Minimal MVP Application Runner
int RunMinimalApp() {
  SimpleState state;
  
  auto screen = ScreenInteractive::FitComponent();
  auto exit = screen.ExitLoopClosure();
  
  // Simple router - no complex state management
  auto router = Renderer([&]{
    switch(state.current_screen) {
      case 0:
        return SimpleTransactionInput(state)->Render();
      case 1:
        return SimpleReview(state)->Render();
      case 2:
        return SimpleResult(state)->Render();
      default:
        return SimpleTransactionInput(state)->Render();
    }
  });
  
  // Simple global shortcuts - no complex event handling
  router |= CatchEvent([&](Event e){
    if (e == Event::Escape) {
      if (state.current_screen > 0) {
        state.current_screen--;
        return true;
      } else {
        exit();
        return true;
      }
    }
    
    // Ctrl+Q to quit
    if (e == Event::CtrlQ) {
      exit();
      return true;
    }
    
    return false;
  });
  
  // NO THREADING - this was causing the crashes
  screen.Loop(router);
  
  return 0;
}
