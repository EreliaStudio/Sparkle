#include "application.hpp"

#include "internal/application_internal.hpp"

namespace spk
{
	Application::Application() : _impl(std::make_unique<Impl>()) {}
	Application::~Application() = default;

	Window &Application::window(const Window::Identifier &identifier)
	{
		return _impl->window(identifier);
	}

	const Window &Application::window(const Window::Identifier &identifier) const
	{
		return _impl->window(identifier);
	}

	Window &Application::createWindow(const Window::Identifier &identifier, const Window::Configuration &configuration)
	{
		return _impl->createWindow(identifier, configuration);
	}

	void Application::closeWindow(const Window::Identifier &identifier)
	{
		_impl->closeWindow(identifier);
	}

	void Application::quit(int exitCode)
	{
		_impl->quit(exitCode);
	}

	int Application::run()
	{
		return _impl->run();
	}
}
