#pragma once

#include <concepts>
#include <memory>
#include <utility>

#include <exception.hpp>

namespace spk
{
	template <typename TType>
	class Singleton final
	{
	private:
		inline static std::unique_ptr<TType> _instance;

	public:
		Singleton() = delete;

		static void instanciate(TType value)
			requires std::move_constructible<TType>
		{
			_instance = std::make_unique<TType>(std::move(value));
		}

		static void instanciate(TType *value)
		{
			if (value == nullptr)
			{
				throw spk::Exception("Cannot instanciate a Singleton from a null pointer");
			}
			_instance.reset(value);
		}

		[[nodiscard]] static bool isInstanciated() noexcept
		{
			return _instance != nullptr;
		}

		[[nodiscard]] static TType &instance()
		{
			if (_instance == nullptr)
			{
				throw spk::Exception("Singleton is not instanciated");
			}
			return *_instance;
		}
	};
}
