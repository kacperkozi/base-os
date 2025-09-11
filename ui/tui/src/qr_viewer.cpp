#include "app/qr_viewer.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/animation.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/canvas.hpp>
#include <ftxui/screen/color.hpp>
#include <sstream>
#include <iomanip>

namespace app {

using namespace ftxui;

QrViewer::QrViewer(const std::vector<QRCode>& qr_codes) 
    : qr_codes_(qr_codes),
      last_auto_advance_(std::chrono::steady_clock::now()) {
  
  // Create Previous button with condition check
  auto prev_button_base = Button("< Previous", [this] { 
    if (CanGoPrevious()) PreviousPart(); 
  });
  
  // Wrap previous button with conditional dimming
  previous_button_ = Renderer(prev_button_base, [this, prev_button_base] {
    auto element = prev_button_base->Render();
    return CanGoPrevious() ? element : element | dim;
  });
  
  // Create Next button with condition check
  auto next_button_base = Button("Next >", [this] { 
    if (CanGoNext()) NextPart(); 
  });
  
  // Wrap next button with conditional dimming
  next_button_ = Renderer(next_button_base, [this, next_button_base] {
    auto element = next_button_base->Render();
    return CanGoNext() ? element : element | dim;
  });
  
  // Create auto-play checkbox with Enter key support
  auto checkbox_base = Checkbox("Auto-play (cycles every 4s)", &auto_play_);
  
  // Wrap checkbox to handle Enter key (Space already works by default)
  auto_play_checkbox_ = CatchEvent(checkbox_base, [this](Event event) {
    if (event == Event::Return) {
      auto_play_ = !auto_play_;  // Toggle on Enter
      return true;
    }
    return false;  // Let other events pass through
  });
  
  // Create a component that monitors checkbox state and triggers animation
  auto animation_monitor = Renderer([this] {
    // Monitor autoplay state and trigger animation when needed
    static bool was_playing = false;
    if (auto_play_ && !was_playing) {
      animation::RequestAnimationFrame();
      last_auto_advance_ = std::chrono::steady_clock::now();
    }
    was_playing = auto_play_;
    return text("");  // Invisible component
  });
  
  // Create controls container with animation monitor
  auto controls = Container::Horizontal({
    previous_button_,
    auto_play_checkbox_,
    next_button_,
    animation_monitor  // Invisible component that monitors checkbox state
  });
  
  // Store controls for rendering
  controls_container_ = controls;
  
  // Add as child so it receives events
  Add(controls);
}

Element QrViewer::OnRender() {
  // Update auto-play if enabled
  if (auto_play_) {
    UpdateAutoPlay();
  }
  
  Elements elements;
  
  // Title
  elements.push_back(text("Transaction QR Code") | bold | center);
  elements.push_back(separator());
  
  // Progress indicator (e.g., "Part 2 of 5")
  if (!qr_codes_.empty()) {
    std::stringstream ss;
    ss << "Part " << (current_index_ + 1) << " of " << qr_codes_.size();
    elements.push_back(text(ss.str()) | bold | center);
    
    // Add navigation hint for multi-part QRs
    if (qr_codes_.size() > 1) {
      elements.push_back(text("Use ← → or h/l to navigate") | dim | center);
    }
    elements.push_back(text(" "));
    
    // Render the QR code
    elements.push_back(RenderQRCode() | center);
    elements.push_back(text(" "));
    
    // Render controls container to maintain proper event handling
    // The container must render to preserve checkbox data binding
    elements.push_back(controls_container_->Render() | center);
    
    // Progress bar for auto-play
    if (auto_play_) {
      elements.push_back(text(" "));
      elements.push_back(RenderProgressBar());
    }
  } else {
    elements.push_back(text("No QR codes to display") | color(Color::Red) | center);
  }
  
  return vbox(elements) | border;
}

bool QrViewer::OnEvent(Event event) {
  // First, give the event to the child components. If they handle it, we are done.
  if (ComponentBase::OnEvent(event)) {
    return true;
  }

  // If the children did not handle the event, then process custom shortcuts.
  if (event == Event::ArrowLeft || event == Event::Character('h')) {
    PreviousPart();
    return true;
  }
  
  if (event == Event::ArrowRight || event == Event::Character('l')) {
    NextPart();
    return true;
  }
  
  return false;
}

void QrViewer::OnAnimation(animation::Params& params) {
  // Update animation for child components
  ComponentBase::OnAnimation(params);
  
  // If autoplay is enabled, update progress and request more animation frames
  if (auto_play_) {
    UpdateAutoPlay();
    
    // Keep requesting animation frames while autoplay is active
    animation::RequestAnimationFrame();
  }
}

void QrViewer::SetQRCodes(const std::vector<QRCode>& qr_codes) {
  qr_codes_ = qr_codes;
  current_index_ = 0;
  auto_play_ = false;
  last_auto_advance_ = std::chrono::steady_clock::now();
}

void QrViewer::NextPart() {
  if (CanGoNext()) {
    current_index_++;
    last_auto_advance_ = std::chrono::steady_clock::now();
  } else if (auto_play_ && !qr_codes_.empty()) {
    // Loop back to beginning in auto-play mode
    current_index_ = 0;
    last_auto_advance_ = std::chrono::steady_clock::now();
  }
}

void QrViewer::PreviousPart() {
  if (CanGoPrevious()) {
    current_index_--;
    last_auto_advance_ = std::chrono::steady_clock::now();
  }
}

void QrViewer::ToggleAutoPlay() {
  auto_play_ = !auto_play_;
  if (auto_play_) {
    last_auto_advance_ = std::chrono::steady_clock::now();
  }
}

void QrViewer::ResetToFirst() {
  current_index_ = 0;
  auto_play_ = false;
  last_auto_advance_ = std::chrono::steady_clock::now();
}

void QrViewer::SetAutoPlayInterval(std::chrono::milliseconds interval) {
  auto_play_interval_ = interval;
}

void QrViewer::SetPreferASCII(bool prefer) {
  prefer_ascii_ = prefer;
}

bool QrViewer::CanGoPrevious() const {
  return current_index_ > 0;
}

bool QrViewer::CanGoNext() const {
  return current_index_ < static_cast<int>(qr_codes_.size()) - 1;
}

void QrViewer::UpdateAutoPlay() {
  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_auto_advance_);
  
  // Calculate progress for progress bar
  auto_play_progress_ = static_cast<float>(elapsed.count()) / static_cast<float>(auto_play_interval_.count());
  
  // Advance to next QR if interval has elapsed
  if (elapsed >= auto_play_interval_) {
    NextPart();
  }
}

Element QrViewer::RenderQRCode() const {
  if (qr_codes_.empty() || current_index_ >= static_cast<int>(qr_codes_.size())) {
    return text("No QR code available");
  }
  
  const auto& qr = qr_codes_[current_index_];
  
  // FTXUI canvas uses 2x4 Braille dots per character cell
  // We want each QR module to be at least 2x2 dots for visibility
  const int dots_per_module = 2;  // 2x2 dots per QR module minimum
  
  // Calculate desired canvas size in terminal characters
  // Canvas width in chars = (qr.size * dots_per_module) / 2 (2 dots per char width)
  // Canvas height in chars = (qr.size * dots_per_module) / 4 (4 dots per char height)
  int desired_width_chars = (qr.size * dots_per_module + 1) / 2;
  int desired_height_chars = (qr.size * dots_per_module + 3) / 4;
  
  // Limit to reasonable terminal size
  desired_width_chars = std::min(desired_width_chars, 60);
  desired_height_chars = std::min(desired_height_chars, 30);
  
  // Use FTXUI's canvas for better QR code rendering
  auto canvas_element = canvas([&qr](Canvas& c) {
    const int dots_per_module = 2;  // Local copy for lambda
    if (qr.modules.empty()) return;
    
    // Draw each QR module as a filled square of dots
    for (int y = 0; y < qr.size; ++y) {
      for (int x = 0; x < qr.size; ++x) {
        if (qr.modules[y][x]) {
          // Draw a dots_per_module x dots_per_module square for each black module
          for (int dy = 0; dy < dots_per_module; ++dy) {
            for (int dx = 0; dx < dots_per_module; ++dx) {
              int canvas_x = x * dots_per_module + dx;
              int canvas_y = y * dots_per_module + dy;
              c.DrawPointOn(canvas_x, canvas_y);
            }
          }
        }
      }
    }
  });
  
  // Use ASCII rendering when preferred or for very large QR codes
  // If prefer_ascii_ is set, always use ASCII regardless of size
  // Otherwise, only use ASCII for QR codes too large for canvas (>50 modules)
  if (prefer_ascii_ || qr.size > 50) {
    // For very large QR codes, use compact ASCII
    std::string qr_ascii = qr.toHalfBlockAscii();
    std::vector<std::string> lines;
    std::stringstream ss(qr_ascii);
    std::string line;
    
    while (std::getline(ss, line)) {
      lines.push_back(line);
    }
    
    Elements line_elements;
    for (const auto& l : lines) {
      line_elements.push_back(text(l));
    }
    
    return vbox(line_elements) | border;
  }
  
  // Add explicit size constraints to ensure canvas renders at correct size
  return canvas_element 
    | size(WIDTH, EQUAL, desired_width_chars)
    | size(HEIGHT, EQUAL, desired_height_chars)
    | border;
}

Element QrViewer::RenderProgressBar() const {
  // Use FTXUI's gauge component for better progress visualization
  return hbox({
    text("Auto-advance: "),
    gauge(auto_play_progress_) | color(Color::Cyan) | size(WIDTH, EQUAL, 40)
  }) | center;
}

Element QrViewer::RenderControls() const {
  // Always render the actual buttons, apply styling for disabled state
  Element prev_btn = previous_button_->Render();
  Element next_btn = next_button_->Render();
  
  // Apply disabled styling when appropriate
  if (!CanGoPrevious()) {
    prev_btn = prev_btn | dim;
  }
  if (!CanGoNext()) {
    next_btn = next_btn | dim;
  }
  
  // Spacing between controls
  Element spacer = text("    ");
  
  return hbox({
    prev_btn,
    spacer,
    auto_play_checkbox_->Render(),
    spacer,
    next_btn
  }) | center;
}

// Factory function
Component MakeQrViewer(const std::vector<QRCode>& qr_codes) {
  return std::make_shared<QrViewer>(qr_codes);
}

} // namespace app