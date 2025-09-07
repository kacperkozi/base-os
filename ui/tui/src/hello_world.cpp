#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <string>

using namespace ftxui;

// Ultra-minimal test to verify FTXUI works
int RunHelloWorld() {
  std::string name = "Base OS";
  
  auto input = Input(&name, "Enter your name");
  auto button = Button("Quit", [&] { std::exit(0); });
  
  auto layout = Container::Vertical({input, button});
  
  auto renderer = Renderer(layout, [&] {
    return vbox({
      text("🔐 Base OS TUI - Hello World Test") | bold | center | color(Color::Green),
      separator(),
      text(""),
      hbox({
        text("Name: "),
        input->Render()
      }),
      text(""),
      hbox({
        text("Hello, " + name + "!") | color(Color::Cyan)
      }),
      text(""),
      hbox({
        filler(),
        button->Render(),
        filler()
      }),
      text(""),
      text("Press Ctrl+C or click Quit to exit") | center | dim
    }) | border | center;
  });
  
  auto screen = ScreenInteractive::FitComponent();
  screen.Loop(renderer);
  
  return 0;
}
