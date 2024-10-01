#include "../Common.h"
#include "BVHTree.h"

void BVHTree::Initialize(const UINT MAX_NODE_NUM)
{
	_ASSERT(MAX_NODE_NUM > 0 && MAX_NODE_NUM < Node::NULL_NODE);

	m_AllocatedNodes = MAX_NODE_NUM;

	m_pNodes = new Node[MAX_NODE_NUM];
	for (UINT i = 0, endI = MAX_NODE_NUM - 1; i < endI; ++i)
	{
		m_pNodes[i].NextNodeID = i + 1;
	}
	m_pNodes[MAX_NODE_NUM - 1].NextNodeID = Node::NULL_NODE;

	m_FreeNodeID = 0;
}

void BVHTree::AddObject(const DirectX::BoundingBox& AABB)
{
	_ASSERT(m_pNodes);
	addObjectInternal(AABB);
}

void BVHTree::Cleanup()
{
	if (m_pNodes)
	{
		delete[] m_pNodes;
		m_pNodes = nullptr;
	}
	m_RootNodeID = Node::NULL_NODE;
	m_FreeNodeID = Node::NULL_NODE;
	m_AllocatedNodes = 0;
	m_TotalNodes = 0;
}

UINT BVHTree::GetMaxDepth()
{
	_ASSERT(m_pNodes);
	return (m_pNodes + m_RootNodeID)->Height;
}

void BVHTree::addObjectInternal(const DirectX::BoundingBox& AABB)
{
	_ASSERT(m_pNodes);

	UINT nodeID = allocateNode();
	m_pNodes[nodeID].AABB = AABB;

	insertLeafNode(nodeID);
	if (!m_pNodes[nodeID].IsLeaf())
	{
		__debugbreak();
	}
	if (nodeID == Node::NULL_NODE)
	{
		__debugbreak();
	}
}

UINT BVHTree::allocateNode()
{
	_ASSERT(m_pNodes);

	if (m_TotalNodes == m_AllocatedNodes)
	{
		UINT oldAllocatedNodes = m_AllocatedNodes;
		m_AllocatedNodes *= 2;

		Node* pOldTree = m_pNodes;
		m_pNodes = new Node[m_AllocatedNodes];
		memcpy(m_pNodes, pOldTree, sizeof(Node) * oldAllocatedNodes);
		delete[] pOldTree;
		pOldTree = nullptr;

		for (UINT i = m_TotalNodes, endI = m_AllocatedNodes - 1; i < endI; ++i)
		{
			m_pNodes[i].NextNodeID = i + 1;
		}
		m_pNodes[m_AllocatedNodes - 1].NextNodeID = Node::NULL_NODE;

		m_FreeNodeID = m_TotalNodes;
	}

	UINT freeNodeID = m_FreeNodeID;
	m_FreeNodeID = m_pNodes[freeNodeID].NextNodeID;

	m_pNodes[freeNodeID].ParentID = Node::NULL_NODE;
	m_pNodes[freeNodeID].Height = 0;
	++m_TotalNodes;

	return freeNodeID;
}

void BVHTree::releaseNode(const UINT NODE_ID)
{
	_ASSERT(m_pNodes);
	_ASSERT(NODE_ID >= 0 && NODE_ID < m_AllocatedNodes);

	m_pNodes[NODE_ID].NextNodeID = m_FreeNodeID;
	m_pNodes[NODE_ID].Height = 0xFFFFFFFF;
	m_FreeNodeID = NODE_ID;
	--m_TotalNodes;
}

void BVHTree::insertLeafNode(const UINT NODE_ID)
{
	if (m_RootNodeID == Node::NULL_NODE)
	{
		m_RootNodeID = NODE_ID;
		m_pNodes[m_RootNodeID].ParentID = Node::NULL_NODE;

		return;
	}


}
