#include "engine/engine_headless.hpp"

#include "core/context/update_context.hpp"
#include "light/contributor.hpp"

#include <mutex>

namespace spk
{
	EngineHeadless::EngineHeadless() :
		_root("EngineHeadless root")
	{
		_lightAdditionContract = Registry<Light::Contributor, EngineHeadless *>::instance().subscribeToAddition(
			this,
			[this](EngineHeadless *const &, Light::Contributor *light) {
				const std::scoped_lock lock(_lightRegistrationMutex);
				const std::uint64_t nextOrder = Light::detail::nextRegistrationOrder(_lastLightRegistrationOrder);
				light->_setRegistrationOrder(nextOrder);
				_lastLightRegistrationOrder = nextOrder;
			});
		_root.changeContext(this);
	}

	EngineHeadless::~EngineHeadless()
	{
		while (!_root.children().empty())
		{
			_root.children().back()->clearParent();
		}

		Registry<Entity, EngineHeadless *> &entityRegistry = Registry<Entity, EngineHeadless *>::instance();
		while (!entityRegistry.elements(this).empty())
		{
			(*entityRegistry.elements(this).begin())->changeContext(nullptr);
		}

		while (!systems().empty())
		{
			removeSystem(*systems().back());
		}
	}

	void EngineHeadless::addEntity(Entity *entity)
	{
		if (entity == nullptr)
		{
			return;
		}

		entity->changeContext(this);
		entity->setParent(_root);
		entity->setGeometry(_geometry);
	}

	void EngineHeadless::removeEntity(Entity *entity)
	{
		if (entity == nullptr)
		{
			return;
		}

		entity->clearParent();
		entity->changeContext(nullptr);
	}

	void EngineHeadless::removeSystem(System &system)
	{
		system.attach(nullptr);
		unregisterSystem(system);
	}

	Entity &EngineHeadless::root() noexcept
	{
		return _root;
	}

	const Entity &EngineHeadless::root() const noexcept
	{
		return _root;
	}

	void EngineHeadless::handleGeometryChange(const spk::Rect2D &geometry)
	{
		_geometry = geometry;
		_root.setGeometry(_geometry);
	}

	const spk::Rect2D &EngineHeadless::geometry() const noexcept
	{
		return _geometry;
	}

	void EngineHeadless::updateState(UpdateContext &context)
	{
		_root.updateState(context);

		const auto systemSnapshot = SystemCollection::snapshotElements();
		for (const auto &snapshot : systemSnapshot)
		{
			if (SystemCollection::containsSnapshotElement(snapshot))
			{
				snapshot.element->updateState(context);
			}
		}
	}
}
