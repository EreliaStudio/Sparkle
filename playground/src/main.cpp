#include "playground.hpp"

namespace spk
{
	class ScrollBar : public spk::Widget
	{
	public:
		using Contract = spk::SliderBar::Contract;
		using Job = spk::SliderBar::Job;
		using VoidJob = spk::SliderBar::VoidJob;
		using Orientation = spk::SliderBar::Orientation;
		
	private:
		spk::TContractProvider<float> _onEditionContractProvider;

		spk::PushButton _positiveButton;
		spk::PushButton::Contract _positiveButtonContract;
		spk::PushButton _negativeButton;
		spk::PushButton::Contract _negativeButtonContract;
		spk::SliderBar _sliderBar;
		spk::SliderBar::Contract _sliderBarContract;

		void _onGeometryChange() override
		{
			switch (_sliderBar.orientation())
			{
			case Orientation::Horizontal:
			{
				spk::Vector2UInt buttonSize = geometry().size.y;
				spk::Vector2UInt sliderSize = spk::Vector2UInt(geometry().size.x - buttonSize.x * 2, geometry().size.y);

				_negativeButton.setGeometry({0, geometry().size.y - buttonSize.y}, buttonSize);
				_sliderBar.setBodyCornerSize(sliderSize.y / 2);
				_sliderBar.setGeometry({buttonSize.x, geometry().size.y - buttonSize.y}, sliderSize);
				_positiveButton.setGeometry({buttonSize.x + sliderSize.x, geometry().size.y - buttonSize.y}, buttonSize);
				break;
			}
			case Orientation::Vertical:
			{
				spk::Vector2UInt buttonSize = geometry().size.x;
				spk::Vector2UInt sliderSize = spk::Vector2UInt(geometry().size.x, geometry().size.y - buttonSize.y * 2);

				_negativeButton.setGeometry({geometry().size.x - buttonSize.x, 0}, buttonSize);
				_sliderBar.setBodyCornerSize(sliderSize.x / 2);
				_sliderBar.setGeometry({geometry().size.x - buttonSize.x, buttonSize.y}, sliderSize);
				_positiveButton.setGeometry({geometry().size.x - buttonSize.x, buttonSize.y + sliderSize.y}, buttonSize);

				break;
			}
			}
		}		
		
	public:
		ScrollBar(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
			spk::Widget(p_name, p_parent),
			_positiveButton(p_name + L" - Positive button", this),
			_negativeButton(p_name + L" - Negative button", this),
			_sliderBar(p_name + L" - Slider bar", this)
		{
			_positiveButton.setCornerSize(2);
			_positiveButton.setIconset(spk::Widget::defaultIconset());
			_positiveButton.activate();

			_positiveButtonContract = _positiveButton.subscribe([&](){
					_sliderBar.setRatio(_sliderBar.ratio() + 0.1f);
					_onEditionContractProvider.trigger(_sliderBar.ratio());
				});
			
			_negativeButton.setCornerSize(2);
			_negativeButton.setIconset(spk::Widget::defaultIconset());
			_negativeButton.activate();
			_negativeButtonContract = _negativeButton.subscribe([&](){
					_sliderBar.setRatio(_sliderBar.ratio() - 0.1f);
					_onEditionContractProvider.trigger(_sliderBar.ratio());
				});

			_sliderBar.activate();
			_sliderBarContract = _sliderBar.subscribe([&](const float& p_ratio){_onEditionContractProvider.trigger(_sliderBar.ratio());});
		}

		Contract subscribe(const Job& p_job)
		{
			return (_onEditionContractProvider.subscribe(p_job));
		}

		Contract subscribe(const VoidJob& p_job)
		{
			return (_onEditionContractProvider.subscribe(p_job));
		}

		void setOrientation(const Orientation& p_orientation)
		{
			_sliderBar.setOrientation(p_orientation);
			switch (p_orientation)
			{
			case Orientation::Horizontal:
			{
				_positiveButton.setSprite(spk::Widget::defaultIconset()->sprite(7));
				_negativeButton.setSprite(spk::Widget::defaultIconset()->sprite(6));
				break;
			}
			case Orientation::Vertical:
			{
				_positiveButton.setSprite(spk::Widget::defaultIconset()->sprite(5));
				_negativeButton.setSprite(spk::Widget::defaultIconset()->sprite(4));
				break;
			}
			}
		}

		void setScale(const float p_scale)
		{
			_sliderBar.setScale(p_scale);
		}

		float ratio()
		{
			return (_sliderBar.ratio());
		}
	};

	class IScrollableWidget : public spk::Widget
	{
	private:
		spk::ScrollBar _horizontalScrollBar;
		spk::ScrollBar::Contract _horizontalBarContract;
		spk::ScrollBar _verticalScrollBar;
		spk::ScrollBar::Contract _verticalBarContract;
		spk::ContainerWidget _containerWidget;

		float _scrollBarWidth = 16;

		void _onGeometryChange() override
		{
			spk::Vector2UInt containerSize = geometry().size;
			spk::Vector2Int contentAnchor = 0;
			spk::Vector2UInt delta = 0;

			if (geometry().size.y <= _containerWidget.contentSize().y)
			{
				containerSize.x -= _scrollBarWidth;
				_verticalScrollBar.activate();
				contentAnchor.y += -_verticalScrollBar.ratio() * static_cast<float>(_containerWidget.contentSize().y - containerSize.y);
			}
			else
			{
				_verticalScrollBar.deactivate();
			}

			if (geometry().size.x <= _containerWidget.contentSize().x)
			{
				containerSize.y -= _scrollBarWidth;
				_horizontalScrollBar.activate();
				contentAnchor.x += -_horizontalScrollBar.ratio() * static_cast<float>(_containerWidget.contentSize().x - containerSize.x);
			}
			else
			{
				_horizontalScrollBar.deactivate();
			}

			containerSize = spk::Vector2UInt::min(containerSize, _containerWidget.contentSize());

			_containerWidget.setContentAnchor(contentAnchor);
			_containerWidget.setGeometry(0, containerSize);

			_horizontalScrollBar.setGeometry({0, containerSize.y}, {containerSize.x, _scrollBarWidth});
			_horizontalScrollBar.setScale(static_cast<float>(geometry().size.x) / static_cast<float>(_containerWidget.contentSize().x));
			_verticalScrollBar.setGeometry({containerSize.x, 0}, {_scrollBarWidth, containerSize.y});
			_verticalScrollBar.setScale(static_cast<float>(geometry().size.y) / static_cast<float>(_containerWidget.contentSize().y));
		}

	public:
		IScrollableWidget(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
			spk::Widget(p_name, p_parent),
			_horizontalScrollBar(p_name + L" - Horizontal ScrollBar", this),
			_verticalScrollBar(p_name + L" - Vertical ScrollBar", this),
			_horizontalBarContract(_horizontalScrollBar.subscribe([&](const float& p_ratio){requireGeometryUpdate();})),
			_verticalBarContract(_verticalScrollBar.subscribe([&](const float& p_ratio){requireGeometryUpdate();})),
			_containerWidget(p_name + L" - Container", this)
		{
			_containerWidget.activate();

			setScrollBarWidth(16);

			_horizontalScrollBar.setOrientation(ScrollBar::Orientation::Horizontal);
			_verticalScrollBar.setOrientation(ScrollBar::Orientation::Vertical);
		}

		spk::SafePointer<spk::Widget> container()
		{
			return (&(_containerWidget));
		}

		void setScrollBarWidth(const float& p_scrollBarWidth)
		{
			_scrollBarWidth = p_scrollBarWidth;
			requireGeometryUpdate();
		}

		void setContent(spk::SafePointer<Widget> p_content)
		{
			_containerWidget.setContent(p_content);
		}	

		void setContentSize(const spk::Vector2UInt& p_contentSize)
		{
			_containerWidget.setContentSize(p_contentSize);
		}
	};

	template<typename TContentType,
	         typename = std::enable_if_t<std::is_base_of<Widget, TContentType>::value>>
	class ScrollableWidget : public IScrollableWidget
	{
	private:
		TContentType _content;

		using spk::IScrollableWidget::setContent;

	public:
		ScrollableWidget(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
			IScrollableWidget(p_name, p_parent),
			_content(p_name + L" - Content", container())
		{
			setContent(&_content);
			_content.activate();
		}

		spk::SafePointer<TContentType> content()
		{
			return (&(_content));
		}
	};
}

class NodeSelectorWidget : public spk::Widget
{
private:
	static inline const spk::Vector2UInt _nbElement = {10, 20};
	static inline const spk::Vector2UInt _elementSize = {32, 32};

	spk::ColorRenderer _backgroundRenderer;

	void _onGeometryChange() override
	{
		_backgroundRenderer.clear();

		for (size_t i = 0; i < _nbElement.x; i++)
		{
			for (size_t j = 0; j < _nbElement.y; j++)
			{
				_backgroundRenderer.prepareSquare(spk::Geometry2D(
					geometry().anchor + ((_elementSize + 5) * spk::Vector2UInt(i, j)),
					_elementSize
				), layer());
			}
		}
		
		_backgroundRenderer.validate();
	}

	void _onPaintEvent(spk::PaintEvent& p_event) override
	{
		_backgroundRenderer.render();
	}

public:
	NodeSelectorWidget(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent)
	{
		_backgroundRenderer.setColor(spk::Color(220, 50, 50));
	}

	spk::Vector2UInt requiredSize() const
	{
		return (_nbElement * _elementSize + 5 * (_nbElement - 1));
	}
};

class MapEditorHUD : public spk::Widget
{
private:
	class InterfaceContent : public spk::Widget
	{
	private:
		spk::PushButton _layerSelectionPushButtons[5];
		spk::ScrollableWidget<NodeSelectorWidget> _nodeSelector;

		spk::Font::Size defaultFontSize()
		{
			return {16, 0};
		}

		void _onGeometryChange() override
		{
			float space = 5;
			spk::Vector2 nbLayerButtons = {5, 1};
			spk::Vector2 layerSelectionButtonSize = {32, 32};

			spk::Vector2 anchorOffset = {
				(geometry().size.x - (layerSelectionButtonSize.x * 5)) / 4, 
				0
			};

			for (size_t i = 0; i < 5; i++)
			{
				_layerSelectionPushButtons[i].setGeometry(
					spk::Vector2((layerSelectionButtonSize.x + anchorOffset.x) * i, 0),
					layerSelectionButtonSize
				);
			}

			_nodeSelector.setGeometry({0, 5 + layerSelectionButtonSize.y}, geometry().size - spk::Vector2UInt(0, layerSelectionButtonSize.y + 5));
		}

	public:
		InterfaceContent(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
			spk::Widget(p_name, p_parent),
			_nodeSelector(L"NodeSelector", this),
			_layerSelectionPushButtons{
				spk::PushButton(L"Layer selection button A", this),
				spk::PushButton(L"Layer selection button B", this),
				spk::PushButton(L"Layer selection button C", this),
				spk::PushButton(L"Layer selection button D", this),
				spk::PushButton(L"Layer selection button E", this)
			}
		{
			for (size_t i = 0; i < 5; i++)
			{
				spk::SafePointer<spk::SpriteSheet> iconset = TextureManager::instance()->spriteSheet(L"iconset");

				_layerSelectionPushButtons[i].setCornerSize(2);
				_layerSelectionPushButtons[i].setIconset(iconset);
				_layerSelectionPushButtons[i].setSprite(iconset->sprite(i));
				_layerSelectionPushButtons[i].activate();
			}

			_nodeSelector.setContentSize(_nodeSelector.content()->requiredSize());
			_nodeSelector.activate();
		}

		spk::Vector2UInt minimalSize()
		{
			float space = 5.0f;
			spk::Vector2 nbLayerButtons = {5, 1};
			spk::Vector2 layerSelectionbutton = {32, 32};

			spk::Vector2 layerLineSize = {
				layerSelectionbutton.x * nbLayerButtons.x + space * (nbLayerButtons.x - 1),
				layerSelectionbutton.y};	

			spk::Vector2 nodeItemSize = {32, 32};
			spk::Vector2 nodeRenderedOnSelector = {8, 4};
			spk::Vector2 nodeSelectorSize = {
				nodeItemSize.x * nodeRenderedOnSelector.x + space * (nodeRenderedOnSelector.x - 1),
				nodeItemSize.y * nodeRenderedOnSelector.y + space * (nodeRenderedOnSelector.y - 1)
			};

			spk::Vector2UInt result = {
					std::max(layerLineSize.x, nodeSelectorSize.x),
					layerLineSize.y + space + nodeSelectorSize.y
				};

			return (result);
		}
	};

	spk::InterfaceWindow<InterfaceContent> _interfaceWindow;

	void _onGeometryChange() override
	{
		spk::Vector2UInt childSize = _interfaceWindow.content()->minimalSize();
		_interfaceWindow.setGeometry((geometry().size - childSize) / 2 , childSize);
	}

	void _onKeyboardEvent(spk::KeyboardEvent& p_event) override
	{
		switch (p_event.type)
		{
			case spk::KeyboardEvent::Type::Press:
			{
				if (p_event.key == spk::Keyboard::F12)
				{
					if (_interfaceWindow.parent() == nullptr)
					{
						addChild(&_interfaceWindow);
					}
				}
				
				break;
			}
		}
	}

	spk::ContractProvider::Contract _quitContract;

public:
	MapEditorHUD(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_interfaceWindow(L"Editor window", this)
	{
		spk::Vector2UInt minimalSize = _interfaceWindow.content()->minimalSize();
		_interfaceWindow.setMinimumContentSize(minimalSize);
		_interfaceWindow.setMenuHeight(25);
		_interfaceWindow.setLayer(10);
		_interfaceWindow.activate();

		_quitContract = _interfaceWindow.subscribeTo(spk::IInterfaceWindow::Event::Close, [&](){removeChild(&_interfaceWindow);});
	}
};

int main()
{
	spk::GraphicalApplication app = spk::GraphicalApplication();

	spk::SafePointer<spk::Window> win = app.createWindow(L"Playground", {{0, 0}, {800, 800}});

	TextureManager::instanciate();
	
	NodeMap nodeMap;

	spk::GameEngine engine;

	spk::Entity worldManager = spk::Entity(L"World manager");
	auto& worldManagerComp = worldManager.addComponent<WorldManager>(L"World manager component");
	worldManager.addComponent<ControlMapper>(L"Control mapper component");

	spk::Entity player = spk::Entity(L"Player");
	player.transform().place(spk::Vector3(0, 0, 0));
	player.addComponent<PlayerController>(L"Player controler component");

	spk::Entity camera = spk::Entity(L"Camera", &player);
	camera.transform().place(spk::Vector3(0, 0, 20));
	camera.transform().lookAt(player.transform().position());

	CameraManager& cameraComp = camera.addComponent<CameraManager>(L"Main camera");

	worldManagerComp.setCamera(&camera);

	engine.addEntity(&worldManager);
	engine.addEntity(&player);

	MapEditorHUD mapEditorWidget = MapEditorHUD(L"Editor window", win->widget());
	mapEditorWidget.setGeometry(win->geometry());
	mapEditorWidget.activate();

	spk::GameEngineWidget gameEngineWidget = spk::GameEngineWidget(L"Engine widget", win->widget());
	gameEngineWidget.setLayer(0);
	gameEngineWidget.setGeometry(win->geometry());
	gameEngineWidget.setGameEngine(&engine);
	gameEngineWidget.activate();
	
	return (app.run());
}