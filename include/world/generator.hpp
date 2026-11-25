#pragma once

#include <concepts>
#include <exception>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "exception.hpp"
#include "world/chunk_coordinate.hpp"

namespace spk::World
{
	template <typename TChunk, typename TGenerationData>
	class Generator
	{
	public:
		class Pass
		{
		public:
			virtual ~Pass() = default;
			virtual void apply(const ChunkCoordinate &coordinate, TGenerationData &data) = 0;
		};

	private:
		TGenerationData _generationData;
		std::vector<std::unique_ptr<Pass>> _passes;

	protected:
		template <typename... TArgs>
		explicit Generator(TArgs &&...args) :
			_generationData(std::forward<TArgs>(args)...)
		{
		}

		// A rejected or throwing preparation must leave no request-local state.
		virtual bool _prepareGenerationData(const ChunkCoordinate &, TGenerationData &)
		{
			return true;
		}
		virtual TChunk _instantiateChunk(const ChunkCoordinate &coordinate, TGenerationData &data) = 0;
		virtual void _cleanGenerationData(const ChunkCoordinate &, TGenerationData &) noexcept
		{
		}

	public:
		virtual ~Generator() = default;

		[[nodiscard]] TGenerationData &generationData() noexcept
		{
			return _generationData;
		}
		[[nodiscard]] const TGenerationData &generationData() const noexcept
		{
			return _generationData;
		}

		// Configure passes before submitting work; the generator must outlive its jobs.
		// The data and passes are responsible for synchronization.
		template <typename TPass, typename... TArgs>
			requires std::derived_from<TPass, Pass>
		TPass &addPass(TArgs &&...args)
		{
			auto pass = std::make_unique<TPass>(std::forward<TArgs>(args)...);
			TPass &result = *pass;
			_passes.push_back(std::move(pass));
			return result;
		}

		[[nodiscard]] std::optional<TChunk> generate(const ChunkCoordinate &coordinate)
		{
			try
			{
				if (!_prepareGenerationData(coordinate, _generationData))
				{
					return std::nullopt;
				}
				std::optional<TChunk> result;
				try
				{
					for (const auto &pass : _passes)
					{
						pass->apply(coordinate, _generationData);
					}
					result.emplace(_instantiateChunk(coordinate, _generationData));
				} catch (...)
				{
					_cleanGenerationData(coordinate, _generationData);
					throw;
				}
				_cleanGenerationData(coordinate, _generationData);
				return result;
			} catch (const spk::Exception &)
			{
				throw;
			} catch (...)
			{
				throw spk::Exception("Chunk generation failed", std::current_exception());
			}
		}
	};
}
