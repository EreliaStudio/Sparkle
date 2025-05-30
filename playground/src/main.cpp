#include <sparkle.hpp>

struct ScrollContent : public spk::Widget
{
    spk::VerticalLayout            layout;
    std::vector<std::unique_ptr<spk::TextLabel>> labels;

    ScrollContent(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent)
    {
        layout.setElementPadding({10, 10});

        for (int i = 0; i < 15; ++i)
        {
            auto lbl = std::make_unique<spk::TextLabel>(p_name + L"/Label" + std::to_wstring(i + 1), this);
            lbl->setText(L"Dummy text label #" + std::to_wstring(i + 1));
            lbl->activate();
            layout.addWidget(lbl.get(), spk::Layout::SizePolicy::HorizontalExtend);

            labels.emplace_back(std::move(lbl));
        }
    }

    void _onGeometryChange() override
    {
        layout.setGeometry({0, geometry().size});
    }

    spk::Vector2UInt minimalSize() const override
    {
        return layout.minimalSize();
    }
};

enum class Event
{
	Exit
};

using EventDispatcher = spk::EventDispatcher<Event>;

class MainWidget : public spk::Widget
{
    spk::ScrollArea<ScrollContent> _scrollArea;

    void _onGeometryChange() override
    {
        _scrollArea.setGeometry({0, geometry().size});
    }

	void _onKeyboardEvent(spk::KeyboardEvent& p_event)
	{
		if (p_event.type == spk::KeyboardEvent::Type::Press &&
			p_event.key == spk::Keyboard::Escape)
		{
			EventDispatcher::emit(Event::Exit);
		}
	}

public:
    MainWidget(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_scrollArea(p_name + L"/ScrollArea", this)
    {
        _scrollArea.setScrollBarVisible(spk::ScrollBar::Orientation::Horizontal, false);
        _scrollArea.activate();
    }
};

int main()
{
	int errorCode = 0;
	{
		spk::GraphicalApplication app;

		{
			auto win = app.createWindow(L"Scroll-area test", {{0, 0}, {800, 600}});

			{
				MainWidget root(L"MainWidget", win->widget());
				root.setGeometry(win->geometry().shrink(50));
				root.activate();

				{
					EventDispatcher::Contract exitContract = EventDispatcher::subscribe(Event::Exit, [&app]()
					{
						app.quit(0);
					});

					errorCode = app.run();
				}
			}
		}
	}
    return errorCode;
}
