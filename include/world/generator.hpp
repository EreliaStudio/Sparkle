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
	template <typename TChunk, typename TGenerationData>
	class Generator
	{
	public:
		class Pass
		{
		public:
			virtual ~Pass() = default;

			virtual void retain(const ChunkCoordinate &)
			{
			}
			virtual void release(const ChunkCoordinate &) noexcept
			{
			}
			virtual void beginJob(const ChunkCoordinate &)
			{
			}
			virtual void endJob(const ChunkCoordinate &) noexcept
			{
			}

			virtual void apply(
				const ChunkCoordinate &coordinate,
				TGenerationData &data) = 0;
		};

	private:
		std::vector<std::unique_ptr<Pass>> _passes;

	protected:
		virtual std::optional<TGenerationData> _createGenerationData(const ChunkCoordinate &coordinate) = 0;
		virtual TChunk _instantiateChunk(TGenerationData &data) = 0;

	public:
		virtual ~Generator() = default;

		// Configure passes before submitting work; the generator must outlive its jobs.
		void retain(const ChunkCoordinate &coordinate)
		{
			std::size_t count = 0;
			try
			{
				for (; count < _passes.size(); ++count)
				{
					_passes[count]->retain(coordinate);
				}
			} catch (...)
			{
				while (count != 0)
				{
					_passes[--count]->release(coordinate);
				}
				throw;
			}
		}

		void release(const ChunkCoordinate &coordinate) noexcept
		{
			for (auto &pass : _passes)
			{
				pass->release(coordinate);
			}
		}

		class JobLease
		{
		private:
			Generator *_generator;
			ChunkCoordinate _coordinate;
			std::size_t _count = 0;
			friend class Generator;

			JobLease(Generator &generator, const ChunkCoordinate &coordinate) :
				_generator(&generator),
				_coordinate(coordinate)
			{
				try
				{
					for (; _count < generator._passes.size(); ++_count)
					{
						generator._passes[_count]->beginJob(coordinate);
					}
				} catch (...)
				{
					_release();
					throw;
				}
			}

			void _release() noexcept
			{
				while (_count != 0)
				{
					_generator->_passes[--_count]->endJob(_coordinate);
				}
			}

		public:
			JobLease(const JobLease &) = delete;
			JobLease &operator=(const JobLease &) = delete;
			JobLease(JobLease &&other) noexcept :
				_generator(other._generator),
				_coordinate(other._coordinate),
				_count(std::exchange(other._count, 0))
			{
			}
			~JobLease()
			{
				_release();
			}
		};

		[[nodiscard]] JobLease reserve(const ChunkCoordinate &coordinate)
		{
			return JobLease(*this, coordinate);
		}

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
				auto job = reserve(coordinate);
				auto data = _createGenerationData(coordinate);
				if (!data.has_value())
				{
					return std::nullopt;
				}
				for (const std::unique_ptr<Pass> &pass : _passes)
				{
					pass->apply(coordinate, *data);
				}
				return _instantiateChunk(*data);
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
