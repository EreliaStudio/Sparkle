#pragma once

namespace spk::Clipboard::Internal
{
	using OpenClipboardFunction = bool (*)();

	void setOpenClipboardFunctionForTesting(OpenClipboardFunction function) noexcept;
}
