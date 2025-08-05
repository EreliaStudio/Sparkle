#pragma once

#include "structure/engine/spk_vertex.hpp"
#include "structure/engine/spk_mesh.hpp"

namespace spk
{
	class Mesh : public TMesh<Vertex>
	{
	private:

	public:
		Mesh() = default;

		void loadFromString(const std::string &p_input);
		void loadFromFile(const std::filesystem::path &p_path);

		void exportToFile(const std::filesystem::path &p_path) const;
	};
}