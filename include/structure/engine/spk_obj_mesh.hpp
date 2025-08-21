#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

#include "structure/engine/spk_mesh.hpp"
#include "structure/engine/spk_vertex.hpp"
#include "structure/math/spk_vector3.hpp"
#include "structure/spk_safe_pointer.hpp"

namespace spk
{
	class ObjMesh : public TMesh<Vertex>
	{
	public:
		struct Material
		{
			spk::Vector3 ambientColor = {0.0f, 0.0f, 0.0f};
			spk::Vector3 diffuseColor = {1.0f, 1.0f, 1.0f};
			spk::Vector3 specularColor = {0.0f, 0.0f, 0.0f};
			std::filesystem::path diffuseTexture;
		};

	private:
		spk::SafePointer<const Material> _material = nullptr;
		static inline std::unordered_map<std::string, spk::SafePointer<const Material>> _materialCache;
		static spk::SafePointer<const Material> _loadMaterial(const std::filesystem::path &p_path, const std::string &p_name);

	public:
		ObjMesh() = default;

		void applyOffset(const spk::Vector3 &p_offset);

		static ObjMesh loadFromString(const std::string &p_input, const std::filesystem::path &p_directory = ".");
		static ObjMesh loadFromFile(const std::filesystem::path &p_path);

		const spk::SafePointer<const Material> &material() const;

		void exportToFile(const std::filesystem::path &p_path) const;
	};
}
