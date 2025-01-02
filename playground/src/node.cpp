#include "node.hpp"

NodeMap::NodeMap() :
	_nodeMapSSBO(BufferObjectCollection::instance()->SSBO("nodeConstants"))
{

}

void NodeMap::addNode(const int& p_id, const Node& p_node)
{
	if (p_id == -1)
	{
		throw std::runtime_error("Invalid node ID");
	}
	if (p_id >= _content.size())
	{
		_content.resize(p_id + 1);
	}
	_content[p_id] = p_node;
}

void NodeMap::validate()
{
	if (_content.size() == 0)
	{
		throw std::runtime_error("Node map is empty");
	}
	
	_nodeMapSSBO["nbNodes"] = int(_content.size());
	_nodeMapSSBO.resizeDynamicArray(_content.size());
	_nodeMapSSBO.dynamicArray() = _content;
	_nodeMapSSBO.validate();
}

const Node& NodeMap::operator[](const int& p_id) const
{
	if (p_id == -1)
	{
		throw std::runtime_error("Invalid node ID");
	}
	return (_content[p_id]);
}