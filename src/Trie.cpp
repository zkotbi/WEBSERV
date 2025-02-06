#include "Trie.hpp"
#include <cstring>
#include <iostream>
#include "Tokenizer.hpp"

Trie::TrieNode::TrieNode() : isEnd(false)
{
	memset(children, 0, sizeof(children));
}

Trie::TrieNode::~TrieNode() {}

Trie::Trie()
{
	root = new TrieNode();
}

void Trie::deleteNode()
{
	this->_deleteNode(root);
}

void Trie::_deleteNode(TrieNode *node)
{
	if (node == NULL)
		return;
	for (size_t i = 0; i < 128; i++)
	{
		if (node->children[i] != NULL)
		{
			_deleteNode(node->children[i]);
			node->children[i] = NULL;
		}
	}
	delete node;
}

Trie::~Trie() {}

bool Trie::insert(Location &location)
{
	const std::string &path = location.getPath();
	TrieNode *currNode = root;
	int idx;
	for (size_t i = 0; i < path.size(); i++)
	{
		idx = path[i];
		if (idx < 0 || idx > 128)
			throw Tokenizer::ParserException("InValide char in path " + path);
		if (currNode->children[idx] == NULL)
			currNode->children[idx] = new TrieNode();
		currNode = currNode->children[idx];
	}
	if (currNode->isEnd)
		throw Tokenizer::ParserException("Duplicate location route at: " + path);
	currNode->isEnd = true;
	currNode->location = location;
	this->locations.push_back(&currNode->location);
	return (true);
}

Location *Trie::findPath(const std::string &route)
{
	TrieNode *currNode = root;
	Location *location = NULL;
	int idx;
	for (size_t i = 0; i < route.size(); i++)
	{
		idx = route[i];
		if (idx < 0 || idx > 128)
			return location;
		if (currNode->children[idx] == NULL)
			break;
		if (currNode->children[idx]->isEnd)
			location = &currNode->children[idx]->location;
		currNode = currNode->children[idx];
	}
	return (location);
}

void Trie::init(const GlobalConfig &conf)
{
	for (size_t i = 0; i < this->locations.size(); i++)
	{
		this->locations[i]->globalConfig.copy(conf);
		if (this->locations[i]->globalConfig.getRoot().empty())
			throw Tokenizer::ParserException("Root in loaction must not be empty");
	}
}
