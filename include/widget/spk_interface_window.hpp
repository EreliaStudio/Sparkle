#pragma once

#include "widget/spk_frame.hpp"
#include "widget/spk_push_button.hpp"
#include "widget/spk_scalable_widget.hpp" // <== Now inheriting from ScalableWidget
#include "widget/spk_text_label.hpp"

namespace spk
{
	class IInterfaceWindow : public spk::ScalableWidget
	{
	public:
		using ResizeContractProvider = spk::TContractProvider<const spk::Vector2UInt&>;

		enum Event
		{
			Minimize,
			Maximize,
			Close
		};

		class MenuBar : public spk::Widget
		{
			friend class IInterfaceWindow;

		public:
			enum class Button
			{
				Minimize,
				Maximize,
				Close
			};

		private:
			spk::TextLabel _titleLabel;
			spk::PushButton _minimizeButton;
			spk::PushButton _maximizeButton;
			spk::PushButton _closeButton;

			spk::Font::Size _computeFontSize(const float &p_menuHeight);
			spk::Vector2Int _computeControlButtonSize(const float &p_menuHeight);

			void _onGeometryChange();

			spk::Vector2UInt _computeMinimalSize(const float &p_menuHeight);

			void _activateMenuButton(const MenuBar::Button &p_button);
			void _deactivateMenuButton(const MenuBar::Button &p_button);

		public:
			MenuBar(const std::wstring &p_name, const spk::SafePointer<spk::Widget> &p_parent);

			spk::SafePointer<spk::TextLabel> titleLabel();
			spk::SafePointer<spk::PushButton> minimizeButton();
			spk::SafePointer<spk::PushButton> maximizeButton();
			spk::SafePointer<spk::PushButton> closeButton();
		};

	private:
		static spk::SpriteSheet _defaultNineSlice_Light;
		static spk::SpriteSheet _defaultNineSlice_Dark;
		static spk::SpriteSheet _defaultNineSlice_Darker;

		spk::Frame _backgroundFrame;
		spk::Frame _minimizedBackgroundFrame;
		MenuBar _menuBar;
		spk::SafePointer<Widget> _content;

		float _menuHeight = 20.0f;

		bool _isMoving = false;
		spk::Vector2Int _positionDelta;

		ResizeContractProvider _onResizeContractProvider;

		spk::ContractProvider::Contract _minimizeContract;
		spk::ContractProvider::Contract _maximizeContract;

		bool _isMaximized = false;
		spk::Geometry2D _previousGeometry;

	protected:
		void _onGeometryChange() override;
		void _onMouseEvent(spk::MouseEvent &p_event) override;

	private:
		using spk::ScalableWidget::setMaximumSize;
		using spk::ScalableWidget::setMinimumSize;

	public:
		IInterfaceWindow(const std::wstring &p_name, const spk::SafePointer<spk::Widget> &p_parent);

		ResizeContractProvider::Contract subscribeOnResize(const ResizeContractProvider::Job& p_job);

		spk::SafePointer<MenuBar> menuBar();

		spk::SafePointer<Frame> backgroundFrame();
		void setContent(spk::SafePointer<Widget> p_content);

		void setMinimumContentSize(const spk::Vector2UInt &p_minimumContentSize);
		void setMaximumContentSize(const spk::Vector2UInt &p_maximumContentSize);

		void setMenuHeight(const float &p_menuHeight);

		void setCornerSize(const spk::Vector2Int &p_cornerSize);

		void minimize();
		void maximize();
		void close();

		void activateMenuButton(const MenuBar::Button &p_button);
		void deactivateMenuButton(const MenuBar::Button &p_button);

		spk::ContractProvider::Contract subscribeTo(const IInterfaceWindow::Event &p_event, const spk::ContractProvider::Job &p_job);
	};

	template <typename TContentType, typename = std::enable_if_t<std::is_base_of<Widget, TContentType>::value>>
	class InterfaceWindow : public spk::IInterfaceWindow
	{
	private:
		TContentType _content;

		using spk::IInterfaceWindow::setContent;

	public:
		InterfaceWindow(const std::wstring &p_name, const spk::SafePointer<spk::Widget> &p_parent) :
			spk::IInterfaceWindow(p_name, p_parent),
			_content(p_name + L" - Content", backgroundFrame())
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
