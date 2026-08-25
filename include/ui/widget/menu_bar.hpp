#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "graphics/sprite_sheet.hpp"
#include "ui/widget/panel.hpp"
#include "ui/widget/push_button.hpp"

namespace spk
{
	class MenuBar : public Widget
	{
	public:
		class Menu : public Widget
		{
		public:
			class Item : public PushButton
			{
			private:
				Menu &_owner;
				ClickContract _actionContract;

			public:
				Item(std::string name, std::string_view label, ClickCallback callback, Menu &owner);
			};

			class Break : public Widget
			{
			private:
				const SpriteSheet *_spriteSheet = nullptr;
				unsigned int _height = 2;
				float _depth = 0.0f;

				void _updateSizeHint() override;
				void _buildRenderSnapshot(RenderSnapshot::Builder &builder) override;

			public:
				Break(std::string name, const SpriteSheet *spriteSheet, Widget *parent = nullptr);

				void setSpriteSheet(const SpriteSheet *spriteSheet);
				void setHeight(unsigned int height);
				void setDepth(float depth);
				[[nodiscard]] const SpriteSheet *spriteSheet() const noexcept;
				[[nodiscard]] unsigned int height() const noexcept;
				[[nodiscard]] float depth() const noexcept;
			};

		private:
			bool _layoutReady = false;
			Panel _background;
			std::vector<std::unique_ptr<Widget>> _elements;
			unsigned int _elementSpacing = 5;

			void _updateSizeHint() override;
			void _onGeometryChange() override;
			void _onMouseButtonPressedEvent(MouseButtonPressedEvent &event) override;

		public:
			explicit Menu(std::string name, Widget *parent = nullptr);

			Item &addItem(std::string name, std::string_view label, PushButton::ClickCallback callback = {});
			Break &addBreak(std::string name, const SpriteSheet *spriteSheet);
			void clear();
			[[nodiscard]] std::size_t nbElement() const noexcept;
			void setElementSpacing(unsigned int spacing);
			[[nodiscard]] unsigned int elementSpacing() const noexcept;
			[[nodiscard]] Panel &background() noexcept;
			[[nodiscard]] const Panel &background() const noexcept;
		};

	private:
		struct Entry
		{
			std::unique_ptr<PushButton> button;
			std::unique_ptr<Menu> menu;
			PushButton::ClickContract toggleContract;
		};

		bool _layoutReady = false;
		Panel _background;
		std::vector<std::unique_ptr<Entry>> _entries;
		std::unordered_map<std::string, Entry *> _entriesByName;
		unsigned int _height = 25;
		unsigned int _contentInset = 2;
		unsigned int _menuSpacing = 5;

		[[nodiscard]] unsigned int _effectiveHeight() const noexcept;
		void _closeMenus(Menu *exception = nullptr);
		void _configureButton(PushButton &button);
		void _updateSizeHint() override;
		void _onGeometryChange() override;

	public:
		explicit MenuBar(std::string name, Widget *parent = nullptr);

		Menu &addMenu(std::string name, std::string_view label = {});
		Menu &menu(std::string_view name);
		[[nodiscard]] const Menu &menu(std::string_view name) const;
		PushButton &button(std::string_view name);
		[[nodiscard]] const PushButton &button(std::string_view name) const;
		[[nodiscard]] std::size_t nbMenu() const noexcept;
		void closeMenus();
		void setHeight(unsigned int height);
		[[nodiscard]] unsigned int height() const noexcept;
		void setContentInset(unsigned int inset);
		[[nodiscard]] unsigned int contentInset() const noexcept;
		void setMenuSpacing(unsigned int spacing);
		[[nodiscard]] unsigned int menuSpacing() const noexcept;
		[[nodiscard]] Panel &background() noexcept;
		[[nodiscard]] const Panel &background() const noexcept;
	};
}
