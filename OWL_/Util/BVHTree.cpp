#include "../Common.h"
#include "BVHTree.h"

inline float GetVolume(const DirectX::BoundingBox& AABB)
{
	DirectX::SimpleMath::Vector3 extent = AABB.Extents;
	return (extent.x * extent.y * extent.z);
}

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

void BVHTree::AddObject(const DirectX::BoundingBox* const pAABB)
{
	_ASSERT(m_pNodes);
	addObjectInternal(pAABB);
}

bool BVHTree::UpdateObject(const UINT NODE_ID, const DirectX::BoundingBox* const pNewAABB, bool bForceReinsert)
{
	_ASSERT(m_pNodes);
	_ASSERT(pNewAABB);
	_ASSERT(NODE_ID >= 0 && NODE_ID < m_AllocatedNodes);
	_ASSERT(m_pNodes[NODE_ID].IsLeaf());
	_ASSERT(m_pNodes[NODE_ID].Height >= 0);

	bool bRet = true;

	if (!bForceReinsert && m_pNodes[NODE_ID].AABB.Contains(*pNewAABB) == DirectX::CONTAINS)
	{
		bRet = false;
		goto LB_RET;
	}

	removeLeafNode(NODE_ID);
	m_pNodes[NODE_ID].AABB = *pNewAABB;
	insertLeafNode(NODE_ID);

LB_RET:
	return bRet;
}

void BVHTree::RemoveObject(const UINT NODE_ID)
{
	_ASSERT(NODE_ID >= 0 && NODE_ID < m_AllocatedNodes);
	_ASSERT(m_pNodes[NODE_ID].IsLeaf());

	removeLeafNode(NODE_ID);
	releaseNode(NODE_ID);
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

int BVHTree::GetMaxDepth()
{
	_ASSERT(m_pNodes);
	return (m_pNodes + m_RootNodeID)->Height;
}

void BVHTree::addObjectInternal(const DirectX::BoundingBox* const pAABB)
{
	_ASSERT(m_pNodes);

	UINT nodeID = allocateNode();
	m_pNodes[nodeID].AABB = *pAABB;

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
	m_pNodes[NODE_ID].Height = -1;
	m_FreeNodeID = NODE_ID;
	--m_TotalNodes;
}

void BVHTree::insertLeafNode(const UINT NODE_ID)
{
	_ASSERT(m_pNodes);
	_ASSERT(NODE_ID >= 0 && NODE_ID < m_AllocatedNodes);

	if (m_RootNodeID == Node::NULL_NODE)
	{
		m_RootNodeID = NODE_ID;
		m_pNodes[m_RootNodeID].ParentID = Node::NULL_NODE;

		return;
	}

	DirectX::BoundingBox newNodeAABB = m_pNodes[NODE_ID].AABB;
	UINT currentNodeID = m_RootNodeID;
	while (!m_pNodes[currentNodeID].IsLeaf())
	{
		UINT leftChild = m_pNodes[currentNodeID].Children[NodeChild_Left];
		UINT rightChild = m_pNodes[currentNodeID].Children[NodeChild_Right];

		float volumeAABB = GetVolume(m_pNodes[currentNodeID].AABB);

		DirectX::BoundingBox mergedAABB;
		DirectX::BoundingBox::CreateMerged(mergedAABB, m_pNodes[currentNodeID].AABB, newNodeAABB);
		float mergedVolume = GetVolume(mergedAABB);

		float costS = 2.0f * mergedVolume;
		float costI = 2.0f * (mergedVolume - volumeAABB);

		float costLeft;
		DirectX::BoundingBox currentAndLeftAABB;
		DirectX::BoundingBox::CreateMerged(currentAndLeftAABB, newNodeAABB, m_pNodes[rightChild].AABB);
		if (m_pNodes[leftChild].IsLeaf())
		{
			costLeft = GetVolume(currentAndLeftAABB) + costI;
		}
		else
		{
			float leftChildVolume = GetVolume(m_pNodes[leftChild].AABB);
			costLeft = costI + GetVolume(currentAndLeftAABB) - leftChildVolume;
		}

		float costRight;
		DirectX::BoundingBox currentAndRightAABB;
		DirectX::BoundingBox::CreateMerged(currentAndRightAABB, newNodeAABB, m_pNodes[rightChild].AABB);
		if (m_pNodes[rightChild].IsLeaf())
		{
			costRight = GetVolume(currentAndRightAABB) + costI;
		}
		else
		{
			float rightChildVolume = GetVolume(m_pNodes[rightChild].AABB);
			costRight = costI + GetVolume(currentAndRightAABB) - rightChildVolume;
		}

		if (costS < costLeft && costS < costRight)
		{
			break;
		}

		if (costLeft < costRight)
		{
			currentNodeID = leftChild;
		}
		else
		{
			currentNodeID = rightChild;
		}
	}

	UINT siblingNode = currentNodeID;

	UINT oldParentNode = m_pNodes[siblingNode].ParentID;
	UINT newParentNode = allocateNode();
	m_pNodes[newParentNode].ParentID = oldParentNode;
	DirectX::BoundingBox::CreateMerged(m_pNodes[newParentNode].AABB, m_pNodes[siblingNode].AABB, newNodeAABB);
	m_pNodes[newParentNode].Height = m_pNodes[siblingNode].Height + 1;
	_ASSERT(m_pNodes[newParentNode].Height > 0);

	if (oldParentNode != Node::NULL_NODE)
	{
		_ASSERT(!m_pNodes[oldParentNode].IsLeaf());
		if (m_pNodes[oldParentNode].Children[NodeChild_Left] == siblingNode)
		{
			m_pNodes[oldParentNode].Children[NodeChild_Left] = newParentNode;
		}
		else
		{
			m_pNodes[oldParentNode].Children[NodeChild_Right] = newParentNode;
		}
		m_pNodes[newParentNode].Children[NodeChild_Left] = siblingNode;
		m_pNodes[newParentNode].Children[NodeChild_Right] = NODE_ID;
		m_pNodes[siblingNode].ParentID = newParentNode;
		m_pNodes[NODE_ID].ParentID = newParentNode;
	}
	else
	{
		m_pNodes[newParentNode].Children[NodeChild_Left] = siblingNode;
		m_pNodes[newParentNode].Children[NodeChild_Right] = NODE_ID;
		m_pNodes[siblingNode].ParentID = newParentNode;
		m_pNodes[NODE_ID].ParentID = newParentNode;
		m_RootNodeID = newParentNode;
	}

	currentNodeID = m_pNodes[NODE_ID].ParentID;
	_ASSERT(!m_pNodes[currentNodeID].IsLeaf());
	while (currentNodeID != Node::NULL_NODE)
	{
		currentNodeID = balanceSubTreeAtNode(currentNodeID);
		_ASSERT(m_pNodes[NODE_ID].IsLeaf());

		_ASSERT(!m_pNodes[currentNodeID].IsLeaf());
		UINT leftChild = m_pNodes[currentNodeID].Children[NodeChild_Left];
		UINT rightChild = m_pNodes[currentNodeID].Children[NodeChild_Right];
		_ASSERT(leftChild != Node::NULL_NODE);
		_ASSERT(rightChild != Node::NULL_NODE);

		m_pNodes[currentNodeID].Height = Max(m_pNodes[leftChild].Height, m_pNodes[rightChild].Height) + 1;
		_ASSERT(m_pNodes[currentNodeID].Height > 0);

		DirectX::BoundingBox::CreateMerged(m_pNodes[currentNodeID].AABB, m_pNodes[leftChild].AABB, m_pNodes[rightChild].AABB);

		currentNodeID = m_pNodes[currentNodeID].ParentID;
	}
	_ASSERT(m_pNodes[NODE_ID].IsLeaf());
}

void BVHTree::removeLeafNode(const UINT NODE_ID)
{
	_ASSERT(NODE_ID >= 0 && NODE_ID < m_AllocatedNodes);
	_ASSERT(m_pNodes[NODE_ID].IsLeaf());

	if (m_RootNodeID == NODE_ID)
	{
		m_RootNodeID = Node::NULL_NODE;
		return;
	}

	UINT parentNodeID = m_pNodes[NODE_ID].ParentID;
	UINT grandParentNodeID = m_pNodes[parentNodeID].ParentID;
	UINT siblingNodeID;
	if (m_pNodes[parentNodeID].Children[NodeChild_Left] == NODE_ID)
	{
		siblingNodeID = m_pNodes[parentNodeID].Children[NodeChild_Right];
	}
	else
	{
		siblingNodeID = m_pNodes[parentNodeID].Children[NodeChild_Left];
	}

	if (grandParentNodeID != Node::NULL_NODE)
	{
		if (m_pNodes[grandParentNodeID].Children[NodeChild_Left] == parentNodeID)
		{
			m_pNodes[grandParentNodeID].Children[NodeChild_Left] = siblingNodeID;
		}
		else
		{
			_ASSERT(m_pNodes[grandParentNodeID].Children[NodeChild_Right] == parentNodeID);
			m_pNodes[grandParentNodeID].Children[NodeChild_Right] = siblingNodeID;
		}
		m_pNodes[siblingNodeID].ParentID = grandParentNodeID;
		releaseNode(parentNodeID);

		UINT currentNodeID = grandParentNodeID;
		while (currentNodeID != Node::NULL_NODE)
		{
			currentNodeID = balanceSubTreeAtNode(currentNodeID);

			_ASSERT(!m_pNodes[currentNodeID].IsLeaf());

			UINT leftChildID = m_pNodes[currentNodeID].Children[NodeChild_Left];
			UINT rightChildID = m_pNodes[currentNodeID].Children[NodeChild_Right];

			DirectX::BoundingBox::CreateMerged(m_pNodes[currentNodeID].AABB, m_pNodes[leftChildID].AABB, m_pNodes[rightChildID].AABB);
			m_pNodes[currentNodeID].Height = Max(m_pNodes[leftChildID].Height, m_pNodes[rightChildID].Height) + 1;
			_ASSERT(m_pNodes[currentNodeID].Height > 0);

			currentNodeID = m_pNodes[currentNodeID].ParentID;
		}
	}
	else
	{
		m_RootNodeID = siblingNodeID;
		m_pNodes[siblingNodeID].ParentID = Node::NULL_NODE;
		releaseNode(parentNodeID);
	}
}

UINT BVHTree::balanceSubTreeAtNode(const UINT NODE_ID)
{
	_ASSERT(m_pNodes);
	_ASSERT(NODE_ID != Node::NULL_NODE);

	Node* pNodeA = m_pNodes + NODE_ID;
	if (pNodeA->IsLeaf() || pNodeA->Height < 2)
	{
		return NODE_ID;
	}

	UINT nodeBID = pNodeA->Children[NodeChild_Left];
	UINT nodeCID = pNodeA->Children[NodeChild_Right];
	_ASSERT(nodeBID >= 0 && nodeBID < m_AllocatedNodes);
	_ASSERT(nodeCID >= 0 && nodeCID < m_AllocatedNodes);
	Node* pNodeB = m_pNodes + nodeBID;
	Node* pNodeC = m_pNodes + nodeCID;
	int balanceFactor = pNodeC->Height - pNodeB->Height;

	if (balanceFactor > 1)
	{
		_ASSERT(!pNodeC->IsLeaf());
		UINT nodeDID = pNodeC->Children[NodeChild_Left];
		UINT nodeEID = pNodeC->Children[NodeChild_Right];
		_ASSERT(nodeDID >= 0 && nodeDID < m_AllocatedNodes);
		_ASSERT(nodeEID >= 0 && nodeEID < m_AllocatedNodes);
		Node* pNodeD = m_pNodes + nodeDID;
		Node* pNodeE = m_pNodes + nodeEID;

		pNodeC->Children[NodeChild_Left] = NODE_ID;
		pNodeC->ParentID = pNodeA->ParentID;
		pNodeA->ParentID = nodeCID;

		if (pNodeC->ParentID != Node::NULL_NODE)
		{

			if (m_pNodes[pNodeC->ParentID].Children[NodeChild_Left] == NODE_ID)
			{
				m_pNodes[pNodeC->ParentID].Children[NodeChild_Left] = nodeCID;
			}
			else
			{
				_ASSERT(m_pNodes[pNodeC->ParentID].Children[NodeChild_Right] == NODE_ID);
				m_pNodes[pNodeC->ParentID].Children[NodeChild_Right] = nodeCID;
			}
		}
		else
		{
			m_RootNodeID = nodeCID;
		}

		_ASSERT(!pNodeC->IsLeaf());
		_ASSERT(!pNodeA->IsLeaf());

		if (pNodeD->Height > pNodeE->Height)
		{
			pNodeC->Children[NodeChild_Right] = nodeDID;
			pNodeA->Children[NodeChild_Right] = nodeEID;
			pNodeE->ParentID = NODE_ID;

			DirectX::BoundingBox::CreateMerged(pNodeA->AABB, pNodeB->AABB, pNodeE->AABB);
			DirectX::BoundingBox::CreateMerged(pNodeC->AABB, pNodeA->AABB, pNodeD->AABB);

			pNodeA->Height = Max(pNodeB->Height, pNodeE->Height) + 1;
			pNodeC->Height = Max(pNodeA->Height, pNodeD->Height) + 1;
			_ASSERT(pNodeA->Height > 0);
			_ASSERT(pNodeC->Height > 0);
		}
		else
		{
			pNodeC->Children[NodeChild_Right] = nodeEID;
			pNodeA->Children[NodeChild_Right] = nodeDID;
			pNodeD->ParentID = NODE_ID;

			DirectX::BoundingBox::CreateMerged(pNodeA->AABB, pNodeB->AABB, pNodeD->AABB);
			DirectX::BoundingBox::CreateMerged(pNodeC->AABB, pNodeA->AABB, pNodeE->AABB);

			pNodeA->Height = Max(pNodeB->Height, pNodeD->Height) + 1;
			pNodeC->Height = Max(pNodeA->Height, pNodeE->Height) + 1;
			_ASSERT(pNodeA->Height > 0);
			_ASSERT(pNodeC->Height > 0);
		}

		return nodeCID;
	}

	if (balanceFactor < -1)
	{
		_ASSERT(!pNodeB->IsLeaf());

		UINT nodeFID = pNodeB->Children[NodeChild_Left];
		UINT nodeGID = pNodeB->Children[NodeChild_Right];
		_ASSERT(nodeFID >= 0 && nodeFID < m_AllocatedNodes);
		_ASSERT(nodeGID >= 0 && nodeGID < m_AllocatedNodes);
		Node* pNodeF = m_pNodes + nodeFID;
		Node* pNodeG = m_pNodes + nodeGID;

		pNodeB->Children[NodeChild_Left] = NODE_ID;
		pNodeB->ParentID = pNodeA->ParentID;
		pNodeA->ParentID = nodeBID;

		if (pNodeB->ParentID != Node::NULL_NODE)
		{

			if (m_pNodes[pNodeB->ParentID].Children[NodeChild_Left] == NODE_ID)
			{
				m_pNodes[pNodeB->ParentID].Children[NodeChild_Left] = nodeBID;
			}
			else
			{
				_ASSERT(m_pNodes[pNodeB->ParentID].Children[NodeChild_Right] == NODE_ID);
				m_pNodes[pNodeB->ParentID].Children[NodeChild_Right] = nodeBID;
			}
		}
		else
		{
			m_RootNodeID = nodeBID;
		}

		_ASSERT(!pNodeB->IsLeaf());
		_ASSERT(!pNodeA->IsLeaf());

		if (pNodeF->Height > pNodeG->Height)
		{
			pNodeB->Children[NodeChild_Right] = nodeFID;
			pNodeA->Children[NodeChild_Left] = nodeGID;
			pNodeG->ParentID = NODE_ID;

			DirectX::BoundingBox::CreateMerged(pNodeA->AABB, pNodeC->AABB, pNodeG->AABB);
			DirectX::BoundingBox::CreateMerged(pNodeB->AABB, pNodeA->AABB, pNodeF->AABB);

			pNodeA->Height = Max(pNodeC->Height, pNodeG->Height) + 1;
			pNodeB->Height = Max(pNodeA->Height, pNodeF->Height) + 1;
			_ASSERT(pNodeA->Height > 0);
			_ASSERT(pNodeB->Height > 0);
		}
		else
		{
			pNodeB->Children[NodeChild_Right] = nodeGID;
			pNodeA->Children[NodeChild_Left] = nodeFID;
			pNodeF->ParentID = NODE_ID;

			DirectX::BoundingBox::CreateMerged(pNodeA->AABB, pNodeC->AABB, pNodeF->AABB);
			DirectX::BoundingBox::CreateMerged(pNodeB->AABB, pNodeA->AABB, pNodeG->AABB);

			pNodeA->Height = Max(pNodeC->Height, pNodeF->Height) + 1;
			pNodeB->Height = Max(pNodeA->Height, pNodeG->Height) + 1;
			_ASSERT(pNodeA->Height > 0);
			_ASSERT(pNodeB->Height > 0);
		}

		return nodeBID;
	}

	return NODE_ID;
}
