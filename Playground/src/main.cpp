#include "sparkle.hpp"
#include <limits>
#include <iostream>

using spk::widget::Row;
using spk::widget::Expanded;
using spk::widget::Column;
using spk::widget::SizedBox;
using spk::widget::Padding;
using spk::widget::FractionallySizedBox;

using spk::widget::TextButton;

using std::make_unique;
using std::move;

class MyDemo : public SingleChildWidget {
  public:
    MyDemo() {
      auto row = make_unique<Row>();

      // Left empty padding
      {
        auto leftPad = make_unique<SizedBox>({50, std::numeric_limits<size_t>::max}, row.get());
        _vault.push_back(move(leftPad);
      }

      // Middle column
      {
        auto padding = make_unique<Padding>({15.0f, 15.0f, 15.0f, 15.0f}, row.get());
        auto col = make_unique<Column>(padding.get());

        std::vector<std::string> labels{"LabelA", "LabelB", "LabelC", "LabelD", "LabelE"};
        for (int i = 0; i < labels.size(); i++) {
          auto callback = [label = labels[i]]() {std::cout << label << std::endl; };
          _vault.emplace(make_unique<TextButton>(labels[i], callback, col.get()));
          
          // Only build separators until the last one.
          if (i < (labels.size() - 1)) {
            _vault.emplace(make_unique<SizedBox>({0.0f, 10.0f}, col.get()));
          }
        }
        _vault.push_back(move(col));
        _vault.push_back(move(padding));
      }

      // Right column
     {
      auto expanded = make_unique<Expanded>(row.get());
      auto col = make_unique<Column>(expanded.get());
      // Top section
      {
        auto expanded = make_unique<Expanded>(col.get());
        auto padding = make_unique<Padding>({15, 15, 15, 15}, expanded.get());
        auto innerCol = make_unique<Column>(padding.get());
        auto top = make_unique<Expanded>(innerCol.get());
        auto topButton = make_unique<TextButton>("TopButton", [](){ std::cout << "TopButton" << std::endl; }, top.get());
        auto separator = make_unique<SizedBox>({0, 15}, innerCol.get());
        auto bot = make_unique<Expanded>(innerCol.get());
        auto botButton = make_unique<TextButton>("BotButton", [](){ std::cout << "BotButton" << std::endl; }, top.get());
        vault({expanded, padding, innerCol, top, topButton, separator, bot, botButton});
      }
      // Bottom section
      {
        auto expanded = make_unique<Expanded>(col.get());
        auto center = make_unique<Center>(expanded.get());
        auto frac = make_unique<FractionallySizedBox>({0.5, 0.5}, center.get());
        auto button = make_unique<TextButton>("Second button", []() { std::cout << "Second button" << std::endl; }  frac.get());
        vault({expanded, center, frac, button});
      }
      vault({col, expanded});
     }
     vault(row);
    }

  private:
    void vault(std::unique_ptr<IWidget>& item) {
      _vault.push_back(std::move(item));
    }

    void vault(std::vector<std::unique_ptr<IWidget>&>& stash) {
      for (auto& item : stash) {
        _vault.push_back(move(item));
      }
    }
    std::vector<std::unique_ptr<IWidget>> _vault;
}

int main()
{
    spk::Application app = spk::Application("Playground", spk::Vector2UInt(800, 800), spk::Application::Mode::Monothread);

    MyDemo demo;
    demo.activateAll();

    return (app.run());
}
