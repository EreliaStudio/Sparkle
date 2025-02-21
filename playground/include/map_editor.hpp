#pragma once

#include "sparkle.hpp"

#include "event.hpp"
#include "texture_manager.hpp"
#include "context.hpp"

class LevelSelector : public spk::Widget
{
private:
    spk::PushButton _pushButtons[5];
    spk::PushButton::Contract _pushButtonContract[5];
    int _level = 0;

    void _onGeometryChange() override;

public:
    LevelSelector(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent);

    int level() const;
};

class NodeSelector : public spk::ScrollableWidget
{
private:
    spk::Vector2UInt _nbElement = {0, 0};
    spk::Vector2UInt _elementSize = {32, 32};

    int _nodeIndex = -1;
    bool _texturePrepared = false;
    spk::Vector2Int _selectedNodePosition;

    static const std::unordered_map<spk::Vector2Int, int> _nodeToTextureIDMap;

    spk::TextureRenderer _contentRenderer;
    spk::TextureRenderer _selectedIconRenderer;
    spk::OpenGL::FrameBufferObject _frameBufferObject;

    void _computeNbElement();
    void _prepareTexture();
    void _onGeometryChange() override;
    void _onPaintEvent(spk::PaintEvent& p_event) override;
    void _onMouseEvent(spk::MouseEvent& p_event) override;

public:
    NodeSelector(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent);

    int selectedNode() const;
    spk::Vector2UInt requiredSize() const;
};

class MapEditorInventory : public spk::Widget
{
private:
    LevelSelector _levelSelector;
    spk::ScrollArea<NodeSelector> _nodeSelector;

    spk::Font::Size defaultFontSize();
    void _onGeometryChange() override;

public:
    MapEditorInventory(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent);

    spk::SafePointer<LevelSelector> levelSelector();
    spk::SafePointer<NodeSelector> nodeSelector();
    spk::Vector2UInt minimalSize() const;
};

class MapEditorInteractor : public spk::Widget
{
private:
    bool _isPlacing;
    spk::SafePointer<NodeSelector> _nodeSelector;
    spk::SafePointer<LevelSelector> _levelSelector;

    void _onMouseEvent(spk::MouseEvent& p_event) override;

public:
    MapEditorInteractor(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent);

    void setNodeSelector(spk::SafePointer<NodeSelector> p_nodeSelector);
    void setLevelSelector(spk::SafePointer<LevelSelector> p_levelSelector);
};

class MapEditorMenu : public spk::Widget
{
private:
    spk::PushButton _saveButton;
    spk::PushButton::Contract _saveContract;
    spk::PushButton _loadButton;
    spk::PushButton::Contract _loadContract;

    spk::Vector2UInt buttonSize() const;
    void _onGeometryChange() override;

public:
    MapEditorMenu(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent);

    spk::Vector2UInt minimalSize() const;
    spk::Vector2UInt maximalSize() const;
};

class MapEditor : public spk::Widget
{
private:
    spk::GameEngineWidget _gameEngineWidget;
    spk::InterfaceWindow<MapEditorMenu> _menu;
    spk::InterfaceWindow<MapEditorInventory> _inventory;
    MapEditorInteractor _interactor;

    spk::ContractProvider::Contract _quitContract;

    void _onGeometryChange() override;
    void _onKeyboardEvent(spk::KeyboardEvent& p_event) override;

public:
    MapEditor(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent);
};
