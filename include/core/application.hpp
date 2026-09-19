#pragma once

#include <cstdlib>
#include <memory>

#include "core/window.hpp"

namespace spk
{
	class Application
	{
	private:
		struct Channels;

		template <typename TType>
		class Runtime;

		class PlatformRuntime;
		class UpdateRuntime;
		class RenderRuntime;
		class Impl;

		std::unique_ptr<Impl> _impl;

	public:
		Application();
		Application(const Application &) = delete;
		Application(Application &&) = delete;
		~Application();

		Application &operator=(const Application &) = delete;
		Application &operator=(Application &&) = delete;

		[[nodiscard]] Window &window(const Window::Identifier &identifier);
		[[nodiscard]] const Window &window(const Window::Identifier &identifier) const;
		Window &createWindow(const Window::Identifier &identifier, const Window::Configuration &configuration);
		void closeWindow(const Window::Identifier &identifier);
		void quit(int exitCode = EXIT_SUCCESS);
		int run();
	};
}
