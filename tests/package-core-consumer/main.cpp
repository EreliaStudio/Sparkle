#include <sparkle_core>

#include <chrono>

namespace
{
	class TickBehaviour final : public spk::Behaviour
	{
	public:
		int updates = 0;

	protected:
		void _updateState(spk::UpdateContext &) override
		{
			++updates;
		}
	};

	class TickSystem final : public spk::System
	{
	public:
		int updates = 0;

	protected:
		void _updateState(spk::UpdateContext &) override
		{
			++updates;
		}
	};
}

int main()
{
	spk::Engine engine;
	spk::Entity3D entity("Server entity");
	auto &behaviour = entity.addBehaviour<TickBehaviour>();
	auto &system = engine.addSystem<TickSystem>();
	engine.addEntity(&entity);

	entity.transform().place({1.0f, 2.0f, 3.0f});
	spk::UpdateContext context{
		.time = std::chrono::steady_clock::duration{},
		.deltaTime = std::chrono::milliseconds(16)};
	engine.updateState(context);

	return behaviour.updates == 1 &&
				   system.updates == 1 &&
				   entity.transform().position() == spk::Vector3(1.0f, 2.0f, 3.0f)
			   ? 0
			   : 1;
}
