#pragma once 

#include <memory>
#include <stdexcept>

#include "structure/spk_safe_pointer.hpp"

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
				if (reference == 0)
					Singleton<TType>::instanciate(std::forward<Args>(p_args)...);
				reference++;
			}

			~Instanciator()
			{
				reference--;
				if (reference == 0)
					Singleton<TType>::release();
			}

			spk::SafePointer<TType> operator ->()
			{
				return (Singleton<TType>::instance());
			}
		};

	protected:
		Singleton()
		{

		}

		static inline TType* _instance = nullptr;

	public:
		template <typename... Args>
		static spk::SafePointer<TType> instanciate(Args &&...p_args)
		{
			if (_instance != nullptr)
				throw std::runtime_error("Can't instanciate an already instancied singleton");

			_instance = new TType(std::forward<Args>(p_args)...);
			return (_instance);
		}

		static spk::SafePointer<TType> instance()
		{
			return (_instance);
		}

		static const spk::SafePointer<const TType> c_instance()
		{
			return _instance;
		}

		static void release()
		{
			delete _instance;
			_instance = nullptr;
		}
	};
}