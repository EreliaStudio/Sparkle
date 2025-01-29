#pragma once

#include "widget/spk_scalable_widget.hpp"   // <== Now inheriting from ScalableWidget
#include "widget/spk_text_label.hpp"
#include "widget/spk_frame.hpp"
#include "widget/spk_push_button.hpp"

namespace spk
{
	class InterfaceWindow : public spk::ScalableWidget
	{
	public:
		enum Event
		{
			Minimize,
			Maximize,
			Close
		};

	private:
		static std::vector<uint8_t> _defaultIconset_Data;
		static spk::SpriteSheet     _defaultIconset;
		static std::vector<uint8_t> _defaultNineSlice_Light_Data;
		static spk::SpriteSheet     _defaultNineSlice_Light;
		static std::vector<uint8_t> _defaultNineSlice_Dark_Data;
		static spk::SpriteSheet     _defaultNineSlice_Dark;
		static std::vector<uint8_t> _defaultNineSlice_Darker_Data;
		static spk::SpriteSheet     _defaultNineSlice_Darker;

		class MenuBar : public spk::Widget
		{
			friend class InterfaceWindow;

		private:
			spk::TextLabel  _titleLabel;
			spk::PushButton _minimizeButton;
			spk::PushButton _maximizeButton;
			spk::PushButton _closeButton;

			void _onGeometryChange();

		public:
			MenuBar(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent);
		};

		spk::Frame _backgroundFrame;
		spk::Frame _minimizedBackgroundFrame;
		MenuBar    _menuBar;
		spk::Frame _contentFrame;

		float _menuHeight = 20.0f;

		bool           _isMoving      = false;
		spk::Vector2Int _positionDelta; 

		spk::ContractProvider::Contract _minimizeContract;
		spk::ContractProvider::Contract _maximizeContract;

		bool          _isMaximized     = false;
		spk::Geometry2D _previousGeometry;

		void _onGeometryChange() override;
		void _onMouseEvent(spk::MouseEvent& p_event) override;

	public:
		InterfaceWindow(const std::wstring& p_name, const spk::SafePointer<spk::Widget>& p_parent);

		spk::SafePointer<spk::Widget> content();

		void setMenuHeight(const float& p_menuHeight);

		void setCornerSize(const spk::Vector2Int& p_cornerSize);

		void minimize();
		void maximize();
		void close();

		spk::ContractProvider::Contract subscribeTo(const InterfaceWindow::Event& p_event, const spk::ContractProvider::Job& p_job);
	};
}
