#pragma once

#include "widget/spk_scalable_widget.hpp"   // <== Now inheriting from ScalableWidget
#include "widget/spk_text_label.hpp"
#include "widget/spk_frame.hpp"
#include "widget/spk_push_button.hpp"

namespace spk
{
	class IInterfaceWindow : public spk::ScalableWidget
	{
	public:
		enum Event
		{
			Minimize,
			Maximize,
			Close
		};

	private:
		static spk::SpriteSheet     _defaultNineSlice_Light;
		static spk::SpriteSheet     _defaultNineSlice_Dark;
		static spk::SpriteSheet     _defaultNineSlice_Darker;

		class MenuBar : public spk::Widget
		{
			friend class IInterfaceWindow;

		private:
			spk::TextLabel  _titleLabel;
			spk::PushButton _minimizeButton;
			spk::PushButton _maximizeButton;
			spk::PushButton _closeButton;

			spk::Font::Size _computeFontSize(const float& p_menuHeight);
			spk::Vector2Int _computeControlButtonSize(const float& p_menuHeight);

			void _onGeometryChange();
			
			spk::Vector2UInt _computeMinimalSize(const float& p_menuHeight);

		public:
			MenuBar(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent);
		};

		spk::Frame _backgroundFrame;
		spk::Frame _minimizedBackgroundFrame;
		MenuBar    _menuBar;
		spk::SafePointer<Widget> _content;

		float _menuHeight = 20.0f;

		bool           _isMoving      = false;
		spk::Vector2Int _positionDelta; 

		spk::ContractProvider::Contract _minimizeContract;
		spk::ContractProvider::Contract _maximizeContract;

		bool          _isMaximized     = false;
		spk::Geometry2D _previousGeometry;

		void _onGeometryChange() override;
		void _onMouseEvent(spk::MouseEvent& p_event) override;

		using spk::ScalableWidget::setMinimumSize;
		using spk::ScalableWidget::setMaximumSize;

	public:
		IInterfaceWindow(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent);

		void setContent(spk::SafePointer<Widget> p_content);

		void setMinimumContentSize(const spk::Vector2UInt& p_minimumContentSize);
		void setMaximumContentSize(const spk::Vector2UInt& p_maximumContentSize);

		void setMenuHeight(const float& p_menuHeight);

		void setCornerSize(const spk::Vector2Int& p_cornerSize);

		void minimize();
		void maximize();
		void close();

		spk::ContractProvider::Contract subscribeTo(const IInterfaceWindow::Event& p_event, const spk::ContractProvider::Job& p_job);
	};

	template<typename TContentType,
	         typename = std::enable_if_t<std::is_base_of<Widget, TContentType>::value>>
	class InterfaceWindow : public spk::IInterfaceWindow
	{
	private:
		TContentType _content;

		using spk::IInterfaceWindow::setContent;

	public:
		InterfaceWindow(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent) :
			spk::IInterfaceWindow(p_name, p_parent),
			_content(p_name + L" - Content", this)
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
