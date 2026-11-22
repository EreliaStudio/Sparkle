#pragma once

#include "design_pattern/contract_provider.hpp"
#include "engine/component3d.hpp"
#include "engine/registry.hpp"
#include "math/vector3.hpp"
#include "world/chunk_interest.hpp"

namespace spk::World
{
	class WorldObserver : public Component3D,
						  public Registry<WorldObserver, EngineHeadless *>::Object
	{
	public:
		using InterestEditionProvider = ContractProvider<const WorldObserver &>;
		using InterestEditionCallback = InterestEditionProvider::callback_type;
		using InterestEditionContract = InterestEditionProvider::Contract;

	private:
		ChunkInterest _interest;
		InterestEditionProvider _interestEditionProvider;

	public:
		explicit WorldObserver(ChunkInterest interest = ChunkInterest{});

		[[nodiscard]] const ChunkInterest &interest() const noexcept;
		void setInterest(ChunkInterest interest);

		[[nodiscard]] Vector3 worldPosition() const;
		[[nodiscard]] InterestEditionContract subscribeToInterestEdition(InterestEditionCallback callback);
	};
}
