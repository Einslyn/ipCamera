
#include "TreeNode.h"
#include <assert.h>

CTreeNode::CTreeNode(unsigned int index)
{
	m_pParent		= NULL;
	m_pFirstChild	= NULL;
	m_pPrevSibling	= NULL;
	m_pNextSibling	= NULL;
	m_pLastChild	= NULL;

	m_nodeIndex		= index;
	m_pData			= NULL;
}

CTreeNode::~CTreeNode(void)
{
}

CTreeNode *CTreeNode::Parent()
{
	return m_pParent;
}


//无类型区分访问
CTreeNode *CTreeNode::PrevSibling()
{
	return m_pPrevSibling;
}

CTreeNode *CTreeNode::NextSibling()
{
	return m_pNextSibling;
}

CTreeNode *CTreeNode::FirstChild()
{
	return m_pFirstChild;
}

CTreeNode *CTreeNode::PrevSibling(const TREE_NODE_TYPE &nodeType)
{
	CTreeNode *pPrev = m_pPrevSibling;
	while(NULL != pPrev)
	{
		if(pPrev->NodeType() == nodeType)
		{
			return pPrev;
		}
	}

	return NULL;
}

CTreeNode *CTreeNode::NextSibling(const TREE_NODE_TYPE &nodeType)
{
	CTreeNode *pNext = m_pNextSibling;
	while(NULL != pNext)
	{
		if(pNext->NodeType() == nodeType)
		{
			return pNext;
		}
	}

	return NULL;
}

CTreeNode *CTreeNode::FirstChild(const TREE_NODE_TYPE &nodeType)
{
	CTreeNode *pChild = m_pFirstChild;
	while(NULL != pChild)
	{
		if(pChild->NodeType() == nodeType)
		{
			return pChild;
		}
		pChild = pChild->m_pNextSibling;
	}

	return NULL;
}

CTreeNode *CTreeNode::FindNode(unsigned int nodeIndex)
{
	if(m_nodeIndex == nodeIndex)
	{
		return this;
	}

	CTreeNode *pRetNode = NULL;
	CTreeNode *pChild = m_pFirstChild;
	while(NULL != pChild)
	{
		pRetNode = pChild->FindNode(nodeIndex);
		if(NULL != pRetNode)
		{
			return pRetNode;
		}
		pChild = pChild->m_pNextSibling;
	}

	return NULL;
}

CTreeNode *CTreeNode::InsertEndChild(const CTreeNode &node)
{
	CTreeNode *pNewNode = node.Clone();
	if (NULL == pNewNode)
	{
		return 0;
	}

	assert((0 == pNewNode->m_pParent) || (this == pNewNode->m_pParent));

	pNewNode->m_pParent = this;
	pNewNode->m_pPrevSibling = m_pLastChild;
	pNewNode->m_pNextSibling = NULL;

	if (NULL != m_pLastChild)
	{
		m_pLastChild->m_pNextSibling = pNewNode;
	}
	else
	{
		m_pFirstChild = pNewNode;
	}

	m_pLastChild = pNewNode;
	return pNewNode;
}

CTreeNode *CTreeNode::InsertBeforeChild(CTreeNode* pBeforeThis, const CTreeNode &node)
{
	//首先验证pBeforeThis确定在当前树中
	CTreeNode *pTempNode = m_pFirstChild;
	while(NULL != pTempNode)
	{
		if(pTempNode == pBeforeThis)
		{
			break;
		}
		pTempNode = pTempNode->m_pNextSibling;
	}
	if(NULL == pTempNode)
	{
		//在当前树的直接子接点中没有找到pBeforeThis
		assert(FALSE);
		return NULL;
	}


	CTreeNode *pNewNode = node.Clone();
	if (NULL == pNewNode)
	{
		return 0;
	}

	pNewNode->m_pParent = this;
	pNewNode->m_pPrevSibling = pBeforeThis->m_pPrevSibling;
	pNewNode->m_pNextSibling = pBeforeThis;
	if(NULL != pBeforeThis->m_pPrevSibling)
	{
		pBeforeThis->m_pPrevSibling->m_pNextSibling = pNewNode;
	}
	else
	{
		assert(m_pFirstChild == pBeforeThis);
		m_pFirstChild = pNewNode;
	}
	pBeforeThis->m_pPrevSibling = pNewNode;

	return pNewNode;
}

BOOL CTreeNode::RemoveChild(CTreeNode *pNode)
{
	//首先验证pBeforeThis确定在当前树中
	CTreeNode *pTempNode = m_pFirstChild;
	while(NULL != pTempNode)
	{
		if(pTempNode == pNode)
		{
			break;
		}
		pTempNode = pTempNode->m_pNextSibling;
	}
	if(NULL == pTempNode)
	{
		//在当前树的直接子接点中没有找到pBeforeThis
		assert(FALSE);
		return FALSE;
	}

	if (NULL != pNode->m_pNextSibling)
	{
		pNode->m_pNextSibling->m_pPrevSibling = pNode->m_pPrevSibling;
	}
	else
	{
		m_pLastChild = pNode->m_pPrevSibling;
	}

	if (NULL != pNode->m_pPrevSibling)
	{
		pNode->m_pPrevSibling->m_pNextSibling = pNode->m_pNextSibling;
	}
	else
	{
		m_pFirstChild = pNode->m_pNextSibling;
	}

	pNode->RemoveAllChild();
	delete pNode;

	return TRUE;
}

void CTreeNode::RemoveAllChild()
{
	CTreeNode *pNode = m_pFirstChild;
	while(NULL != pNode)
	{
		m_pFirstChild = pNode->m_pNextSibling;
		pNode->RemoveAllChild();
		delete pNode;

		pNode = m_pFirstChild;
	}
}

TREE_NODE_TYPE CTreeNode::NodeType() const
{
	return TREE_NODE_NONE;
}

void CTreeNode::Traverse(int (*pTraverseCall)(CTreeNode *pNode, void *pUser), void *pUser)
{
	CTreeNode *pNode = m_pFirstChild;

	while(NULL != pNode)
	{
		pTraverseCall(pNode, pUser);		//访问树节点pNode
		pNode->Traverse(pTraverseCall, pUser);//访问节点pNode的所有子树
		pNode = pNode->m_pNextSibling;
	}
}

CTreeNode *CTreeNode::Clone() const
{
	CTreeNode *pClone = new CTreeNode(m_nodeIndex);

	if (NULL == pClone)
	{
		return NULL;
	}

	return pClone;
}


CRegionNode::CRegionNode(const char *pRegionName, unsigned int index):CTreeNode(index)
{
	strncpy(m_regionName, pRegionName, sizeof(m_regionName));
	m_regionName[sizeof(m_regionName)-1] = '\0';
}

CRegionNode::~CRegionNode()
{

}

CTreeNode *CRegionNode::Clone() const
{
	CRegionNode *pClone = new CRegionNode(m_regionName, m_nodeIndex);

	if (NULL == pClone)
	{
		return NULL;
	}

	return pClone;
}


CDeviceNode::CDeviceNode(const char *pDeviceName, unsigned int index):CTreeNode(index)
{
	strncpy(m_deviceName, pDeviceName, sizeof(m_deviceName));
	m_deviceName[sizeof(m_deviceName)-1] = '\0';
}

CDeviceNode::~CDeviceNode()
{

}

CTreeNode *CDeviceNode::Clone() const
{
	CDeviceNode *pClone = new CDeviceNode(m_deviceName, m_nodeIndex);

	if (NULL == pClone)
	{
		return NULL;
	}

	pClone->m_serverPort = m_serverPort;
	strcpy(pClone->m_serverAddr, m_serverAddr);
	strcpy(pClone->m_userName, m_userName);
	strcpy(pClone->m_password, m_password);
	pClone->m_channelNum = m_channelNum;

	return pClone;
}

void CDeviceNode::SetNetInfo(unsigned short serverPort, const char *serverAddr)
{
	m_serverPort = serverPort;
	strcpy(m_serverAddr, serverAddr);
}
void CDeviceNode::SetUserInfo(const char *username, const char *passowrd)
{
	strcpy(m_userName, username);
	strcpy(m_password, passowrd);
}

void CDeviceNode::SetChannelNum(unsigned short channelNum)
{
	m_channelNum = channelNum;
}

BOOL CDeviceNode::GetNetInfo(unsigned short &serverPort, char *serverAddr)
{
	serverPort = m_serverPort;
	strcpy(serverAddr, m_serverAddr);

	return TRUE;
}

BOOL CDeviceNode::GetUserInfo(char *username, char *password)
{
	strcpy(username, m_userName);
	strcpy(password, m_password);

	return TRUE;
}

unsigned short CDeviceNode::GetChannelNum() const
{
	return m_channelNum;
}

