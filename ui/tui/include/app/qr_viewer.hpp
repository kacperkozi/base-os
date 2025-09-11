#pragma once
#include <ftxui/component/component.hpp>
#include <vector>
#include <chrono>
#include "app/qr_generator.hpp"

namespace app {

// Interactive component for viewing multi-part QR codes
class QrViewer : public ftxui::ComponentBase {
public:
  // Constructor takes QR codes to display
  explicit QrViewer(const std::vector<QRCode>& qr_codes);
  
  // ComponentBase overrides
  ftxui::Element OnRender() override;
  bool OnEvent(ftxui::Event event) override;
  void OnAnimation(ftxui::animation::Params& params) override;

  // Public methods for state management
  void SetQRCodes(const std::vector<QRCode>& qr_codes);
  void NextPart();
  void PreviousPart();
  void ToggleAutoPlay();
  void ResetToFirst();
  
  // Configuration
  void SetAutoPlayInterval(std::chrono::milliseconds interval);
  void SetPreferASCII(bool prefer);
  
private:
  // QR code data
  std::vector<QRCode> qr_codes_;
  int current_index_ = 0;
  
  // Auto-play state
  bool auto_play_ = false;
  std::chrono::milliseconds auto_play_interval_{4000}; // 4 seconds default
  std::chrono::steady_clock::time_point last_auto_advance_;
  float auto_play_progress_ = 0.0f;
  
  // Rendering preferences
  bool prefer_ascii_ = false;
  
  // UI components
  ftxui::Component controls_container_;
  ftxui::Component previous_button_;
  ftxui::Component next_button_;
  ftxui::Component auto_play_checkbox_;
  
  // Helper methods
  bool CanGoPrevious() const;
  bool CanGoNext() const;
  void UpdateAutoPlay();
  ftxui::Element RenderQRCode() const;
  ftxui::Element RenderProgressBar() const;
  ftxui::Element RenderControls() const;
};

// Factory function to create QrViewer component
ftxui::Component MakeQrViewer(const std::vector<QRCode>& qr_codes);

} // namespace app