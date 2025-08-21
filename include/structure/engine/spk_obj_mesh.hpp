#pragma once

#include <filesystem>
#include <string>

#include "structure/engine/spk_vertex.hpp"
#include "structure/engine/spk_mesh.hpp"

namespace spk
{
	class ObjMesh : public TMesh<Vertex>
	{
	private:

	public:
		ObjMesh() = default;

		void applyOffset(const spk::Vector3& p_offset);

		static ObjMesh loadFromString(const std::string &p_input);
		static ObjMesh loadFromFile(const std::filesystem::path &p_path);

		void exportToFile(const std::filesystem::path &p_path) const;
	};
}