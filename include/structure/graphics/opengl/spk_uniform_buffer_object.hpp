#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include "spk_vertex_buffer_object.hpp"

namespace spk
{
	class ShaderParser;
}

namespace spk::OpenGL
{
	class UniformBufferObject : public VertexBufferObject
	{
	public:
		using BindingPoint = int;
		using BlockIndex = int;

		class Factory;
		class Layout;

		struct Element
		{
			std::vector<Layout> layouts;

			template<typename TType,
				typename std::enable_if_t<!spk::IsContainer<TType>::value &&
				!spk::IsSpecialization<TType, std::initializer_list>::value>* = nullptr>
			Element& operator=(const TType& data)
			{
				if (layouts.empty())
					throw std::runtime_error("No layouts to assign to.");

				layouts[0] = data;
				return *this;
			}

			template<typename TContainer,
				typename std::enable_if_t<spk::IsContainer<TContainer>::value &&
				!spk::IsSpecialization<TContainer, std::initializer_list>::value>* = nullptr>
			Element& operator=(const TContainer& data)
			{
				if (layouts.size() != data.size())
					throw std::invalid_argument("Data size does not match the number of layouts.");

				auto dataIt = std::begin(data);
				for (auto& layout : layouts)
				{
					layout = *dataIt;
					++dataIt;
				}
				return *this;
			}

			template<typename TType>
			Element& operator=(std::initializer_list<TType> data)
			{
				if (layouts.size() != data.size())
					throw std::invalid_argument("Data size does not match the number of layouts.");

				auto dataIt = data.begin();
				for (auto& layout : layouts)
				{
					layout = *dataIt;
					++dataIt;
				}
				return *this;
			}

			operator Layout& ()
			{
				if (layouts.empty())
					throw std::runtime_error("No layouts to convert from.");
				return layouts[0];
			}

			Layout& operator[](size_t index)
			{
				if (index >= layouts.size())
					throw std::out_of_range("Index out of range.");
				return layouts[index];
			}
		};

		class Layout
		{
			friend class Factory;
			friend class spk::ShaderParser;

		public:

			struct DataRepresentation
			{
				size_t offset;
				size_t size;
			};

		private:
			char* _destination;
			DataRepresentation _cpu;
			DataRepresentation _gpu;
			bool _tightlyPacked;

			std::unordered_map<std::wstring, Element> _innerLayouts;

			void _pushData(const char* p_basePtr);

		public:
			Layout();
			Layout(const DataRepresentation& p_cpu, const DataRepresentation& p_gpu);

			void bind(char* p_destination);

			template <typename TType>
			Layout& operator=(const TType& p_data)
			{
				if (sizeof(TType) != _cpu.size)
					throw std::invalid_argument("Unexpected parameter type provided to uniform buffer object.\nExpected [" + std::to_string(_cpu.size) + "] byte(s), received [" + std::to_string(sizeof(TType)) + "].");

				_pushData(reinterpret_cast<const char*>(&p_data));
				return *this;
			}

			Element& operator[](const std::wstring& p_name);
		};

		class Factory
		{
			friend class spk::ShaderParser;

		private:
			std::string _typeName;
			BindingPoint _bindingPoint;
			Layout _layout;

		public:
			void setTypeName(const std::string& p_name);
			void setBindingPoint(BindingPoint p_bindingPoint);

			const std::string& typeName() const;
			BindingPoint bindingPoint() const;

			Layout& mainLayout();
			const Layout& mainLayout() const;
			Layout& addInnerLayout(Layout& p_layout, const std::wstring& p_name, const Layout::DataRepresentation& p_cpu, const Layout::DataRepresentation& p_gpu);
			UniformBufferObject construct() const;
		};

	private:
		std::string _typeName;
		BindingPoint _bindingPoint;
		BlockIndex _blockIndex;
		Layout _layout;

	public:
		UniformBufferObject();
		
		UniformBufferObject(const UniformBufferObject& p_other);
		UniformBufferObject& operator=(const UniformBufferObject& p_other);

		UniformBufferObject(UniformBufferObject&& p_other) noexcept;
		UniformBufferObject& operator=(UniformBufferObject&& p_other) noexcept;

		const std::string& typeName() const;
		const BindingPoint& bindingPoint() const;

		void activate();

		template <typename TType>
		UniformBufferObject& operator=(const TType& p_layout)
		{
			_layout = p_layout;
			validate();
			return *this;
		}

		Element& operator[](const std::wstring& p_name);
	};
}
