#pragma once

#include <cstdint>
#include <string>

#include "design_pattern/contract_provider.hpp"
#include "engine/component3d.hpp"
#include "engine/registry.hpp"
#include "graphics/color.hpp"
#include "math/vector3.hpp"

namespace spk::Light
{
	class Contributor : public Component3D,
						public Registry<Contributor, EngineHeadless *>::Object
	{
	public:
		using OnEditionContractProvider = ContractProvider<const Contributor &>;
		using OnEditionContract = OnEditionContractProvider::Contract;
		using OnEditionCallback = OnEditionContractProvider::callback_type;

	private:
		friend class spk::EngineHeadless;

		RGB _color{1.0f, 1.0f, 1.0f};
		float _intensity = 1.0f;
		float _priorityWeight = 1.0f;
		bool _castsShadows = false;
		std::uint32_t _shadowResolution = 1024;
		std::uint64_t _registrationOrder = 0;
		OnEditionContractProvider _onEditionContractProvider;
		ContextualizableTrait<EngineHeadless *>::OnContextEditionContract _onContextEditionContract;

		void _setRegistrationOrder(std::uint64_t order) noexcept;

	protected:
		Contributor(const std::string &name, bool castShadows);
		void _notifyEdition();

		// All direct lights use the owning Entity3D world transform, including parent transforms.
		[[nodiscard]] Vector3 _worldPosition() const;
		[[nodiscard]] Vector3 _worldForward() const;

	public:
		~Contributor() override = default;

		[[nodiscard]] RGB color() const noexcept;
		void setColor(RGB color);
		[[nodiscard]] float intensity() const noexcept;
		void setIntensity(float intensity);
		[[nodiscard]] float priorityWeight() const noexcept;
		void setPriorityWeight(float priorityWeight);
		[[nodiscard]] bool castsShadows() const noexcept;
		void setCastsShadows(bool enabled);
		// Side length of a square D32F shadow map; actual GPU allocation belongs to later PRs.
		[[nodiscard]] std::uint32_t shadowResolution() const noexcept;
		void setShadowResolution(std::uint32_t resolution);
		// Zero denotes no current engine registration.
		[[nodiscard]] std::uint64_t registrationOrder() const noexcept;
		[[nodiscard]] OnEditionContract subscribeToEdition(OnEditionCallback callback);
	};
}
