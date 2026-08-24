#pragma once

#include <string>

#include "ui/widget.hpp"

namespace spk
{
	class Screen : public Widget
	{
	private:
		static Screen *_activeScreen;

		ActivationContract _activationContract;

	public:
		explicit Screen(std::string name, Widget *parent = nullptr);
		~Screen() override;

		[[nodiscard]] static Screen *activeScreen() noexcept;
	};
}
