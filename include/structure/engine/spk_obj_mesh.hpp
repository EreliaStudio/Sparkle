#pragma once

#include <filesystem>
#include <string>

#include "structure/engine/spk_mesh.hpp"
#include "structure/engine/spk_vertex.hpp"
#include "structure/graphics/texture/spk_texture.hpp"
#include "structure/spk_safe_pointer.hpp"

namespace spk
{
	class ObjMesh : public TMesh<Vertex>
	{
	private:
		spk::SafePointer<const spk::Texture> _material;

	public:
		ObjMesh() = default;

		void setMaterial(spk::SafePointer<const spk::Texture> p_material);
		const spk::SafePointer<const spk::Texture> &material() const;

		void applyOffset(const spk::Vector3 &p_offset);

		static ObjMesh loadFromString(const std::string &p_input);
		static ObjMesh loadFromFile(const std::filesystem::path &p_path);

		void exportToFile(const std::filesystem::path &p_path) const;
	};
}