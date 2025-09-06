#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/canvas.hpp>
#include <ftxui/screen/color.hpp>
#include "app/state.hpp"
#include "app/validation.hpp"
#include <functional>
#include <fstream>

using namespace ftxui;
using app::AppState;
using app::UnsignedTx;

namespace {

Component StatusBar(AppState& s) {
  return Renderer([&] {
    auto left = text("Offline • ChainId 8453") | dim;
    auto mid = s.status.empty() ? text("") : text(" "+s.status+" ") | color(Color::Green);
    auto right = text("F1 Help  F10 Quit  Ctrl+R Raw  Ctrl+S Save") | dim;
    return hbox({ left, filler(), mid, filler(), right }) | bgcolor(Color::Black) | color(Color::White);
  });
}

Component Banner(AppState& s) {
  return Renderer([&]{
    Elements lines;
    if (!s.error.empty()) lines.push_back(text("Error: "+s.error) | color(Color::Red));
    if (!s.info.empty()) lines.push_back(text("Info: "+s.info) | color(Color::Yellow));
    if (lines.empty()) return text("");
    return vbox(std::move(lines)) | border | color(Color::White);
  });
}

Component HomeView(AppState& s) {
  std::vector<std::string> items = {
    "Create (Simple)","Create (Advanced)","Import","Settings","Help"
  };
  int selected = 0;
  auto menu = Menu(&items, &selected);
  auto content = Renderer(menu, [&]{
    return vbox({
      text("Base OS – Offline Signer") | bold | center,
      separator(),
      menu->Render() | border,
    }) | center | border;
  });
  content |= CatchEvent([&](Event e){
    if (e == Event::Return) {
      switch(selected){
        case 0: s.route = app::Route::CreateSimple; break;
        case 1: s.route = app::Route::CreateAdvanced; break;
        case 2: s.route = app::Route::Import; break;
        case 3: s.route = app::Route::Settings; break;
        case 4: s.route = app::Route::Help; break;
      }
      return true;
    }
    return false;
  });
  return content;
}

Component CreateSimpleView(AppState& s) {
  UnsignedTx tx = s.unsigned_tx;
  std::string to = tx.to;
  std::string value = tx.value;
  std::string nonce = tx.nonce;
  std::string gas_limit = tx.gas_limit;
  std::string max_fee = tx.max_fee_per_gas;
  std::string max_tip = tx.max_priority_fee_per_gas;
  std::string data = tx.data.empty()?"0x":tx.data;
  std::string chain = std::to_string(tx.chain_id);

  auto in_to = Input(&to, "0x...");
  auto in_val = Input(&value, "1000000000000000");
  auto in_nonce = Input(&nonce, "7");
  auto in_gas = Input(&gas_limit, "21000");
  auto in_fee = Input(&max_fee, "2000000000");
  auto in_tip = Input(&max_tip, "100000000");
  auto in_data = Input(&data, "0x");
  auto in_chain = Input(&chain, "8453");

  std::string err;

  auto review_btn = Button("Review", [&]{
    UnsignedTx next{ to, value, data, nonce, gas_limit, max_fee, max_tip, std::stoi(chain), 2 };
    auto errs = app::Validate(next, true);
    if (!errs.empty()) {
      err = "Fields: ";
      for (size_t i=0;i<errs.size();++i){ err += errs[i]; if (i+1<errs.size()) err += ", "; }
      return;
    }
    s.unsigned_tx = next; s.has_unsigned = true; s.route = app::Route::Review; err.clear();
  });
  auto back_btn = Button("Back", [&]{ s.route = app::Route::Home; });

  auto layout = Container::Vertical({in_to,in_val,in_nonce,in_gas,in_fee,in_tip,in_data,in_chain,review_btn,back_btn});
  return Renderer(layout, [&]{
    return vbox({
      text("Create – Simple Transfer") | bold,
      separator(),
      hbox({text("To" ) | size(WIDTH, EQUAL, 18), in_to->Render()}) ,
      hbox({text("Amount (wei)") | size(WIDTH, EQUAL, 18), in_val->Render()}),
      hbox({text("Nonce") | size(WIDTH, EQUAL, 18), in_nonce->Render(), filler(), text("Gas Limit"), in_gas->Render()}),
      hbox({text("Max Fee (wei)") | size(WIDTH, EQUAL, 18), in_fee->Render()}),
      hbox({text("Max Priority Fee") | size(WIDTH, EQUAL, 18), in_tip->Render()}),
      hbox({text("Data") | size(WIDTH, EQUAL, 18), in_data->Render()}),
      hbox({text("ChainId") | size(WIDTH, EQUAL, 18), in_chain->Render(), text("(locked)") | dim}),
      (err.empty()? text("") : text("Errors: "+err) | color(Color::Red)),
      hbox({review_btn->Render(), text("  "), back_btn->Render()}),
    }) | border;
  });
}

Component CreateAdvancedView(AppState& s) {
  auto simple = CreateSimpleView(s);
  return Renderer(simple, [&]{ return vbox({ text("Create – Advanced") | bold, separator(), simple->Render() }) | border; });
}

Component ImportView(AppState& s) {
  std::vector<std::string> sources = {"Paste","File"};
  int which = 0;
  auto src = Radiobox(&sources, &which);
  std::string pasted;
  std::string path;
  auto in_paste = Input(&pasted, "{...} or 0x...");
  auto in_path = Input(&path, "/media/usb/unsigned.json");
  std::string msg;
  auto parse = Button("Parse", [&]{
    try {
      if (!pasted.empty() && pasted.rfind("0x",0)==0) throw std::runtime_error("RLP hex not supported in scaffold");
      if (pasted.empty()) throw std::runtime_error("Paste is empty (scaffold only supports paste)");
      UnsignedTx t = s.unsigned_tx;
      t.to = "0x"; t.value = "0"; t.data = "0x"; t.nonce = "0"; t.gas_limit = "21000"; t.max_fee_per_gas = "0"; t.max_priority_fee_per_gas = "0"; t.chain_id = 8453; t.type=2;
      s.unsigned_tx = t; s.has_unsigned = true; s.route = app::Route::Review; msg.clear();
    } catch(const std::exception& e){ msg = e.what(); }
  });
  auto back = Button("Back", [&]{ s.route = app::Route::Home; });
  auto layout = Container::Vertical({src,in_paste,in_path,parse,back});
  return Renderer(layout, [&]{
    return vbox({
      text("Import Unsigned Transaction") | bold,
      separator(),
      hbox({text("Source" ) | size(WIDTH,EQUAL,12), src->Render()}),
      hbox({text("Paste" ) | size(WIDTH,EQUAL,12), in_paste->Render()}),
      hbox({text("File" ) | size(WIDTH,EQUAL,12), in_path->Render()}),
      (msg.empty()? text("") : text(msg) | color(Color::Red)),
      hbox({parse->Render(), text("  "), back->Render()}),
    }) | border;
  });
}

Component ReviewView(AppState& s) {
  int tab = 0; std::vector<std::string> tabs = {"Summary","Raw JSON","Raw RLP"};
  auto tabsel = Radiobox(&tabs, &tab);
  auto next = Button("Connect & Sign", [&]{ s.route = app::Route::Sign; });
  auto back = Button("Back", [&]{ s.route = app::Route::Home; });
  auto content = Renderer([&]{
    auto tx = s.unsigned_tx;
    Element body;
    if (tab==0) {
      body = vbox({
        text("To: ")+text(tx.to),
        text("Amount (wei): ")+text(tx.value),
        text("Nonce: ")+text(tx.nonce),
        text("Gas: ")+text("limit ")+text(tx.gas_limit)+text(", maxFee ")+text(tx.max_fee_per_gas)+text(", tip ")+text(tx.max_priority_fee_per_gas),
        text("ChainId: ")+text(std::to_string(tx.chain_id)),
        text("Data: ")+text(tx.data),
      });
    } else if (tab==1) {
      body = paragraph("{ \"to\": \""+tx.to+"\", ... } (scaffold)") | frame | size(HEIGHT, LESS_THAN, 10);
    } else {
      body = paragraph("0x... (RLP scaffold)") | frame | size(HEIGHT, LESS_THAN, 10);
    }
    return vbox({ tabsel->Render(), separator(), body, separator(), hbox({ next->Render(), text("  "), back->Render() }) });
  });
  auto layout = Container::Vertical({tabsel, next, back});
  auto rendered = Renderer(layout, [&]{ return window(text("Review"), content->Render()); });
  rendered |= CatchEvent([&](Event e){
    if ((e.is_ctrl() && e == Event::Character('r')) || e == Event::Character('r')) { tab = (tab+1)%3; return true; }
    return false;
  });
  return rendered;
}

Component SignView(AppState& s) {
  std::vector<std::string> devices = {"Ledger", "Trezor"};
  int selected = s.selected_device;
  auto menu = Menu(&devices, &selected);
  std::string path = s.account_path;
  auto in_path = Input(&path, "m/44'/60'/0'/0/0");
  auto sign = Button("Sign on Device", [&]{ s.signed_hex = "0xdeadbeef"; s.has_signed = true; s.route = app::Route::Export; });
  auto back = Button("Back", [&]{ s.route = app::Route::Review; });
  auto layout = Container::Vertical({menu, in_path, sign, back});
  return Renderer(layout, [&]{
    return vbox({
      text("Connect & Sign") | bold,
      separator(),
      hbox({text("Device") | size(WIDTH,EQUAL,12), menu->Render()}),
      hbox({text("Account path") | size(WIDTH,EQUAL,12), in_path->Render()}),
      hbox({sign->Render(), text("  "), back->Render()}),
      text("Status: Open Ethereum app on device.") | dim,
    }) | border;
  });
}

Component ExportView(AppState& s) {
  std::string path;
  auto in_path = Input(&path, "/home/user/signed.txt");
  std::string save_msg;
  auto do_save = [&]{
    if (s.signed_hex.empty()) { save_msg = "Nothing to save"; return; }
    std::ofstream f(path);
    if (!f.good()) { save_msg = "Cannot open file"; return; }
    f << s.signed_hex << "\n";
    save_msg = "Saved";
  };
  auto save = Button("Save", do_save);
  auto back = Button("Back to Home", [&]{ s.route = app::Route::Home; });
  auto qr = Renderer([&]{
    int w = 30, h = 12; Canvas c(w,h);
    c.DrawText(10,6, "QR TBD");
    return canvas(c) | border;
  });
  auto layout = Container::Vertical({in_path, save, back});
  auto rendered = Renderer(layout, [&]{
    return vbox({
      text("Export Signed Transaction") | bold,
      separator(),
      text("Signed Hex:"),
      paragraph(s.signed_hex.empty()?"":s.signed_hex) | frame | size(HEIGHT, LESS_THAN, 6),
      text("QR:") , qr->Render(),
      hbox({ text("Save to file"), in_path->Render(), save->Render()}),
      (save_msg.empty()? text("") : text(save_msg) | color(Color::Yellow)),
      back->Render(),
    }) | border;
  });
  rendered |= CatchEvent([&](Event e){
    if ((e.is_ctrl() && e == Event::Character('s')) || e == Event::Character('s')) { do_save(); return true; }
    return false;
  });
  return rendered;
}

Component SettingsView(AppState& s) {
  std::vector<std::string> theme = {"Auto","Light","Dark"};
  int t=0; auto rb = Radiobox(&theme,&t);
  auto back = Button("Back", [&]{ s.route = app::Route::Home; });
  auto layout = Container::Vertical({rb, back});
  return Renderer(layout, [&]{
    return vbox({ text("Settings") | bold, separator(), hbox({text("Theme"), rb->Render()}), back->Render() }) | border;
  });
}

Component HelpView(AppState& s) {
  auto back = Button("Back", [&]{ s.route = app::Route::Home; });
  auto layout = Container::Vertical({back});
  return Renderer(layout, [&]{
    return vbox({
      text("Help") | bold,
      separator(),
      paragraph("Navigation: Tab/Shift+Tab, Arrows, Enter, Esc. Global: F1 Help, F10 Quit, Ctrl+R Raw, Ctrl+S Save."),
      back->Render()
    }) | border;
  });
}

} // namespace

Component MakeRoute(AppState& s) {
  switch(s.route) {
    case app::Route::Home: return HomeView(s);
    case app::Route::CreateSimple: return CreateSimpleView(s);
    case app::Route::CreateAdvanced: return CreateAdvancedView(s);
    case app::Route::Import: return ImportView(s);
    case app::Route::Review: return ReviewView(s);
    case app::Route::Sign: return SignView(s);
    case app::Route::Export: return ExportView(s);
    case app::Route::Settings: return SettingsView(s);
    case app::Route::Help: return HelpView(s);
  }
  return HomeView(s);
}

int RunApp() {
  AppState state;
  auto screen = ScreenInteractive::Fullscreen();
  auto exit = screen.ExitLoopClosure();

  auto route = Renderer([&]{ return MakeRoute(state)->Render(); });
  auto layout = Container::Vertical({ route });
  auto ban = Banner(state);
  auto status = StatusBar(state);
  auto root = Renderer(layout, [&]{
    return vbox({ ban->Render(), route->Render(), separator(), status->Render() });
  });

  root |= CatchEvent([&](Event e){
    if (e == Event::F1 || e == Event::Character('?') || e == Event::Character('h')) { state.route = app::Route::Help; return true; }
    if ((e.is_ctrl() && e == Event::Character('q')) || e == Event::F10 || e == Event::Character('q')) { exit(); return true; }
    return false;
  });

  screen.Loop(root);
  return 0;
}
