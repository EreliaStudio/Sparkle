#pragma once

#include <concepts>
#include <memory>
#include <utility>

#include "engine/entity.hpp"
#include "engine/system_collection.hpp"
#include "math/rect2d.hpp"

namespace spk
{
	struct UpdateContext;

	class Engine : public SystemCollection
	{
	private:
		using SystemCollection::registerSystem;
		using SystemCollection::unregisterSystem;

		Entity _root;
		spk::Rect2D _geometry{};

	public:
		Engine();
		~Engine();

		void addEntity(Entity *entity);
		void removeEntity(Entity *entity);

		template <typename TSystemType, typename... TArgs>
			requires std::derived_from<TSystemType, System>
		TSystemType &addSystem(TArgs &&...args)
		{
			std::unique_ptr<TSystemType> system =
				std::make_unique<TSystemType>(std::forward<TArgs>(args)...);

			TSystemType &result = *system;
			result.attach(this);
			registerSystem(std::move(system));

			return result;
		}

		void removeSystem(System &system);

		[[nodiscard]] Entity &root() noexcept;
		[[nodiscard]] const Entity &root() const noexcept;

		void handleGeometryChange(const spk::Rect2D &geometry);
		[[nodiscard]] const spk::Rect2D &geometry() const noexcept;
		void updateState(UpdateContext &context);
	};
}
