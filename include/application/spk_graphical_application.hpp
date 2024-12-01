#pragma once

#include "application/spk_application.hpp"

#include "structure/spk_safe_pointer.hpp"
#include "structure/graphics/spk_window.hpp"

namespace spk
{
	class GraphicalApplication : public Application
	{
	private:
		spk::ThreadSafeQueue<spk::SafePointer<Window>> _windowToRemove;
		std::map<std::wstring, std::unique_ptr<spk::Window>> _windows;
		

	public:
		GraphicalApplication();

		spk::SafePointer<Window> createWindow(const std::wstring& p_title, const spk::Geometry2D& p_geometry);
		void closeWindow(spk::SafePointer<Window> p_windowToClose);
	};
}