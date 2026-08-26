#pragma once

#include <memory>
#include <string>

#include "graphics/color.hpp"
#include "math/rect2d.hpp"
#include "type/focus_mode.hpp"

#include "graphics/opengl/gpu_resource.hpp"
#include "graphics/opengl/gpu_resource_collection.hpp"

namespace spk
{
	class Application;
	class Widget;
	struct Keyboard;
	struct Mouse;

	namespace WinAPI
	{
		class Window;
	}

	class Window
	{
	public:
		using Identifier = std::string;

		enum class LifeCycle
		{
			Pending,
			Ready,
			Releasing,
			Released
		};

		struct Configuration
		{
			std::string title;
			spk::Rect2D area;
			spk::Color backgroundColor{
				.r = 0.05f,
				.g = 0.05f,
				.b = 0.08f,
				.a = 1.0f};
		};

		class Native
		{
		private:
			struct Impl;
			std::unique_ptr<Impl> _impl;

		public:
			explicit Native(const Window::Identifier &windowID);
			Native(const Native &) = delete;
			Native(Native &&) = delete;
			~Native();

			Native &operator=(const Native &) = delete;
			Native &operator=(Native &&) = delete;

			[[nodiscard]] LifeCycle lifeCycle() const noexcept;
			[[nodiscard]] WinAPI::Window &window() noexcept;
			[[nodiscard]] const WinAPI::Window &window() const noexcept;

			void markReady() noexcept;
			void beginRelease() noexcept;
			void markReleased() noexcept;
		};

		class State
		{
		private:
			struct Impl;
			std::unique_ptr<Impl> _impl;

		public:
			explicit State(const Window::Identifier &windowID);
			State(const State &) = delete;
			State(State &&) = delete;
			~State();

			State &operator=(const State &) = delete;
			State &operator=(State &&) = delete;

			[[nodiscard]] LifeCycle lifeCycle() const noexcept;
			[[nodiscard]] Widget *focusedWidget(FocusMode::Channel channel) noexcept;
			[[nodiscard]] const Widget *focusedWidget(FocusMode::Channel channel) const noexcept;

			void setBackgroundColor(const spk::Color &backgroundColor);

			void takeFocus(FocusMode::Channel channel, Widget *widget) noexcept;
			void releaseFocus(FocusMode::Channel channel, Widget *widget) noexcept;
			void clearFocus(FocusMode::Channel channel) noexcept;
			[[nodiscard]] Widget &dispatchRoot(FocusMode::Channel channel) noexcept;

			void markReady() noexcept;
			void beginRelease() noexcept;
			void markReleased() noexcept;

			[[nodiscard]] Widget &root() noexcept;
			[[nodiscard]] const Widget &root() const noexcept;
			[[nodiscard]] spk::Keyboard &keyboard() noexcept;
			[[nodiscard]] const spk::Keyboard &keyboard() const noexcept;
			[[nodiscard]] spk::Mouse &mouse() noexcept;
			[[nodiscard]] const spk::Mouse &mouse() const noexcept;
		};

		class Surface
		{
			friend class GPUResource;

		private:
			struct Impl;
			std::unique_ptr<Impl> _impl;

		public:
			explicit Surface(const Window::Identifier &windowID);
			Surface(const Surface &) = delete;
			Surface(Surface &&) = delete;
			~Surface();

			Surface &operator=(const Surface &) = delete;
			Surface &operator=(Surface &&) = delete;

			[[nodiscard]] LifeCycle lifeCycle() const noexcept;
			void create(const WinAPI::Window &window);
			void destroy();
			void makeCurrent();
			void present();

			void setGeometry(const spk::Rect2D &geometry) noexcept;
			[[nodiscard]] const spk::Rect2D &geometry() const noexcept;

			[[nodiscard]] GPUResourceCollection &_gpuResources();
		};

	private:
		std::shared_ptr<Native> _native;
		std::shared_ptr<State> _state;
		std::shared_ptr<Surface> _surface;

	public:
		Window(std::shared_ptr<Native> native, std::shared_ptr<State> state, std::shared_ptr<Surface> surface);

		[[nodiscard]] bool isClosing() const noexcept;
		[[nodiscard]] bool isClosed() const noexcept;
		[[nodiscard]] Widget &root() noexcept;
		[[nodiscard]] const Widget &root() const noexcept;
		[[nodiscard]] const spk::Rect2D &geometry() const noexcept;
	};
}
