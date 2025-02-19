#include "playground.hpp"

class LevelSelector : public spk::Widget
{
private:
	spk::PushButton _pushButtons[5];
	spk::PushButton::Contract _pushButtonContract[5];
	int _level = 0;
	
	void _onGeometryChange() override
	{
		spk::Vector2UInt buttonSize = geometry().size.y;
		float space = (geometry().size.x - (buttonSize.x * 5)) / 4;

		for (size_t i = 0; i < 5; i++)
		{
			_pushButtons[i].setGeometry({(buttonSize.x + space) * i, 0}, buttonSize);
		}
	}

public:
	LevelSelector(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_pushButtons{
			spk::PushButton(p_name + L" - Button A", this),
			spk::PushButton(p_name + L" - Button B", this),
			spk::PushButton(p_name + L" - Button C", this),
			spk::PushButton(p_name + L" - Button D", this),
			spk::PushButton(p_name + L" - Button E", this)
		}
	{
		for (size_t i = 0; i < 5; i++)
		{
			spk::SafePointer<spk::SpriteSheet> iconset = TextureManager::instance()->spriteSheet(L"iconset");

			_pushButtons[i].subscribe([&](){_level = i;});
			_pushButtons[i].setCornerSize(2);
			_pushButtons[i].setIconset(iconset);
			_pushButtons[i].setSprite(iconset->sprite(i));
			_pushButtons[i].activate();
		}
	}

	int level() const
	{
		return (_level);
	}
};

class NodeSelector : public spk::ScrollableWidget
{
private:
	spk::Vector2UInt _nbElement = {0, 0};
	spk::Vector2UInt _elementSize = {32, 32};

	int _nodeIndex = -1;
	bool _texturePrepared = false;
	spk::Vector2Int _selectedNodePosition;

	static inline const std::unordered_map<spk::Vector2Int, int> _nodeToTextureIDMap = {
		{{0, 0}, 0},
		{{1, 0}, 1},
		{{0, 1}, 2},
		{{1, 1}, 3},
		{{5, 0}, 0},
		{{6, 0}, 1},
		{{5, 1}, 2},
		{{6, 1}, 3},
		{{0, 4}, 0},
		{{1, 4}, 1},
		{{0, 5}, 2},
		{{1, 5}, 3},
		{{5, 4}, 0},
		{{6, 4}, 1},
		{{5, 5}, 2},
		{{6, 5}, 3},
	};

	spk::TextureRenderer _contentRenderer;
	spk::TextureRenderer _selectedIconRenderer;
	spk::OpenGL::FrameBufferObject _frameBufferObject;

	void _computeNbElement()
	{
		for (const auto& [position, index] : _nodeToTextureIDMap)
		{
			_nbElement = spk::Vector2UInt::max(_nbElement, position + 1);
		}
	}

	void _prepareTexture()
	{
		_frameBufferObject.activate();

		{
			spk::TextureRenderer renderer;

			spk::SafePointer<spk::SpriteSheet> chunkSpriteSheet = TextureManager::instance()->spriteSheet(L"chunkSpriteSheet");
			renderer.setTexture(chunkSpriteSheet);

			renderer.clear();

			for (const auto& [key, element] : _nodeToTextureIDMap)
			{
				renderer.prepare(
						spk::Geometry2D(key * (_elementSize + 5), _elementSize),
						spk::SpriteSheet::Sprite(Context::instance()->nodeMap[element].animationStartPos * chunkSpriteSheet->unit(), chunkSpriteSheet->unit()),
						0
					);
			}

			renderer.validate();
			renderer.render();
		}

		_frameBufferObject.deactivate();
	}

	void _onGeometryChange() override
	{
		_contentRenderer.clear();
		_contentRenderer.setTexture(_frameBufferObject.bindedTexture(L"outputColor"));
		_contentRenderer.prepare(geometry(), {{0.0f, 0.0f}, {1.0f, 1.0f}}, layer());
		_contentRenderer.validate();

		spk::SafePointer<spk::SpriteSheet> iconset = TextureManager::instance()->spriteSheet(L"iconset");

		_selectedIconRenderer.clear();
		if (_nodeIndex != -1)
		{
			_selectedIconRenderer.prepare(
									spk::Geometry2D(geometry().anchor + _selectedNodePosition * (_elementSize + 5), _elementSize),
									iconset->sprite(10),
									layer() + 0.01f
								);
		}
		_selectedIconRenderer.validate();
	}

	void _onPaintEvent(spk::PaintEvent& p_event) override
	{
		if (_texturePrepared == false)
		{
			_prepareTexture();
			_texturePrepared = true;
		}

		_contentRenderer.render();
		_selectedIconRenderer.render();
	}

	void _onMouseEvent(spk::MouseEvent& p_event) override
	{
		switch (p_event.type)
		{
			case spk::MouseEvent::Type::Press:
			{
				if (viewport().geometry().contains(p_event.mouse->position) == true)
				{
					spk::Vector2Int relativeMousePosition = p_event.mouse->position - absoluteAnchor();

					spk::Vector2 relativePosition = relativeMousePosition / spk::Vector2Int(_elementSize + 5);

					spk::Vector2Int relativeIcon = relativePosition * spk::Vector2Int(_elementSize + 5);
					spk::Vector2Int relativeEndIcon = relativeIcon + _elementSize;

					if (relativeMousePosition.isBetween(relativeIcon, relativeEndIcon) == true &&
						_nodeToTextureIDMap.contains(relativePosition) == true)
					{
						_selectedNodePosition = relativePosition;
						_nodeIndex = _nodeToTextureIDMap.at(relativePosition);
						requireGeometryUpdate();
					}
					else
					{
						if (_nodeIndex != -1)
						{
							requireGeometryUpdate();
						}
						_nodeIndex = -1;
					}	
					p_event.consume();
				}
				break;
			}
		}
	}

public:
	NodeSelector(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::ScrollableWidget(p_name, p_parent)
	{
		_computeNbElement();
		
		_selectedIconRenderer.setTexture(TextureManager::instance()->spriteSheet(L"iconset"));

		_frameBufferObject.resize(requiredSize());

		_frameBufferObject.addAttachment(L"outputColor", 0, spk::OpenGL::FrameBufferObject::Type::Float4);
	}

	int selectedNode() const
	{
		return (_nodeIndex);
	}

	spk::Vector2UInt requiredSize() const
	{
		return (_nbElement * _elementSize + 5 * (_nbElement - 1));
	}
};

class MapEditorInventory : public spk::Widget
{
private:
	LevelSelector _levelSelector;
	spk::ScrollArea<NodeSelector> _nodeSelector;

	spk::Font::Size defaultFontSize()
	{
		return {16, 0};
	}

	void _onGeometryChange() override
	{	
		_levelSelector.setGeometry({0, 0}, {geometry().size.x, 40});
		_nodeSelector.setGeometry({0, 5 + _levelSelector.geometry().size.y}, geometry().size - spk::Vector2UInt(0, _levelSelector.geometry().size.y + 5));
	}

public:
	MapEditorInventory(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_levelSelector(p_name + L" - LevelSelector", this),
		_nodeSelector(p_name + L" - NodeSelector", this)
	{
		_levelSelector.activate();
		_nodeSelector.activate();
	}

	spk::SafePointer<LevelSelector> levelSelector()
	{
		return (&_levelSelector);
	}

	spk::SafePointer<NodeSelector> nodeSelector()
	{
		return (_nodeSelector.content());
	}

	spk::Vector2UInt minimalSize() const
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

class MapEditorInteractor : public spk::Widget
{
private:
	bool _isPlacing;

	spk::SafePointer<NodeSelector> _nodeSelector;
	spk::SafePointer<LevelSelector> _levelSelector;

	spk::Vector2Int convertScreenToWorldPosition(const spk::Vector2Int& p_screenPosition)
	{
		if (Context::instance() == nullptr)
			throw std::runtime_error("Can't access Context.");

		spk::Vector2 relPosition = static_cast<spk::Vector2>(p_screenPosition) / static_cast<spk::Vector2>(geometry().size);
		spk::Vector2 screenPosition = relPosition * 2 - 1;

		spk::Vector3 result = Context::instance()->mainCamera.inverseProjectionMatrix() * spk::Vector3(screenPosition, 0);

		return (spk::Vector2Int::floor(result.xy()) + Context::instance()->cameraEntity.transform().position().xy());
	}
	
	void _onMouseEvent(spk::MouseEvent& p_event)
	{
		if (Context::instance() == nullptr)
			return;

		switch (p_event.type)
		{
			case spk::MouseEvent::Type::Press:
			{
				_isPlacing = true;
				p_event.consume();
			}
			case spk::MouseEvent::Type::Motion:
			{
				if (_isPlacing == true && _nodeSelector != nullptr && _levelSelector != nullptr)
				{
					spk::Vector2Int worldPosition = convertScreenToWorldPosition(p_event.mouse->position);
					int layer = _levelSelector->level();
					int node = _nodeSelector->selectedNode();

					Context::instance()->mapManager.setNode(worldPosition, layer, node);
					p_event.consume();
				}
				break;
			}
			case spk::MouseEvent::Type::Release:
			{
				_isPlacing = false;
				p_event.consume();
				break;
			}
		}
	}

public:
	MapEditorInteractor(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent)
	{

	}

	void setNodeSelector(spk::SafePointer<NodeSelector> p_nodeSelector)
	{
		_nodeSelector = p_nodeSelector;
	}

	void setLevelSelector(spk::SafePointer<LevelSelector> p_levelSelector)
	{
		_levelSelector = p_levelSelector;
	}
};

class MapEditorMenu : public spk::Widget
{
private:
	spk::PushButton _saveButton;
	spk::PushButton::Contract _saveContract;
	spk::PushButton _loadButton;
	spk::PushButton::Contract _loadContract;

	spk::Vector2UInt buttonSize() const
	{
		return {30, 30};
	}

	void _onGeometryChange()
	{
		_saveButton.setGeometry({0, 0}, buttonSize());
		_loadButton.setGeometry({buttonSize().x + 5, 0}, buttonSize());
	}

public:
	MapEditorMenu(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_saveButton(p_name + L" - SaveButton", this),
		_loadButton(p_name + L" - LoadButton", this)
	{
		_saveButton.setIconset(TextureManager::instance()->spriteSheet(L"iconset"));
		_saveButton.setSprite(TextureManager::instance()->spriteSheet(L"iconset")->sprite(5));
		_saveButton.setCornerSize(2);
		_saveContract = _saveButton.subscribe([&](){EventCenter::instance()->notifyEvent(Event::SaveMap);});
		_saveButton.activate();

		_loadButton.setIconset(TextureManager::instance()->spriteSheet(L"iconset"));
		_loadButton.setSprite(TextureManager::instance()->spriteSheet(L"iconset")->sprite(6));
		_loadButton.setCornerSize(2);
		_loadContract = _loadButton.subscribe([&](){EventCenter::instance()->notifyEvent(Event::LoadMap);});
		_loadButton.activate();
	}

	spk::Vector2UInt minimalSize() const
	{
		return {buttonSize().x * 2 + 5, buttonSize().y};
	}

	spk::Vector2UInt maximalSize() const
	{
		return minimalSize();
	}
};

class MapEditorHUD : public spk::Widget
{
private:
	spk::InterfaceWindow<MapEditorMenu> _menu;
	spk::InterfaceWindow<MapEditorInventory> _inventory;
	MapEditorInteractor _interactor;

	spk::ContractProvider::Contract _quitContract;

	void _onGeometryChange() override
	{
		spk::Vector2UInt inventorySize = _inventory.minimalSize();
		spk::Vector2UInt menuSize = _menu.minimalSize();

		_inventory.setGeometry((geometry().size - inventorySize) / 2 , inventorySize);
		_menu.setGeometry({geometry().size.x - menuSize.x, 0}, menuSize);
		_interactor.setGeometry(geometry());
	}

	void _onKeyboardEvent(spk::KeyboardEvent& p_event) override
	{
		switch (p_event.type)
		{
			case spk::KeyboardEvent::Type::Press:
			{
				if (p_event.key == spk::Keyboard::F12)
				{
					if (_inventory.parent() == nullptr)
					{
						addChild(&_inventory);
					}
				}
				
				break;
			}
		}
	}

public:
	MapEditorHUD(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_inventory(p_name + L" - Inventory", this),
		_interactor(p_name + L" - Interactor", this),
		_menu(p_name + L" - Menu", this)
	{
		_inventory.deactivateMenuButton(spk::IInterfaceWindow::MenuBar::Button::Maximize);
		_inventory.setMenuHeight(25);
		_inventory.setLayer(100);
		_inventory.activate();

		_interactor.setLayer(10);
		_interactor.activate();
		_interactor.setNodeSelector(_inventory.content()->nodeSelector());
		_interactor.setLevelSelector(_inventory.content()->levelSelector());

		_menu.setMenuHeight(25);
		_menu.deactivateMenuButton(spk::IInterfaceWindow::MenuBar::Button::Maximize);
		_menu.activate();

		_quitContract = _inventory.subscribeTo(spk::IInterfaceWindow::Event::Close, [&](){removeChild(&_inventory);});
	}
};

int main()
{
	spk::GraphicalApplication app = spk::GraphicalApplication();

	spk::SafePointer<spk::Window> win = app.createWindow(L"Playground", {{0, 0}, {800, 800}});

	Context::instanciate();
	
	MapEditorHUD mapEditorWidget = MapEditorHUD(L"Editor window", win->widget());
	mapEditorWidget.setGeometry(win->geometry());
	mapEditorWidget.activate();

	spk::GameEngineWidget gameEngineWidget = spk::GameEngineWidget(L"Engine widget", win->widget());
	gameEngineWidget.setGeometry(win->geometry());
	gameEngineWidget.setGameEngine(&(Context::instance()->gameEngine));
	gameEngineWidget.activate();
	
	return (app.run());
}