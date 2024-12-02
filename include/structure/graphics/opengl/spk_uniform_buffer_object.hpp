#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include "spk_vertex_buffer_object.hpp"

namespace spk
{
	class Pipeline;	
}

namespace spk::OpenGL
{
	/**
	 * @brief Represents a Uniform Buffer Object (UBO) for OpenGL rendering.
	 *
	 * This class provides functionality to manage and use UBOs, enabling
	 * efficient handling of data buffers in shaders.
	 *
	 * Example usage:
	 * @code
	 * spk::OpenGL::UniformBufferObject ubo;
	 * ubo = someLayout; // Assign a layout to the UBO
	 * ubo.activate(); // Activate the UBO
	 * auto& element = ubo[L"someElement"];
	 * element = 42; // Assign data to the element
	 * @endcode
	 */
	class UniformBufferObject : public VertexBufferObject
	{
	public:
		using BindingPoint = int;
		using BlockIndex = int;

		/**
		 * @brief Factory class to construct UniformBufferObjects.
		 */
		class Factory;

		/**
		 * @brief Layout class to describe the buffer layout.
		 */
		class Layout;

		/**
		 * @brief Represents an element in the Uniform Buffer Object.
		 */
		struct Element
		{
			std::vector<Layout> layouts;

			/**
			 * @brief Assigns data to the first layout.
			 * @tparam TType The type of data to assign.
			 * @param data The data to assign.
			 * @return Reference to the current Element.
			 * @throws std::runtime_error If no layouts are available.
			 */
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

			/**
			 * @brief Assigns a container of data to all layouts.
			 * @tparam TContainer The type of container to assign.
			 * @param data The data container.
			 * @return Reference to the current Element.
			 * @throws std::invalid_argument If the container size does not match the layouts size.
			 */
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

			/**
			 * @brief Assigns data from an initializer list to all layouts.
			 * @tparam TType The type of data in the initializer list.
			 * @param data The initializer list.
			 * @return Reference to the current Element.
			 * @throws std::invalid_argument If the list size does not match the layouts size.
			 */
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

			/**
			 * @brief Converts the Element to the first layout.
			 * @return Reference to the first Layout.
			 * @throws std::runtime_error If no layouts are available.
			 */
			operator Layout& ()
			{
				if (layouts.empty())
					throw std::runtime_error("No layouts to convert from.");
				return layouts[0];
			}

			/**
			 * @brief Accesses a specific layout by index.
			 * @param index The index of the layout.
			 * @return Reference to the Layout at the given index.
			 * @throws std::out_of_range If the index is invalid.
			 */
			Layout& operator[](size_t index)
			{
				if (index >= layouts.size())
					throw std::out_of_range("Index out of range.");
				return layouts[index];
			}
		};

		/**
		 * @brief Represents a buffer layout.
		 */
		class Layout
		{
			friend class Factory;
			friend class spk::Pipeline;

		public:
			/**
			 * @brief Represents a data representation with offset and size.
			 */
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

			/**
			 * @brief Pushes data into the buffer.
			 * @param p_basePtr Pointer to the base data.
			 */
			void _pushData(const char* p_basePtr);

		public:
			/**
			 * @brief Constructs a Layout with no parameters.
			 */
			Layout();

			/**
			 * @brief Constructs a Layout with given CPU and GPU data representations.
			 * @param p_cpu The CPU data representation.
			 * @param p_gpu The GPU data representation.
			 */
			Layout(const DataRepresentation& p_cpu, const DataRepresentation& p_gpu);

			/**
			 * @brief Binds the Layout to a destination buffer.
			 * @param p_destination Pointer to the destination buffer.
			 */
			void bind(char* p_destination);

			/**
			 * @brief Assigns data to the layout.
			 * @tparam TType The type of data.
			 * @param p_data The data to assign.
			 * @return Reference to the current Layout.
			 * @throws std::invalid_argument If the data size does not match the layout size.
			 */
			template <typename TType>
			Layout& operator=(const TType& p_data)
			{
				if (sizeof(TType) != _cpu.size)
					throw std::invalid_argument("Unexpected parameter type provided to uniform buffer object.\nExpected [" + std::to_string(_cpu.size) + "] byte(s), received [" + std::to_string(sizeof(TType)) + "].");

				_pushData(reinterpret_cast<const char*>(&p_data));
				return *this;
			}

			/**
			 * @brief Accesses an inner layout by name.
			 * @param p_name The name of the inner layout.
			 * @return Reference to the corresponding Element.
			 */
			Element& operator[](const std::wstring& p_name);
		};

		/**
		 * @brief Factory class for creating Uniform Buffer Objects.
		 */
		class Factory
		{
			friend class spk::Pipeline ;

		private:
			std::string _typeName;
			BindingPoint _bindingPoint;
			Layout _layout;

		public:
			/**
			 * @brief Sets the type name of the UBO.
			 * @param p_name The type name.
			 */
			void setTypeName(const std::string& p_name);

			/**
			 * @brief Sets the binding point of the UBO.
			 * @param p_bindingPoint The binding point.
			 */
			void setBindingPoint(BindingPoint p_bindingPoint);

			/**
			 * @brief Gets the type name of the UBO.
			 * @return The type name.
			 */
			const std::string& typeName() const;

			/**
			 * @brief Gets the binding point of the UBO.
			 * @return The binding point.
			 */
			BindingPoint bindingPoint() const;

			/**
			 * @brief Gets the main layout of the UBO.
			 * @return Reference to the main layout.
			 */
			Layout& mainLayout();

			/**
			 * @brief Gets the main layout of the UBO as a constant reference.
			 * @return Constant reference to the main layout.
			 */
			const Layout& mainLayout() const;

			/**
			 * @brief Adds an inner layout to the UBO.
			 * @param p_layout The layout to add.
			 * @param p_name The name of the inner layout.
			 * @param p_cpu The CPU data representation.
			 * @param p_gpu The GPU data representation.
			 * @return Reference to the added Layout.
			 */
			Layout& addInnerLayout(Layout& p_layout, const std::wstring& p_name, const Layout::DataRepresentation& p_cpu, const Layout::DataRepresentation& p_gpu);

			/**
			 * @brief Constructs the Uniform Buffer Object.
			 * @return The constructed UBO.
			 */
			UniformBufferObject construct() const;
		};

	private:
		std::string _typeName;
		BindingPoint _bindingPoint;
		BlockIndex _blockIndex;
		Layout _layout;

	public:
		/**
		 * @brief Default constructor.
		 */
		UniformBufferObject();
		
		/**
		 * @brief Copy constructor.
		 * @param p_other The object to copy from.
		 */
		UniformBufferObject(const UniformBufferObject& p_other);

		/**
		 * @brief Copy assignment operator.
		 * @param p_other The object to copy from.
		 * @return Reference to the current object.
		 */
		UniformBufferObject& operator=(const UniformBufferObject& p_other);

		/**
		 * @brief Move constructor.
		 * @param p_other The object to move from.
		 */
		UniformBufferObject(UniformBufferObject&& p_other) noexcept;

		/**
		 * @brief Move assignment operator.
		 * @param p_other The object to move from.
		 * @return Reference to the current object.
		 */
		UniformBufferObject& operator=(UniformBufferObject&& p_other) noexcept;

		/**
		 * @brief Gets the type name of the UBO.
		 * @return The type name.
		 */
		const std::string& typeName() const;

		/**
		 * @brief Gets the binding point of the UBO.
		 * @return The binding point.
		 */
		const BindingPoint& bindingPoint() const;

		/**
		 * @brief Activates the UBO for rendering.
		 */
		void activate();

		/**
		 * @brief Assigns a layout to the UBO.
		 * @tparam TType The type of layout.
		 * @param p_layout The layout to assign.
		 * @return Reference to the current UBO.
		 */
		template <typename TType>
		UniformBufferObject& operator=(const TType& p_layout)
		{
			_layout = p_layout;
			validate();
			return *this;
		}

		/**
		 * @brief Accesses an element by name.
		 * @param p_name The name of the element.
		 * @return Reference to the corresponding Element.
		 */
		Element& operator[](const std::wstring& p_name);
	};
}
