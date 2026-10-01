#include <sparkle>

#include <chrono>

namespace
{
	class DeviceSystem final : public spk::RenderingSystem
	{
	public:
		int updates = 0;

	protected:
		void _updateState(
			spk::UpdateContext &,
			spk::DeviceContext &) override
		{
			++updates;
		}
	};
}

int main()
{
	spk::Engine engine;
	auto &system = engine.addSystem<DeviceSystem>();
	spk::RenderingEngine renderingEngine(&engine);
	spk::UpdateContext updateContext{
		.time = std::chrono::steady_clock::duration{},
		.deltaTime = std::chrono::milliseconds(16)};
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	spk::DeviceContext deviceContext{keyboard, mouse};

	renderingEngine.updateState(updateContext, deviceContext);
	return system.updates == 1 ? 0 : 1;
}
