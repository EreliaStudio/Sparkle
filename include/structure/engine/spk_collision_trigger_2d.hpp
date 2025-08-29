#pragma once

#include "structure/design_pattern/spk_contract_provider.hpp"
#include "structure/engine/spk_component.hpp"
#include "structure/engine/spk_rigid_body_2d.hpp"
#include "structure/spk_safe_pointer.hpp"
#include <mutex>
#include <set>
#include <vector>

namespace spk
{
	class CollisionTrigger2D : public spk::Component
	{
	public:
		using TriggerEnterContract = spk::TContractProvider<spk::SafePointer<spk::Entity>>::Contract;
		using TriggerEnterJob = spk::TContractProvider<spk::SafePointer<spk::Entity>>::Job;

	private:
		bool _isTrigger = false;
		spk::TContractProvider<spk::SafePointer<spk::Entity>> _onTriggerEnterProvider;
		std::set<RigidBody2D *> _currentCollisions;
		std::set<RigidBody2D *> _frameCollisions;

		static inline std::vector<spk::SafePointer<CollisionTrigger2D>> _triggers;
		static inline std::mutex _triggerMutex;

	public:
		CollisionTrigger2D(const std::wstring &p_name);

		void setTrigger(const bool &p_state);
		bool isTrigger() const;

		TriggerEnterContract onTriggerEnter(const TriggerEnterJob &p_job);

		void start() override;
		void stop() override;

		void startFrame();
		void registerCollision(RigidBody2D *p_body);
		void endFrame();
		static std::vector<spk::SafePointer<CollisionTrigger2D>> getTriggers();
	};
}
