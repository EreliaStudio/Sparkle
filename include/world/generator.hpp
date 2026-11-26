#pragma once

#include <concepts>
#include <cstddef>
#include <exception>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "exception.hpp"
#include "world/chunk_coordinate.hpp"

namespace spk::World
{
	template <typename T>
	concept GenerationData = requires(T &data) {
		{ data.flush() } -> std::same_as<void>;
	};

	template <typename TChunk, GenerationData TGenerationData>
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
		virtual void _applyPass(Pass &pass, const ChunkCoordinate &coordinate, TGenerationData &data)
		{
			pass.apply(coordinate, data);
		}
		virtual TChunk _instantiateChunk(const ChunkCoordinate &coordinate, TGenerationData &data) = 0;
		virtual void _cleanGenerationData(const ChunkCoordinate &, TGenerationData &) noexcept
		{
		}

	public:
		virtual ~Generator() = default;

		void flush()
		{
			_generationData.flush();
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

		// Insert before the current pass at index; index == size appends.
		template <typename TPass, typename... TArgs>
			requires std::derived_from<TPass, Pass>
		TPass &insertPass(std::size_t index, TArgs &&...args)
		{
			if (index > _passes.size())
			{
				throw spk::Exception("Generator pass insertion index is out of range");
			}
			auto pass = std::make_unique<TPass>(std::forward<TArgs>(args)...);
			TPass &result = *pass;
			_passes.insert(_passes.begin() + static_cast<std::ptrdiff_t>(index), std::move(pass));
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
						_applyPass(*pass, coordinate, _generationData);
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
