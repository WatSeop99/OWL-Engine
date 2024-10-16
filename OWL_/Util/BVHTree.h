#pragma once

#include <DirectXCollision.h>
#include <intsafe.h>

enum eNodeChild
{
	NodeChild_Left = 0,
	NodeChild_Right,
	NodeChild_Count
};

class Node
{
public:
	static const UINT NULL_NODE = 0xffffffff;

public:
	Node() = default;
	~Node() = default;

	inline bool IsLeaf() { return (Height == 0); }

public:
	UINT ParentID = NULL_NODE;
	UINT NextNodeID = NULL_NODE;
	UINT ID = NULL_NODE;
	UINT Children[NodeChild_Count] = { NULL_NODE, NULL_NODE };
	int Height = -1;
	DirectX::BoundingBox AABB;
};

class BVHTree
{
public:
	BVHTree() = default;
	~BVHTree() { Cleanup(); }

	void Initialize(const UINT MAX_NODE_NUM);

	void AddObject(const DirectX::BoundingBox* const pAABB);

	bool UpdateObject(const UINT NODE_ID, const DirectX::BoundingBox* const pNewAABB, bool bForceReinsert = false);

	void RemoveObject(const UINT NODE_ID);

	void Cleanup();

	inline Node* GetAllNodes() { return m_pNodes; }
	inline UINT GetRootNodeID() { return m_RootNodeID; }
	int GetMaxDepth();

protected:
	void addObjectInternal(const DirectX::BoundingBox* const pAABB);
	
	UINT allocateNode();
	void releaseNode(const UINT NODE_ID);

	void insertLeafNode(const UINT NODE_ID);
	void removeLeafNode(const UINT NODE_ID);
	UINT balanceSubTreeAtNode(const UINT NODE_ID);

private:
	Node* m_pNodes = nullptr;
	UINT m_RootNodeID = Node::NULL_NODE;
	UINT m_FreeNodeID = Node::NULL_NODE;
	UINT m_AllocatedNodes = 0;
	UINT m_TotalNodes = 0;
};
