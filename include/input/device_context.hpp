#pragma once

namespace spk
{
	struct Keyboard;
	struct Mouse;

	struct DeviceContext
	{
		const Keyboard *keyboard = nullptr;
		const Mouse *mouse = nullptr;
	};
}
