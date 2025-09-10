#pragma once

#include <memory>
#include <mutex>
#include <stdexcept>

#include "structure/spk_safe_pointer.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	template <typename TType>
	class Singleton
	{
	public:
		using Type = TType;

		class Instanciator
		{
		private:
			static inline size_t reference = 0;

		public:
			template <typename... Args>
			Instanciator(Args &&...p_args)
			{
				std::lock_guard<std::recursive_mutex> lock(Singleton<TType>::mutex());

				if (Singleton<TType>::instance() == nullptr)
				{
					Singleton<TType>::instanciate(std::forward<Args>(p_args)...);
				}

				reference++;
			}

			~Instanciator()
			{
				std::lock_guard<std::recursive_mutex> lock(Singleton<TType>::mutex());

				reference--;
				if (reference == 0)
				{
					Singleton<TType>::release();
				}
			}

			spk::SafePointer<TType> operator->()
			{
				std::lock_guard<std::recursive_mutex> lock(Singleton<TType>::mutex());

				return (Singleton<TType>::instance());
			}

			spk::SafePointer<const TType> operator->() const
			{
				std::lock_guard<std::recursive_mutex> lock(Singleton<TType>::mutex());

				return (Singleton<TType>::instance());
			}
		};

	protected:
		Singleton()
		{
		}

		static inline std::unique_ptr<TType> _instance = nullptr;
		static std::recursive_mutex& _mutex()
		{
			static std::recursive_mutex result;

			return (result);
		}

	public:
		template <typename... Args>
		static spk::SafePointer<TType> instanciate(Args &&...p_args)
		{
			std::lock_guard<std::recursive_mutex> lock(Singleton<TType>::mutex());

			if (_instance != nullptr)
			{
				GENERATE_ERROR("Can't instanciate an already instancied singleton");
			}

			_instance.reset(new TType(std::forward<Args>(p_args)...));
			return (_instance.get());
		}

		static spk::SafePointer<TType> instance()
		{
			return (_instance.get());
		}

		static const spk::SafePointer<const TType> cInstance()
		{
			return (_instance.get());
		}

		static std::recursive_mutex &mutex()
		{
			return _mutex();
		}

		static void release()
		{
			std::lock_guard<std::recursive_mutex> lock(Singleton<TType>::mutex());

			_instance = nullptr;
		}
	};
}