#pragma once

#include "widget/spk_frame.hpp"
#include "widget/spk_push_button.hpp"
#include "widget/spk_widget.hpp"

namespace spk
{
	class MenuBar : public spk::Widget
	{
	public:
		class Menu : public spk::Widget
		{
		public:
			class Item : public spk::PushButton
			{
			private:
			public:
				Item(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent);
			};

			class Break : public spk::Widget
			{
			private:
				static spk::SpriteSheet _defaultBreakSpriteSheet;

				uint32_t _height;
				spk::TextureRenderer _renderer;

				void _onGeometryChange();
				void _onPaintEvent(spk::PaintEvent &p_event);

			public:
				static spk::SafePointer<const spk::SpriteSheet> defaultBreakSpriteSheet();

				Break(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent);
				spk::Vector2UInt minimalSize() const override;
				void setSpriteSheet(const spk::SafePointer<const spk::SpriteSheet> &p_spriteSheet);
				void setHeight(const uint32_t &p_height);
			};

		private:
			struct Element
			{
				std::unique_ptr<spk::Widget> item;
				spk::PushButton::Contract itemContract;
			};

			bool _selected = false;
			spk::Frame _backgroundFrame;
			std::vector<Element> _elements;

			spk::Font::Size _computeElementSize() const;
			void _onGeometryChange() override;
			void _onMouseEvent(spk::MouseEvent &p_event);

		public:
			Menu(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent);

			size_t nbItem() const;

			spk::SafePointer<Item> addItem(const std::wstring &p_itemName, std::function<void()> p_callback);
			spk::SafePointer<Break> addBreak();

			spk::Vector2UInt minimalSize() const;
		};

	private:
		float _height = 25;
		spk::Frame _backgroundFrame;

		struct MenuEntry
		{
			std::unique_ptr<spk::PushButton> menuButton;
			spk::PushButton::Contract menuButtonContract;
			std::unique_ptr<Menu> Menu;
		};

		std::vector<std::unique_ptr<MenuEntry>> _menus;

		spk::Font::Size _computeFontSize() const;
		void _onGeometryChange() override;

	public:
		MenuBar(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent);

		void setHeight(const float &p_height);
		const float &height() const;

		spk::SafePointer<Menu> addMenu(const std::wstring &p_menuName);
	};
}