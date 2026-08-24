#pragma once

#include <Windows.h>

#include <cstdint>
#include <exception>
#include <functional>
#include <string>
#include <string_view>

#include "math/rect2d.hpp"

namespace spk::WinAPI
{
	class Window final
	{
	public:
		using MessageHandler = std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>;

		struct CreationInfo
		{
			std::string title;
			int x = 0;
			int y = 0;
			std::uint32_t width = 1280;
			std::uint32_t height = 720;
			bool visible = true;
			MessageHandler messageHandler;
		};

		class Class final
		{
			friend class Window;

		private:
			HINSTANCE _instance = nullptr;
			std::wstring _identifier;
			ATOM _atom = 0;

			void _register();

		public:
			explicit Class(std::string identifier);
			Class(const Class &) = delete;
			Class(Class &&) = delete;
			~Class();

			Class &operator=(const Class &) = delete;
			Class &operator=(Class &&) = delete;
		};

	private:
		static constexpr DWORD Style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

		HWND _handle = nullptr;
		bool _closureRequested = false;
		bool _closureDispatched = false;
		MessageHandler _messageHandler;
		std::exception_ptr _pendingException = nullptr;

		[[noreturn]] static void _throwLastError(std::string_view operation);
		[[nodiscard]] static int _wideSize(std::string_view value);
		static void _writeWide(std::string_view value, std::wstring &result);
		[[nodiscard]] static std::wstring _toWide(std::string_view value);
		[[nodiscard]] static SIZE _outerSize(const CreationInfo &info);
		[[nodiscard]] static Window *_instance(HWND handle) noexcept;
		[[nodiscard]] static Window *_bind(HWND handle, LPARAM parameter) noexcept;
		LRESULT _process(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) noexcept;
		LRESULT _processDestruction(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) noexcept;
		static LRESULT CALLBACK _procedure(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) noexcept;
		void _createHandle(const Class &windowClass, const CreationInfo &info, SIZE size);

	public:
		Window() = default;
		Window(const Window &) = delete;
		Window(Window &&) = delete;
		~Window() = default;

		Window &operator=(const Window &) = delete;
		Window &operator=(Window &&) = delete;

		void create(const Class &windowClass, const CreationInfo &info);
		void destroy();
		[[nodiscard]] bool consumeClosureRequest() noexcept;
		[[nodiscard]] HWND handle() const noexcept;
		void rethrowPendingException();

		[[nodiscard]] spk::Rect2D geometry() const;
	};
}
