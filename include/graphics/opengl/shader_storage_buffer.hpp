#pragma once

#include <GL/glew.h>

#include <cstddef>
#include <cstring>
#include <memory>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "graphics/opengl/buffer_gpu_resource.hpp"

namespace spk
{
	class RenderContext;

	class ShaderStorageBuffer final : public BufferGPUResource
	{
	public:
		template <typename TFixedPart, typename TDynamicElement>
		struct View
		{
			TFixedPart &fixedPart;
			std::size_t nbElement;
			TDynamicElement *dynamicArray;
		};

		template <typename TDynamicElement>
		struct View<void, TDynamicElement>
		{
			std::size_t nbElement;
			TDynamicElement *dynamicArray;
		};

		template <typename TFixedPart, typename TDynamicElement>
		struct GPUView
		{
			TFixedPart fixedPart;
			std::vector<TDynamicElement> dynamicArray;
		};

		template <typename TDynamicElement>
		struct GPUView<void, TDynamicElement>
		{
			std::vector<TDynamicElement> dynamicArray;
		};

	public:
		class State final : public BufferGPUResource::State
		{
			friend class ShaderStorageBuffer;

		private:
			std::size_t _bindingPoint = 0;
			std::size_t _fixedPartSize = 0;
			std::size_t _dynamicElementSize = 0;
			std::size_t _dynamicElementCount = 0;

		protected:
			[[nodiscard]] GLenum _target() const noexcept override;
			void _bind(GPUResource::Instance &instance, RenderContext &context) const override;

		public:
			State(std::size_t bindingPoint, std::size_t fixedPartSize, std::size_t dynamicElementSize) :
				_bindingPoint(bindingPoint),
				_fixedPartSize(fixedPartSize),
				_dynamicElementSize(dynamicElementSize)
			{
			}
		};
		using Handle = GPUResource::Handle<State>;

	private:
		using BufferGPUResource::clear;

		[[nodiscard]] std::size_t _checkedSize(std::size_t nbElement) const;

		template <typename TType>
		using ConstIfNotVoid = std::conditional_t<
			std::is_void_v<TType>,
			void,
			const TType>;

		template <typename TFixedPart, typename TDynamicElement>
		void _validateTypes() const
		{
			const auto &content = state<State>();
			static_assert(
				std::is_trivially_copyable_v<TDynamicElement>,
				"ShaderStorageBuffer dynamic element requires a trivially copyable type.");

			if constexpr (std::is_void_v<TFixedPart>)
			{
				if (content._fixedPartSize != 0)
				{
					throw std::logic_error("ShaderStorageBuffer contains a fixed part");
				}
			}
			else
			{
				static_assert(
					std::is_trivially_copyable_v<TFixedPart>,
					"ShaderStorageBuffer fixed part requires a trivially copyable type.");

				if (sizeof(TFixedPart) != content._fixedPartSize)
				{
					throw std::logic_error("ShaderStorageBuffer fixed part size does not match the requested type");
				}
			}

			if (sizeof(TDynamicElement) != content._dynamicElementSize)
			{
				throw std::logic_error("ShaderStorageBuffer dynamic element size does not match the requested type");
			}
		}

		template <typename TFixedPart, typename TDynamicElement>
		void _validateCast() const
		{
			_validateTypes<TFixedPart, TDynamicElement>();
			const auto &content = state<State>();

			const auto dynamicAddress =
				reinterpret_cast<std::uintptr_t>(_data() + content._fixedPartSize);

			if (dynamicAddress % alignof(TDynamicElement) != 0)
			{
				throw std::logic_error("ShaderStorageBuffer dynamic array is not correctly aligned for the requested type");
			}

			if constexpr (!std::is_void_v<TFixedPart>)
			{
				const auto fixedAddress =
					reinterpret_cast<std::uintptr_t>(_data());

				if (fixedAddress % alignof(TFixedPart) != 0)
				{
					throw std::logic_error("ShaderStorageBuffer fixed part is not correctly aligned for the requested type");
				}
			}
		}

	public:
		ShaderStorageBuffer(std::size_t bindingPoint, std::size_t fixedPartSize, std::size_t dynamicElementSize);
		[[nodiscard]] Handle handle() const
		{
			return createHandle<State>();
		}

		[[nodiscard]] std::size_t bindingPoint() const noexcept;
		[[nodiscard]] std::size_t fixedPartSize() const noexcept;
		[[nodiscard]] std::size_t dynamicElementSize() const noexcept;
		[[nodiscard]] std::size_t dynamicElementCount() const noexcept;

		void resize(std::size_t nbElement);

		void setFixedData(const void *data, std::size_t size);
		void setDynamicData(const void *data, std::size_t nbElement);

		template <typename TFixedPart>
		void setFixedData(const TFixedPart &data)
		{
			static_assert(
				std::is_trivially_copyable_v<TFixedPart>,
				"ShaderStorageBuffer fixed part requires a trivially copyable type.");

			if (sizeof(TFixedPart) != state<State>()._fixedPartSize)
			{
				throw std::invalid_argument("ShaderStorageBuffer fixed part size does not match the provided type");
			}

			setFixedData(std::addressof(data), sizeof(TFixedPart));
		}

		template <typename TDynamicElement>
		void setDynamicData(std::span<const TDynamicElement> data)
		{
			static_assert(
				std::is_trivially_copyable_v<TDynamicElement>,
				"ShaderStorageBuffer dynamic element requires a trivially copyable type.");

			if (sizeof(TDynamicElement) != state<State>()._dynamicElementSize)
			{
				throw std::invalid_argument("ShaderStorageBuffer dynamic element size does not match the provided type");
			}

			setDynamicData(data.data(), data.size());
		}

		template <typename TFixedPart, typename TDynamicElement>
		[[nodiscard]] View<TFixedPart, TDynamicElement> cast()
		{
			_validateCast<TFixedPart, TDynamicElement>();
			const auto &content = state<State>();

			if constexpr (std::is_void_v<TFixedPart>)
			{
				return {
					.nbElement = content._dynamicElementCount,
					.dynamicArray = reinterpret_cast<TDynamicElement *>(_data() + content._fixedPartSize)};
			}
			else
			{
				return {
					.fixedPart = *reinterpret_cast<TFixedPart *>(_data()),
					.nbElement = content._dynamicElementCount,
					.dynamicArray = reinterpret_cast<TDynamicElement *>(_data() + content._fixedPartSize)};
			}
		}

		template <typename TFixedPart, typename TDynamicElement>
		[[nodiscard]] View<ConstIfNotVoid<TFixedPart>, const TDynamicElement> cast() const
		{
			_validateCast<TFixedPart, TDynamicElement>();
			const auto &content = state<State>();

			if constexpr (std::is_void_v<TFixedPart>)
			{
				return {
					.nbElement = content._dynamicElementCount,
					.dynamicArray = reinterpret_cast<const TDynamicElement *>(_data() + content._fixedPartSize)};
			}
			else
			{
				return {
					.fixedPart = *reinterpret_cast<const TFixedPart *>(_data()),
					.nbElement = content._dynamicElementCount,
					.dynamicArray = reinterpret_cast<const TDynamicElement *>(_data() + content._fixedPartSize)};
			}
		}

		template <typename TFixedPart, typename TDynamicElement>
		[[nodiscard]] GPUView<TFixedPart, TDynamicElement> retrieve(RenderContext &context) const
		{
			_validateTypes<TFixedPart, TDynamicElement>();
			const auto &content = state<State>();

			const auto data = BufferGPUResource::retrieve(context);

			if constexpr (std::is_void_v<TFixedPart>)
			{
				GPUView<void, TDynamicElement> result{
					.dynamicArray = std::vector<TDynamicElement>(content._dynamicElementCount)};

				if (!result.dynamicArray.empty())
				{
					std::memcpy(
						result.dynamicArray.data(),
						data.data() + content._fixedPartSize,
						result.dynamicArray.size() * sizeof(TDynamicElement));
				}

				return result;
			}
			else
			{
				GPUView<TFixedPart, TDynamicElement> result{
					.fixedPart = {},
					.dynamicArray = std::vector<TDynamicElement>(content._dynamicElementCount)};

				std::memcpy(
					std::addressof(result.fixedPart),
					data.data(),
					sizeof(TFixedPart));

				if (!result.dynamicArray.empty())
				{
					std::memcpy(
						result.dynamicArray.data(),
						data.data() + content._fixedPartSize,
						result.dynamicArray.size() * sizeof(TDynamicElement));
				}

				return result;
			}
		}
	};
}
