#pragma once

#include "structure/math/spk_vector2.hpp"
#include "buffer_object_collection.hpp"

struct Node
{
	spk::Vector2 animationStartPos;
	int frameDuration;
	int animationLength;
	int animationStep;
};

class NodeMap
{
private:
	BufferObjectCollection::Instanciator _bindingPointInstanciator;
	spk::OpenGL::ShaderStorageBufferObject& _nodeMapSSBO;
	std::vector<Node> _content;

public:
	NodeMap();

	void addNode(const int& p_id, const Node& p_node);

	void validate();

	const Node& operator[](const int& p_id) const;
};