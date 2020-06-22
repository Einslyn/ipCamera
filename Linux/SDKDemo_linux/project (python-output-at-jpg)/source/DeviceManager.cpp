#include <stdio.h>
#include <assert.h>
#include "DeviceManager.h"
#include <algorithm>
#include "DVR_NET_SDK.h"

#ifdef __ENVIRONMENT_WIN32__
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

CDeviceManager::CDeviceManager(void)
{
	  m_pRgnDevTree = NULL;
	m_nextID = RGN_DEV_INVALID_ID;

	m_threadID = PUB_THREAD_ID_NOINIT;
	m_bConnecting = false;
}

CDeviceManager::~CDeviceManager(void)
{
}

CDeviceManager *CDeviceManager::Instance()
{
	static CDeviceManager s_deviceMan;
	return &s_deviceMan;
}

BOOL CDeviceManager::Initial()
{
	m_pRgnDevTree = new CTreeNode(RGN_DEV_INVALID_ID);
	assert(NULL != m_pRgnDevTree);

	if(NULL == m_pRgnDevTree)
	{
		return FALSE;
	}

	if(!m_rgnDevLog.GetLogInfo(m_pRgnDevTree, m_groupInfo, m_channelInfo, MAX_CHANNEL_NUM, m_nextID))
	{
		delete m_pRgnDevTree;
		m_pRgnDevTree = NULL;

		return FALSE;
	}

	m_nextID += 1;

	return TRUE;
}

BOOL CDeviceManager::Start()
{

	m_threadID = PUB_CreateThread(ConnectThread, this, &m_bConnecting);

	if(PUB_CREATE_THREAD_FAIL == m_threadID)
	{
		return FALSE;
	}

	return TRUE;
}

void CDeviceManager::Stop()
{
	if(PUB_THREAD_ID_NOINIT != m_threadID)
	{
		PUB_ExitThread(&m_threadID, &m_bConnecting);
	}
}

void CDeviceManager::Quit()
{
	if(NULL != m_pRgnDevTree)
	{
		list<DEV_RGN_NODE> listDevRgn;

		m_deviceLock.Lock();
		m_pRgnDevTree->Traverse(GetRgnDevNodeCall, &listDevRgn);

		for(list<DEV_RGN_NODE>::iterator it = listDevRgn.begin(); it != listDevRgn.end(); it++)
		{
			if((*it).nodeType == TREE_NODE_DEVICE)
			{
				CTreeNode *pDevice = m_pRgnDevTree->FindNode((*it).selfID);
				if(NULL == pDevice)
				{
					continue;
				}
				if(NULL != pDevice->GetNodeData())
				{
					DEV_LINK_INFO *pDevLinkInfo = (DEV_LINK_INFO *)pDevice->GetNodeData();
					delete pDevLinkInfo;
				}
			}
		}

		m_pRgnDevTree->RemoveAllChild();
		delete m_pRgnDevTree;
		m_pRgnDevTree = NULL;

		m_deviceLock.UnLock();
	}

	m_groupInfo.clear();
}

BOOL CDeviceManager::RegionNameExist(const char *pRegionName)
{
	if(NULL == m_pRgnDevTree)
	{
		return FALSE;
	}

	FIND_NAME_HELPER finder;
	finder.bFound = FALSE;
	finder.nodeType = TREE_NODE_REGION;
	finder.pName = pRegionName;

	m_deviceLock.Lock();
	m_pRgnDevTree->Traverse(FindNameCall, &finder);
	m_deviceLock.UnLock();

	return finder.bFound;
}

BOOL CDeviceManager::DeviceNameExist(const char *pDeviceName)
{
	if(NULL == m_pRgnDevTree)
	{
		return FALSE;
	}

	FIND_NAME_HELPER finder;
	finder.bFound = FALSE;
	finder.nodeType = TREE_NODE_DEVICE;
	finder.pName = pDeviceName;

	m_deviceLock.Lock();
	m_pRgnDevTree->Traverse(FindNameCall, &finder);
	m_deviceLock.UnLock();

	return finder.bFound;
}

BOOL CDeviceManager::GroupNameExist(const char *pGroupName)
{
	m_deviceLock.Lock();
	for(int i = 0; i < m_groupInfo.size(); i++)
	{
		if(strcmp(m_groupInfo[i].groupName, pGroupName) == 0)
		{
			m_deviceLock.UnLock();
			return TRUE;
		}
	}

	m_deviceLock.UnLock();
	return FALSE;
}

BOOL CDeviceManager::DeviceExist(const char *sDVRIP, WORD wDVRPort)
{
	if(NULL == m_pRgnDevTree)
	{
		return FALSE;
	}

	FIND_NETPOS_HELPER finder;
	finder.bFound = FALSE;
	finder.pDVRAddr = sDVRIP;
	finder.wPort = wDVRPort;

	m_deviceLock.Lock();
	m_pRgnDevTree->Traverse(FindNetposCall, &finder);
	m_deviceLock.UnLock();

	return finder.bFound;
}

DWORD CDeviceManager::AddRegion(DWORD parentID, const char *pRegionName)
{
	if(NULL == m_pRgnDevTree)
	{
		return RGN_DEV_INVALID_ID;
	}

	if(RegionNameExist(pRegionName))
	{
		return RGN_DEV_INVALID_ID;
	}

	m_deviceLock.Lock();
	CRegionNode regionNode(pRegionName, m_nextID++);
	CTreeNode *pParent = m_pRgnDevTree->FindNode(parentID);
	if(NULL == pParent)
	{
		m_deviceLock.UnLock();
		return RGN_DEV_INVALID_ID;
	}

	CTreeNode *pFirstDevChild = pParent->FirstChild(TREE_NODE_DEVICE);
	if(NULL != pFirstDevChild)
	{
		pParent->InsertBeforeChild(pFirstDevChild, regionNode);
	}
	else
	{
		pParent->InsertEndChild(regionNode);
	}

	//同时写入数据库文件
	m_rgnDevLog.SetLogInfo(m_pRgnDevTree, m_groupInfo, m_channelInfo, MAX_CHANNEL_NUM);

	m_deviceLock.UnLock();

	return regionNode.NodeIndex();
}

DWORD CDeviceManager::AddDevice(DWORD parentID, const char *pDeviceName, char *sDVRIP,WORD wDVRPort,char *sUserName,char *sPassword, int channelNum)
{
	if(NULL == m_pRgnDevTree)
	{
		printf("\r\n**************** NULL == m_pRgnDevTree ***************\r\n");
		return RGN_DEV_INVALID_ID;
	}

	if(RGN_DEV_INVALID_ID == parentID)
	{
		printf("\r\n**************** RGN_DEV_INVALID_ID == parentID***************\r\n");
		return RGN_DEV_INVALID_ID;
	}

	m_deviceLock.Lock();
	CTreeNode *pParent = m_pRgnDevTree->FindNode(parentID);
	if(NULL == pParent)
	{
		printf("\r\n**************** NULL == pParent ***************\r\n");
		m_deviceLock.UnLock();
		return RGN_DEV_INVALID_ID;
	}
	m_deviceLock.UnLock();

	if(DeviceNameExist(pDeviceName) || DeviceExist(sDVRIP, wDVRPort))
	{
		printf("\r\n**************** DeviceNameExist(pDeviceName) || DeviceExist(sDVRIP, wDVRPort) ***************\r\n");
		return RGN_DEV_INVALID_ID;
	}

	CDeviceNode deviceNode(pDeviceName, m_nextID++);
	deviceNode.SetNetInfo(wDVRPort, sDVRIP);
	deviceNode.SetUserInfo(sUserName, sPassword);
	deviceNode.SetChannelNum(channelNum);

	m_deviceLock.Lock();
	BYTE addChannel = 0;
	for(BYTE i = 0; i < MAX_CHANNEL_NUM; i++)
	{
		if(addChannel >= channelNum)
		{
			break;
		}

		if(m_channelInfo[i].deviceID == RGN_DEV_INVALID_ID)
		{
			m_channelInfo[i].deviceID = deviceNode.NodeIndex();
			m_channelInfo[i].channel = addChannel;
			m_channelInfo[i].channelID = i;
			sprintf(m_channelInfo[i].channelName, "channel %02d", addChannel+1);
			addChannel++;
		}
	}

	if(addChannel > 0)
	{
		CTreeNode *pDevice = pParent->InsertEndChild(deviceNode);
		//同时写入数据库文件
		m_rgnDevLog.SetLogInfo(m_pRgnDevTree, m_groupInfo, m_channelInfo, MAX_CHANNEL_NUM);

		m_deviceLock.UnLock();
		return deviceNode.NodeIndex();
	}
	else
	{
		m_deviceLock.UnLock();
		return RGN_DEV_INVALID_ID;
	}
}

BOOL CDeviceManager::RemoveRegion(DWORD regionID)
{
	if(NULL == m_pRgnDevTree)
	{
		return FALSE;
	}

	m_deviceLock.Lock();
	CTreeNode *pNode = m_pRgnDevTree->FindNode(regionID);
	if(NULL == pNode)
	{
		m_deviceLock.UnLock();
		return FALSE;
	}

	assert(TREE_NODE_REGION == pNode->NodeType());

	pNode->Traverse(RemoveDeviceCall, this);

	pNode->Parent()->RemoveChild(pNode);

	//同时写入数据库文件
	m_rgnDevLog.SetLogInfo(m_pRgnDevTree, m_groupInfo, m_channelInfo, MAX_CHANNEL_NUM);

	m_deviceLock.UnLock();

	return TRUE;
}

BOOL CDeviceManager::RemoveDevice(DWORD deviceID)
{
	if(NULL == m_pRgnDevTree)
	{
		return FALSE;
	}

	m_deviceLock.Lock();
	CTreeNode *pNode = m_pRgnDevTree->FindNode(deviceID);
	if(NULL == pNode)
	{
		m_deviceLock.UnLock();
		return FALSE;
	}

	assert(TREE_NODE_DEVICE == pNode->NodeType());

	RemoveDeviceCall(pNode, this);

	pNode->Parent()->RemoveChild(pNode);

	//同时写入数据库文件
	m_rgnDevLog.SetLogInfo(m_pRgnDevTree, m_groupInfo, m_channelInfo, MAX_CHANNEL_NUM);

	m_deviceLock.UnLock();

	return TRUE;
}

BOOL CDeviceManager::ModifyRegion(DWORD regionID, const char *pNewName)
{
	if(NULL == m_pRgnDevTree)
	{
		return FALSE;
	}

	if(RegionNameExist(pNewName))
	{
		return FALSE;
	}

	m_deviceLock.Lock();
	CTreeNode *pNode = m_pRgnDevTree->FindNode(regionID);
	if(NULL == pNode)
	{
		m_deviceLock.UnLock();
		return FALSE;
	}

	assert(TREE_NODE_REGION == pNode->NodeType());

	pNode->ToRegion()->SetRegionName(pNewName);

	//同时写入数据库文件
	m_rgnDevLog.SetLogInfo(m_pRgnDevTree, m_groupInfo, m_channelInfo, MAX_CHANNEL_NUM);

	m_deviceLock.UnLock();

	return TRUE;
}

BOOL CDeviceManager::ModifyDevice(DWORD deviceID, const char *pDeviceName, char *sDVRIP,WORD wDVRPort,char *sUserName,char *sPassword)
{
	if(NULL == m_pRgnDevTree)
	{
		return FALSE;
	}

	m_deviceLock.Lock();
	CTreeNode *pNode = m_pRgnDevTree->FindNode(deviceID);
	if(NULL == pNode)
	{
		m_deviceLock.UnLock();
		return FALSE;
	}

	//新的设备名与其他设备名冲突，拒绝修改
	if(strcmp(pNode->ToDevice()->DeviceName(), pDeviceName) != 0)
	{
		if(DeviceNameExist(pDeviceName))
		{
			m_deviceLock.UnLock();
			return FALSE;
		}
	}

	//新设备网络地址与其他设备冲突，拒绝修改
	char sOldIP[64];
	WORD wOldPort;
	pNode->ToDevice()->GetNetInfo(wOldPort, sOldIP);
	if((strcmp(sOldIP, sDVRIP) != 0) || (wOldPort != wDVRPort))
	{
		if(DeviceExist(sDVRIP, wDVRPort))
		{
			m_deviceLock.UnLock();
			return FALSE;
		}
	}
	char sOldName[64];
	char sOldPwd[64];
	pNode->ToDevice()->GetUserInfo(sOldName, sOldPwd);

	assert(TREE_NODE_DEVICE == pNode->NodeType());

	pNode->ToDevice()->SetRegionName(pDeviceName);
	pNode->ToDevice()->SetNetInfo(wDVRPort, sDVRIP);
	pNode->ToDevice()->SetUserInfo(sUserName, sPassword);
	if ((strcmp(sOldIP, sDVRIP) != 0) || (wOldPort != wDVRPort)||(strcmp(sOldName, sUserName) != 0)||(strcmp(sOldPwd, sPassword) != 0))
	{//如果地址、端口、或用户名密码有改变则重新连一下
		void *pData = pNode->ToDevice()->GetNodeData();
		delete (DEV_LINK_INFO *)pData;
		pNode->ToDevice()->SetNodeData(NULL);
	}

	//同时写入数据库文件
	m_rgnDevLog.SetLogInfo(m_pRgnDevTree, m_groupInfo, m_channelInfo, MAX_CHANNEL_NUM);

	m_deviceLock.UnLock();

	return TRUE;
}

BOOL CDeviceManager::ModifyChannel(DWORD channelID, const char *pChannel)
{
	m_deviceLock.Lock();

	assert(channelID < MAX_CHANNEL_NUM);
	assert(m_channelInfo[channelID].channelID == channelID);
	if((channelID >= MAX_CHANNEL_NUM) || (m_channelInfo[channelID].channelID != channelID))
	{
		m_deviceLock.UnLock();

		return FALSE;
	}

	strcpy(m_channelInfo[channelID].channelName, pChannel);

	//同时写入数据库文件
	m_rgnDevLog.SetLogInfo(m_pRgnDevTree, m_groupInfo, m_channelInfo, MAX_CHANNEL_NUM);

	m_deviceLock.UnLock();

	return TRUE;
}

DWORD CDeviceManager::AddGroup(const char *pGroupName)
{
	if(NULL == m_pRgnDevTree)
	{
		return RGN_DEV_INVALID_ID;
	}

	if(GroupNameExist(pGroupName))
	{
		return RGN_DEV_INVALID_ID;
	}

	m_deviceLock.Lock();

	GROUP_INFO groupInfo;
	memset(groupInfo.groupChannel, INVALID_CHANNEL_ID, sizeof(groupInfo.groupChannel));
	groupInfo.groupID = m_nextID++;
	strcpy(groupInfo.groupName, pGroupName);
	m_groupInfo.push_back(groupInfo);

	//同时写入数据库文件
	m_rgnDevLog.SetLogInfo(m_pRgnDevTree, m_groupInfo, m_channelInfo, MAX_CHANNEL_NUM);

	m_deviceLock.UnLock();

	return groupInfo.groupID;
}

BOOL CDeviceManager::AddChannelToGroup(BYTE channelID, DWORD groupID)
{
	m_deviceLock.Lock();
	int ch;
	for(ch = 0; ch < MAX_CHANNEL_NUM; ch++)
	{
		if(m_channelInfo[ch].channelID == channelID)
		{
			if(m_channelInfo[ch].deviceID == RGN_DEV_INVALID_ID)
			{
				m_deviceLock.UnLock();
				return FALSE;
			}
			break;
		}
	}
	if(ch == MAX_CHANNEL_NUM)
	{
		m_deviceLock.UnLock();
		return FALSE;
	}

	for(int i = 0; i < m_groupInfo.size(); i++)
	{
		if(m_groupInfo[i].groupID == groupID)
		{
			//查看分组中是否已包含该通道
			int index;
			for(index = 0; index < MAX_CHANNEL_NUM; index++)
			{
				if(m_groupInfo[i].groupChannel[index] == channelID)
				{
					m_deviceLock.UnLock();
					return FALSE;
				}
			}

			//通道还没有加入到当前分组
			for(index = 0; index < MAX_CHANNEL_NUM; index++)
			{
				if(m_groupInfo[i].groupChannel[index] == INVALID_CHANNEL_ID)
				{
					m_groupInfo[i].groupChannel[index] = channelID;
					break;
				}
			}
			break;
		}
	}
	//同时写入数据库文件
	m_rgnDevLog.SetLogInfo(m_pRgnDevTree, m_groupInfo, m_channelInfo, MAX_CHANNEL_NUM);

	m_deviceLock.UnLock();

	return TRUE;
}

BOOL CDeviceManager::RemoveGroup(DWORD groupID)
{
	m_deviceLock.Lock();
	vector<GROUP_INFO>::iterator it = m_groupInfo.begin();
	for(; it != m_groupInfo.end(); it++)
	{
		if((*it).groupID == groupID)
		{
			m_groupInfo.erase(it);

			//同时写入数据库文件
			m_rgnDevLog.SetLogInfo(m_pRgnDevTree, m_groupInfo, m_channelInfo, MAX_CHANNEL_NUM);
			break;
		}
	}
	m_deviceLock.UnLock();

	return TRUE;
}

BOOL CDeviceManager::RemoveChannelFromGroup(BYTE channelID, unsigned groupID)
{
	m_deviceLock.Lock();
	for(int i = 0; i < m_groupInfo.size(); i++)
	{
		if(m_groupInfo[i].groupID == groupID)
		{
			for(int j = 0; j < MAX_CHANNEL_NUM; j++)
			{
				if(m_groupInfo[i].groupChannel[j] == channelID)
				{
					m_groupInfo[i].groupChannel[j] = INVALID_CHANNEL_ID;
					break;
				}
			}
			break;
		}
	}
	//同时写入数据库文件
	m_rgnDevLog.SetLogInfo(m_pRgnDevTree, m_groupInfo, m_channelInfo, MAX_CHANNEL_NUM);
	m_deviceLock.UnLock();

	return TRUE;
}

BOOL CDeviceManager::ModifyGroup(DWORD groupID, const char *pNewName)
{
	if(GroupNameExist(pNewName))
	{
		return FALSE;
	}

	m_deviceLock.Lock();
	for(int i = 0; i < m_groupInfo.size(); i++)
	{
		if(m_groupInfo[i].groupID == groupID)
		{
			strcpy(m_groupInfo[i].groupName, pNewName);
			break;
		}
	}
	//同时写入数据库文件
	m_rgnDevLog.SetLogInfo(m_pRgnDevTree, m_groupInfo, m_channelInfo, MAX_CHANNEL_NUM);
	m_deviceLock.UnLock();

	return TRUE;
}

TREE_NODE_TYPE CDeviceManager::GetNodeType(DWORD dwNodeID)
{
	if(NULL == m_pRgnDevTree)
	{
		assert(FALSE);
		return TREE_NODE_NONE;
	}

	m_deviceLock.Lock();
	CTreeNode *pNode = m_pRgnDevTree->FindNode(dwNodeID);
	if(NULL == pNode)
	{
		m_deviceLock.UnLock();
		return TREE_NODE_NONE;
	}

	TREE_NODE_TYPE nodeType = pNode->NodeType();
	m_deviceLock.UnLock();

	return nodeType;
}

BOOL CDeviceManager::GetDevRgnNode(list<DEV_RGN_NODE> &listDevRgn)
{
	if(NULL == m_pRgnDevTree)
	{
		assert(FALSE);
		return FALSE;
	}

	m_deviceLock.Lock();
	m_pRgnDevTree->Traverse(GetRgnDevNodeCall, &listDevRgn);
	m_deviceLock.UnLock();

	return TRUE;
}

BOOL CDeviceManager::GetDeviceInfo(DWORD deviceID, DEVICE_INFO &deviceInfo)
{
	m_deviceLock.Lock();
	CTreeNode *pNode = m_pRgnDevTree->FindNode(deviceID);
	if(NULL == pNode)
	{
		m_deviceLock.UnLock();
		assert(FALSE);
		return FALSE;
	}
	if(pNode->ToDevice() == NULL)
	{
		m_deviceLock.UnLock();
		return FALSE;
	}

	strcpy(deviceInfo.deviceName, pNode->ToDevice()->DeviceName());
	deviceInfo.deviceID = pNode->NodeIndex();
	deviceInfo.parentID = pNode->Parent()->NodeIndex();
	deviceInfo.channelNum = pNode->ToDevice()->GetChannelNum();
	pNode->ToDevice()->GetNetInfo(deviceInfo.serverPort, deviceInfo.serverAddr);
	pNode->ToDevice()->GetUserInfo(deviceInfo.userName, deviceInfo.password);

	m_deviceLock.UnLock();

	return TRUE;
}

LONG CDeviceManager::GetDeviceChannel(DWORD deviceID, CHANNEL_INFO *pChannelInfo, LONG channelNum)
{
	m_deviceLock.Lock();
	LONG maxChannel = 0;
	for(int i = 0; i < MAX_CHANNEL_NUM; i++)
	{
		if(m_channelInfo[i].deviceID == deviceID)
		{
			assert(m_channelInfo[i].channel < channelNum);
			pChannelInfo[m_channelInfo[i].channel] = m_channelInfo[i];
			maxChannel = (maxChannel < m_channelInfo[i].channel) ? m_channelInfo[i].channel : maxChannel;
		}
	}
	m_deviceLock.UnLock();

	return maxChannel+1;
}

BOOL CDeviceManager::GetChannelInfo(DWORD dwChannelID, CHANNEL_INFO &channelInfo)
{
	m_deviceLock.Lock();
	for(int i = 0; i < MAX_CHANNEL_NUM; i++)
	{
		if(m_channelInfo[i].channelID == dwChannelID)
		{
			channelInfo = m_channelInfo[i];

			m_deviceLock.UnLock();
			return TRUE;
		}
	}

	m_deviceLock.UnLock();
	return FALSE;
}

BOOL CDeviceManager::GetGroupNode(vector<GROUP_INFO> &groupInfo)
{
	m_deviceLock.Lock();
	for(int i = 0; i < m_groupInfo.size(); i++)
	{
		groupInfo.push_back(m_groupInfo[i]);
	}
	m_deviceLock.UnLock();

	return TRUE;
}

LONG CDeviceManager::GetGroupChannelInfo(DWORD groupID, CHANNEL_INFO *pChannelInfo, LONG channelNum)
{
	m_deviceLock.Lock();
	assert(pChannelInfo != NULL);

	int groupChannelNum = 0;
	for(int i = 0; i < m_groupInfo.size(); i++)
	{
		if(m_groupInfo[i].groupID == groupID)
		{
			for(int j = 0; j < MAX_CHANNEL_NUM; j++)
			{
				if(m_groupInfo[i].groupChannel[j] != INVALID_CHANNEL_ID)
				{
					assert(m_channelInfo[m_groupInfo[i].groupChannel[j]].channelID == m_groupInfo[i].groupChannel[j]);
					pChannelInfo[groupChannelNum] = m_channelInfo[m_groupInfo[i].groupChannel[j]];
					groupChannelNum++;
					if(groupChannelNum >= channelNum)
					{
						m_deviceLock.UnLock();
						return groupChannelNum;
					}
				}
			}

			m_deviceLock.UnLock();
			return groupChannelNum;
		}
	}

	m_deviceLock.UnLock();
	return 0;
}

LONG CDeviceManager::StartLivePlay(DWORD channelID, LONG streamType, LIVE_DATA_CALLBACK fLiveDataCallBack, void* pUser)
{
	m_deviceLock.Lock();
	for(int i = 0; i < MAX_CHANNEL_NUM; i++)
	{
		if(m_channelInfo[i].channelID == channelID)
		{
			CTreeNode *pNode = m_pRgnDevTree->FindNode(m_channelInfo[i].deviceID);
			if(NULL == pNode)
			{
				m_deviceLock.UnLock();
				assert(FALSE);
				return -1;
			}
			if(pNode->ToDevice() == NULL)
			{
				m_deviceLock.UnLock();
				assert(FALSE);
				return -1;
			}

			DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pNode->GetNodeData();
			if(pLinkInfo != NULL)
			{
				NET_SDK_CLIENTINFO clientInfo;
				clientInfo.lChannel = m_channelInfo[i].channel;
				clientInfo.streamType = streamType;
				clientInfo.hPlayWnd = NULL;
                LONG ret = NET_SDK_LivePlay(pLinkInfo->lLoginID, &clientInfo, fLiveDataCallBack, pUser);
                printf("playHandle:%ld, lastError:%d \n", ret, NET_SDK_GetLastError());
				m_deviceLock.UnLock();
				return ret;
			}
			else
			{
				printf("plinkInfo == NULL!\n");
			}

			m_deviceLock.UnLock();
			return -1;
		}
	}

	m_deviceLock.UnLock();
	return -1;
}

BOOL CDeviceManager::GetVideoColor(DWORD channelID, LONG lChannel, DWORD *pBrightValue, DWORD *pContrastValue, DWORD *pSaturationValue, DWORD *pHueValue)
{
	m_deviceLock.Lock();
	CTreeNode *pDeviceNode = m_pRgnDevTree->FindNode(channelID);
	if((NULL == pDeviceNode) || (NULL == pDeviceNode->ToDevice()))
	{
		m_deviceLock.UnLock();
		assert(FALSE);
		return -1;
	}

	DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pDeviceNode->GetNodeData();
	if(pLinkInfo != NULL)
	{
		int ret = NET_SDK_GetVideoEffect(pLinkInfo->lLoginID, lChannel, pBrightValue, pContrastValue, pSaturationValue, pHueValue);
		m_deviceLock.UnLock();
		return ret;
	}

	m_deviceLock.UnLock();
	return -1;

}

BOOL CDeviceManager::GetDefualtVideoColor(DWORD channelID, DWORD *pBrightValue, DWORD *pContrastValue, DWORD *pSaturationValue, DWORD *pHueValue)
{
	m_deviceLock.Lock();
	CTreeNode *pDeviceNode = m_pRgnDevTree->FindNode(channelID);
	if((NULL == pDeviceNode) || (NULL == pDeviceNode->ToDevice()))
	{
		m_deviceLock.UnLock();
		assert(FALSE);
		return -1;
	}

	DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pDeviceNode->GetNodeData();
	if(pLinkInfo != NULL)
	{
		int ret = NET_SDK_GetDefaultVideoEffect(pLinkInfo->lLoginID,  pBrightValue, pContrastValue, pSaturationValue, pHueValue);
		m_deviceLock.UnLock();
		return ret;
	}

	m_deviceLock.UnLock();
	return -1;

}

BOOL CDeviceManager::SetVideoColor(DWORD channelID, LONG lChannel, DWORD dwBrightValue, DWORD dwContrastValue, DWORD dwSaturationValue, DWORD dwpHueValue)
{
	m_deviceLock.Lock();
	CTreeNode *pDeviceNode = m_pRgnDevTree->FindNode(channelID);
	if((NULL == pDeviceNode) || (NULL == pDeviceNode->ToDevice()))
	{
		m_deviceLock.UnLock();
		assert(FALSE);
		return -1;
	}

	DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pDeviceNode->GetNodeData();
	if(pLinkInfo != NULL)
	{
		int ret = NET_SDK_SetVideoEffect(pLinkInfo->lLoginID, lChannel, dwBrightValue, dwContrastValue, dwSaturationValue, dwpHueValue);
		m_deviceLock.UnLock();
		return ret;
	}

	m_deviceLock.UnLock();
	return -1;
}

BOOL CDeviceManager::SaveVideoColor(DWORD channelID, LONG lChannel, DWORD dwBrightValue, DWORD dwContrastValue, DWORD dwSaturationValue, DWORD dwpHueValue)
{
	m_deviceLock.Lock();
	CTreeNode *pDeviceNode = m_pRgnDevTree->FindNode(channelID);
	if((NULL == pDeviceNode) || (NULL == pDeviceNode->ToDevice()))
	{
		m_deviceLock.UnLock();
		assert(FALSE);
		return -1;
	}

	DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pDeviceNode->GetNodeData();
	if(pLinkInfo != NULL)
	{
		int ret = NET_SDK_SaveVideoEffect(pLinkInfo->lLoginID, lChannel, dwBrightValue, dwContrastValue, dwSaturationValue, dwpHueValue);
		m_deviceLock.UnLock();
		return ret;
	}

	m_deviceLock.UnLock();
	return -1;
}

BOOL CDeviceManager::StopLivePlay(LONG lLiveHandle)
{
	return NET_SDK_StopLivePlay(lLiveHandle);
}

LONG CDeviceManager::PlaybackByTime(LONG deviceID, LONG *pChannels, LONG channelNum, unsigned int startTime, unsigned int stopTime)
{
	m_deviceLock.Lock();
	
	CTreeNode *pDeviceNode = m_pRgnDevTree->FindNode(deviceID);
	if((NULL == pDeviceNode) || (NULL == pDeviceNode->ToDevice()))
	{
		m_deviceLock.UnLock();
		assert(FALSE);
		return -1;
	}

	DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pDeviceNode->GetNodeData();
	if(pLinkInfo != NULL)
	{
		DD_TIME netStartTime, netStopTime;
		DVRTimeToNetTime(startTime, netStartTime);
		DVRTimeToNetTime(stopTime, netStopTime);
		int ret = NET_SDK_PlayBackByTime(pLinkInfo->lLoginID, pChannels, channelNum, &netStartTime, &netStopTime, NULL);

		m_deviceLock.UnLock();
		return ret;
	}

	m_deviceLock.UnLock();
	return -1;
}
LONG CDeviceManager::GetFileByTime(LONG deviceID, LONG channel, unsigned int startTime, unsigned int stopTime, char* fileName)
{
	m_deviceLock.Lock();

	CTreeNode *pDeviceNode = m_pRgnDevTree->FindNode(deviceID);
	if((NULL == pDeviceNode) || (NULL == pDeviceNode->ToDevice()))
	{
		m_deviceLock.UnLock();
		assert(FALSE);
		return -1;
	}

	DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pDeviceNode->GetNodeData();
	if(pLinkInfo != NULL)
	{
		DD_TIME netStartTime, netStopTime;
		DVRTimeToNetTime(startTime, netStartTime);
		DVRTimeToNetTime(stopTime, netStopTime);
		int ret = NET_SDK_GetFileByTime(pLinkInfo->lLoginID, channel, &netStartTime, &netStopTime, fileName);

		m_deviceLock.UnLock();
		return ret;
	}

	m_deviceLock.UnLock();
	return -1;
}

DWORD CDeviceManager::GetLastError()
{
	return NET_SDK_GetLastError();
}
BOOL CDeviceManager::StopPlayback(LONG lPlayHandle)
{
	return NET_SDK_StopPlayBack(lPlayHandle);
}

LONG CDeviceManager::GetFilePos(LONG lHandle)
{
	return NET_SDK_GetDownloadPos(lHandle);
}

BOOL CDeviceManager::StopDownLoad(LONG lHandle)
{
	return NET_SDK_StopGetFile(lHandle);
}


BOOL CDeviceManager::SearchRecDate(LONG deviceID, list<DD_DATE> &recDataList)
{
	m_deviceLock.Lock();

	CTreeNode *pDeviceNode = m_pRgnDevTree->FindNode(deviceID);
	if((NULL == pDeviceNode) || (NULL == pDeviceNode->ToDevice()) || (NULL == pDeviceNode->GetNodeData()))
	{
		m_deviceLock.UnLock();
//		assert(FALSE);
		return FALSE;
	}

	DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pDeviceNode->GetNodeData();

	LONG lFindHandle = NET_SDK_FindRecDate(pLinkInfo->lLoginID);
	if(lFindHandle < 0)
	{
		m_deviceLock.UnLock();
		return FALSE;
	}

	DD_DATE recDate;
	while(NET_SDK_FindNextRecDate(lFindHandle, &recDate) == NET_SDK_FILE_SUCCESS)
	{
		recDataList.push_back(recDate);
	}
	NET_SDK_FindRecDateClose(lFindHandle);
	m_deviceLock.UnLock();
	return TRUE;
}

BOOL CDeviceManager::SearchFile(LONG deviceID, LONG lChannel, unsigned int startTime, unsigned int stopTime, \
								list<DD_REC_FILE> &recFileList)
{
	m_deviceLock.Lock();

	CTreeNode *pDeviceNode = m_pRgnDevTree->FindNode(deviceID);
	if((NULL == pDeviceNode) || (NULL == pDeviceNode->ToDevice()) || (NULL == pDeviceNode->GetNodeData()))
	{
		m_deviceLock.UnLock();
		assert(FALSE);
		return FALSE;
	}

	DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pDeviceNode->GetNodeData();
	
	DD_TIME netStartTime, netStopTime;
	DVRTimeToNetTime(startTime, netStartTime);
	DVRTimeToNetTime(stopTime, netStopTime);

	LONG lFindHandle = NET_SDK_FindFile(pLinkInfo->lLoginID, lChannel, &netStartTime, &netStopTime);
	if(lFindHandle < 0)
	{
		m_deviceLock.UnLock();
		return FALSE;
	}

	NET_SDK_REC_FILE netRecFile;
	DD_REC_FILE ddRecFile;
	while(NET_SDK_FindNextFile(lFindHandle, &netRecFile) == NET_SDK_FILE_SUCCESS)
	{
		ddRecFile.deviceID = deviceID;
		ddRecFile.dwChannel = lChannel;
		ddRecFile.startTime = NetTimeToDVRTime(netRecFile.startTime);
		ddRecFile.stopTime = NetTimeToDVRTime(netRecFile.stopTime);
		ddRecFile.bFileLocked = netRecFile.bFileLocked;
		ddRecFile.dwPartition = netRecFile.dwPartition;
		ddRecFile.dwFileIndex = netRecFile.dwFileIndex;
		recFileList.push_back(ddRecFile);
	}
	NET_SDK_FindClose(lFindHandle);
	m_deviceLock.UnLock();
	return TRUE;
}

BOOL CDeviceManager::SearchEvent(LONG deviceID, LONG lChannel, DWORD dwRecType, unsigned int startTime, unsigned int stopTime,\
								 list<DD_REC_EVENT> &recEventList)
{
	m_deviceLock.Lock();

	CTreeNode *pDeviceNode = m_pRgnDevTree->FindNode(deviceID);
	if((NULL == pDeviceNode) || (NULL == pDeviceNode->ToDevice()) || (NULL == pDeviceNode->GetNodeData()))
	{
		m_deviceLock.UnLock();
		assert(FALSE);
		return FALSE;
	}

	DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pDeviceNode->GetNodeData();

	DD_TIME netStartTime, netStopTime;
	DVRTimeToNetTime(startTime, netStartTime);
	DVRTimeToNetTime(stopTime, netStopTime);

	LONG lFindHandle = NET_SDK_FindEvent(pLinkInfo->lLoginID, lChannel, dwRecType, &netStartTime, &netStopTime);
	if(lFindHandle < 0)
	{
		m_deviceLock.UnLock();
		return FALSE;
	}

	NET_SDK_REC_EVENT netRecEvent;
	DD_REC_EVENT ddRecEvent;
	while(NET_SDK_FindNextEvent(lFindHandle, &netRecEvent) == NET_SDK_FILE_SUCCESS)
	{
		ddRecEvent.deviceID = deviceID;
		ddRecEvent.dwChannel = lChannel;
		ddRecEvent.dwRecType = netRecEvent.dwRecType;
		ddRecEvent.startTime = NetTimeToDVRTime(netRecEvent.startTime);
		ddRecEvent.stopTime = NetTimeToDVRTime(netRecEvent.stopTime);
		recEventList.push_back(ddRecEvent);
	}
	NET_SDK_FindEventClose(lFindHandle);
	m_deviceLock.UnLock();
	return TRUE;
}

BOOL CDeviceManager::SearchTime(LONG deviceID,LONG lChannel, unsigned int startTime, unsigned int stopTime, \
								list<DD_REC_TIME> &recTimeList)
{
	m_deviceLock.Lock();

	CTreeNode *pDeviceNode = m_pRgnDevTree->FindNode(deviceID);
	if((NULL == pDeviceNode) || (NULL == pDeviceNode->ToDevice()) || (NULL == pDeviceNode->GetNodeData()))
	{
		m_deviceLock.UnLock();
		printf("deviceID = %d\n", deviceID);
		if(pDeviceNode != NULL)
		{
			printf("pDeviceNode = %p\n", pDeviceNode);
			if(pDeviceNode->ToDevice() != NULL)
			{
				printf("Is Device, nodeData = %p\n", pDeviceNode->GetNodeData());
			}
			else
			{
				printf("nodeType = %d\n", pDeviceNode->NodeType());
			}
		}
		else
		{
			printf("No device has the DeviceID %d\n", deviceID);
		}

		assert(FALSE);
		return FALSE;
	}

	DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pDeviceNode->GetNodeData();

	DD_TIME netStartTime, netStopTime;
	DVRTimeToNetTime(startTime, netStartTime);
	DVRTimeToNetTime(stopTime, netStopTime);

	LONG lFindHandle = NET_SDK_FindTime(pLinkInfo->lLoginID, lChannel, &netStartTime, &netStopTime);
	if(lFindHandle < 0)
	{
		m_deviceLock.UnLock();
		return FALSE;
	}

	NET_SDK_REC_TIME netRecTime;
	DD_REC_TIME ddRecTime;
	while(NET_SDK_FindNextTime(lFindHandle, &netRecTime) == NET_SDK_FILE_SUCCESS)
	{
		ddRecTime.deviceID = deviceID;
		ddRecTime.dwChannel = lChannel;
		ddRecTime.startTime = NetTimeToDVRTime(netRecTime.startTime);
		ddRecTime.stopTime = NetTimeToDVRTime(netRecTime.stopTime);
		recTimeList.push_back(ddRecTime);
	}
	NET_SDK_FindTimeClose(lFindHandle);

	m_deviceLock.UnLock();
	return TRUE;
}

BOOL CDeviceManager::LockFile(LONG deviceID, NET_SDK_REC_FILE* pRecFile, LONG fileNum)
{
	m_deviceLock.Lock();

	CTreeNode *pDeviceNode = m_pRgnDevTree->FindNode(deviceID);
	if((NULL == pDeviceNode) || (NULL == pDeviceNode->ToDevice()) || (NULL == pDeviceNode->GetNodeData()))
	{
		m_deviceLock.UnLock();
		assert(FALSE);
		return FALSE;
	}

	DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pDeviceNode->GetNodeData();
	BOOL ret = NET_SDK_LockFile(pLinkInfo->lLoginID, pRecFile, fileNum);
	if (ret)
	{
		m_deviceLock.UnLock();
		return ret;
	}

	m_deviceLock.UnLock();
	return TRUE;

}

BOOL CDeviceManager::DeleteFile(LONG deviceID, NET_SDK_REC_FILE* pRecFile, LONG fileNum)
{
	m_deviceLock.Lock();

	CTreeNode *pDeviceNode = m_pRgnDevTree->FindNode(deviceID);
	if((NULL == pDeviceNode) || (NULL == pDeviceNode->ToDevice()) || (NULL == pDeviceNode->GetNodeData()))
	{
		m_deviceLock.UnLock();
		assert(FALSE);
		return FALSE;
	}

	DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pDeviceNode->GetNodeData();
	BOOL ret = NET_SDK_DeleteRecFile(pLinkInfo->lLoginID, pRecFile, fileNum);
	if (ret)
	{
		m_deviceLock.UnLock();
		return ret;
	}

	m_deviceLock.UnLock();
	return TRUE;

}

BOOL CDeviceManager::UnlockFile(LONG deviceID, NET_SDK_REC_FILE* pRecFile, LONG fileNum)
{
	m_deviceLock.Lock();

	CTreeNode *pDeviceNode = m_pRgnDevTree->FindNode(deviceID);
	if((NULL == pDeviceNode) || (NULL == pDeviceNode->ToDevice()) || (NULL == pDeviceNode->GetNodeData()))
	{
		m_deviceLock.UnLock();
		assert(FALSE);
		return FALSE;
	}

	DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pDeviceNode->GetNodeData();
	BOOL ret = NET_SDK_UnlockFile(pLinkInfo->lLoginID, pRecFile, fileNum);
	if (ret)
	{
		m_deviceLock.UnLock();
		return ret;
	}

	m_deviceLock.UnLock();
	return FALSE;

}

BOOL CDeviceManager::PTZControl(LONG deviceID, LONG channel, LONG command, LONG value, LONG speed)
{
	m_deviceLock.Lock();

	CTreeNode *pDeviceNode = m_pRgnDevTree->FindNode(deviceID);
	if((NULL == pDeviceNode) || (NULL == pDeviceNode->ToDevice()) || (NULL == pDeviceNode->GetNodeData()))
	{
		m_deviceLock.UnLock();
		assert(FALSE);
		return FALSE;
	}

	DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pDeviceNode->GetNodeData();
	BOOL ret =  FALSE;	
	switch (command)
	{
	case 	PTZ_CMD_STOP:
	case	PTZ_CMD_LEFT:				
	case	PTZ_CMD_RIGHT:				
	case	PTZ_CMD_UP:					
	case	PTZ_CMD_DOWN:				
	case	PTZ_CMD_LEFT_UP:			
	case	PTZ_CMD_LEFT_DOWN:			
	case	PTZ_CMD_RIGHT_UP:			
	case	PTZ_CMD_RIGHT_DOWN:			
	case	PTZ_CMD_NEAR:				
	case	PTZ_CMD_FAR:				
	case	PTZ_CMD_ZOOM_OUT:			
	case	PTZ_CMD_ZOOM_IN:			
	case	PTZ_CMD_IRIS_OPEN:			
	case	PTZ_CMD_IRIS_CLOSE:	
//	case	PTZ_CMD_TRACK_DEL:
		ret = NET_SDK_PTZControl_Other(pLinkInfo->lLoginID, channel, command, speed);
		break;
	case	PTZ_CMD_TRACK_START:
	case	PTZ_CMD_TRACK_STOP:
//	case	PTZ_CMD_TRACK_SET:
		ret = NET_SDK_PTZTrack_Other(pLinkInfo->lLoginID, channel, command);
		break;
	case	PTZ_CMD_AUTO_SCAN_START:
	case	PTZ_CMD_AUTO_SCAN_STOP:
		ret = NET_SDK_PTZAutoScan_Other(pLinkInfo->lLoginID, channel, command);
		break;
	case	PTZ_CMD_PRESET_GO:
	case	PTZ_CMD_PRESET_SET:
	case	PTZ_CMD_PRESET_DEL:
		ret = NET_SDK_PTZPreset_Other(pLinkInfo->lLoginID, channel, command, value);
		break;
	case	PTZ_CMD_CRUISE_RUN:
	case	PTZ_CMD_CRUISE_STOP:
		ret = NET_SDK_PTZCruise_Other(pLinkInfo->lLoginID, channel, command, value);
		break;
	default:
		break;
	}
	if (ret)
	{
		m_deviceLock.UnLock();
		return ret;
	}

	m_deviceLock.UnLock();
	return FALSE;
}

BOOL CDeviceManager::PTZSetCruise(LONG deviceID, LONG channel, BYTE byCruiseIndex, DD_CRUISE_POINT_INFO *pCruisePoint, int pointNum)
{
	m_deviceLock.Lock();

	CTreeNode *pDeviceNode = m_pRgnDevTree->FindNode(deviceID);
	if((NULL == pDeviceNode) || (NULL == pDeviceNode->ToDevice()) || (NULL == pDeviceNode->GetNodeData()))
	{
		m_deviceLock.UnLock();
		assert(FALSE);
		return FALSE;
	}

	DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pDeviceNode->GetNodeData();
	BOOL ret = NET_SDK_PTZSetCruise_Other(pLinkInfo->lLoginID, channel, byCruiseIndex, pCruisePoint, pointNum);

	m_deviceLock.UnLock();

	return ret;
}

//capture JPEG data
BOOL CDeviceManager::CaptureJPEGData(LONG lUserID, LONG lChannel, char *sJpegPicBuffer, DWORD dwPicSize,  LPDWORD lpSizeReturned)
{
	//deviceID要减去前面所有的区域
	//即比deviceID值小的所有区域个数
	//因为显示的区域ID和设备ID仅仅是保存在DEMO数据库
	//而内存中的设备ID是SDK中自己维护的
	//SDK不包含区域
	list<DEV_RGN_NODE> listDevRgn;
	if (!GetDevRgnNode(listDevRgn))
	{
		return FALSE;
	}
	DWORD dwRGNNum = 0;
	printf("iterator: parentID | selfID | nodeType(Region/Device/None) | devRgnName\n");
	for(list<DEV_RGN_NODE>::iterator it = listDevRgn.begin(); it != listDevRgn.end(); it++)
	{
		if (TREE_NODE_REGION == it->nodeType && it->selfID < lUserID)
		{
			dwRGNNum++;
		}
		printf("iterator: %d\t\t%d\t\t%d\t%s\n",it->parentID, it->selfID, it->nodeType, it->devRgnName);
	}
	lUserID -= dwRGNNum;
	if (lUserID <= 0)
	{
		return FALSE;
	}
	printf("SDK lUserID = %d, lChannel = %d\n",lUserID, lChannel);
#if 0
	return NET_SDK_CaptureJPEGData_V2(lUserID, lChannel, sJpegPicBuffer, dwPicSize, lpSizeReturned);
#else
	  BOOL bResult = FALSE;
		LPNET_SDK_JPEGPARA lpJpegPara;
		//bResult = NET_SDK_CaptureJPEGData_V2(lUserID, lChannel, sJpegPicBuffer, dwPicSize, lpSizeReturned);
		bResult = NET_SDK_CaptureJPEGData_V2(lUserID, lChannel, sJpegPicBuffer, dwPicSize, lpSizeReturned);
		printf("[%s] line:%d  NET_SDK_GetLastError:%d \n", __FILE__, __LINE__, NET_SDK_GetLastError());

		return bResult;
#endif
}

BOOL CDeviceManager::SetDVRConfig(LONG deviceID, DWORD dwCommand, LONG lChannel, LPVOID lpInBuffer, DWORD dwInBufferSize)
{
	m_deviceLock.Lock();

	CTreeNode *pDeviceNode = m_pRgnDevTree->FindNode(deviceID);
	if((NULL == pDeviceNode) || (NULL == pDeviceNode->ToDevice()) || (NULL == pDeviceNode->GetNodeData()))
	{
		m_deviceLock.UnLock();
		assert(FALSE);
		return FALSE;
	}

	DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pDeviceNode->GetNodeData();
	BOOL bRet = NET_SDK_SetDVRConfig(pLinkInfo->lLoginID, dwCommand, lChannel, lpInBuffer, dwInBufferSize);
	if (!bRet)
	{
		m_deviceLock.UnLock();
		return FALSE;
	}
	m_deviceLock.UnLock();
	return TRUE;
}

LONG CDeviceManager::EnterDVRConfig(LONG deviceID)
{
	m_deviceLock.Lock();

	if (m_pRgnDevTree == NULL)
	{
		m_deviceLock.UnLock();
		return -2;
	}

	CTreeNode *pDeviceNode = m_pRgnDevTree->FindNode(deviceID);
	if((NULL == pDeviceNode) || (NULL == pDeviceNode->ToDevice()) || (NULL == pDeviceNode->GetNodeData()))
	{
		m_deviceLock.UnLock();
		//assert(FALSE);
		return -2;
	}

	DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pDeviceNode->GetNodeData();
	LONG ret = NET_SDK_EnterDVRConfig(pLinkInfo->lLoginID);
	m_deviceLock.UnLock();
	return ret;
}

LONG CDeviceManager::GetDVRConfig(LONG deviceID, DWORD dwCommand, LONG lChannel, LPVOID lpOutBuffer, DWORD dwOutBufferSize, BOOL bDefaultConfig)
{
	m_deviceLock.Lock();
	
	if (m_pRgnDevTree == NULL)
	{
		m_deviceLock.UnLock();
		return -2;
	}

	CTreeNode *pDeviceNode = m_pRgnDevTree->FindNode(deviceID);
	if((NULL == pDeviceNode) || (NULL == pDeviceNode->ToDevice()) || (NULL == pDeviceNode->GetNodeData()))
	{
		m_deviceLock.UnLock();
		//assert(FALSE);
		return -2;
	}

	DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pDeviceNode->GetNodeData();
	DWORD dwBytesReturned;
	BOOL ret = NET_SDK_GetDVRConfig(pLinkInfo->lLoginID, dwCommand, lChannel, lpOutBuffer, dwOutBufferSize, &dwBytesReturned, bDefaultConfig);
	if (ret <= 0)
	{
		m_deviceLock.UnLock();
		return ret;
	}
	m_deviceLock.UnLock();
	return dwBytesReturned;
}

BOOL CDeviceManager::ExitDVRConfig(LONG deviceID)
{
	m_deviceLock.Lock();

	if (m_pRgnDevTree == NULL)
	{
		m_deviceLock.UnLock();
		return FALSE;
	}
	CTreeNode *pDeviceNode = m_pRgnDevTree->FindNode(deviceID);
	if((NULL == pDeviceNode) || (NULL == pDeviceNode->ToDevice()) || (NULL == pDeviceNode->GetNodeData()))
	{
		m_deviceLock.UnLock();
		//assert(FALSE);
		return FALSE;
	}

	DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pDeviceNode->GetNodeData();
	BOOL ret = NET_SDK_ExitDVRConfig(pLinkInfo->lLoginID);
	m_deviceLock.UnLock();
	return ret;
}

LONG CDeviceManager::GetLoginIDToConfig()
{
	return m_deivceID;
}

void CDeviceManager::SetDeviceID(LONG deviceID)
{
	m_deivceID = deviceID;
}

int CDeviceManager::RemoveDeviceCall(CTreeNode *pNode, void *pUser)
{
	CDeviceManager *pThis = (CDeviceManager *)pUser;

	if(pNode->NodeType() == TREE_NODE_DEVICE)
	{
		DWORD devIndex = pNode->ToDevice()->NodeIndex();
		for(int i = 0; i < MAX_CHANNEL_NUM; i++)
		{
			if(pThis->m_channelInfo[i].deviceID == devIndex)
			{
				pThis->m_channelInfo[i].deviceID = RGN_DEV_INVALID_ID;
				for(int j = 0; j < pThis->m_groupInfo.size(); j++)
				{
					for(int k = 0; k < MAX_CHANNEL_NUM; k++)
					{
						if(pThis->m_groupInfo[j].groupChannel[k] == pThis->m_channelInfo[i].channelID)
						{
							pThis->m_groupInfo[j].groupChannel[k] = INVALID_CHANNEL_ID;
						}
					}
				}
			}
		}

		DEV_LINK_INFO *pDevLinkInfo = (DEV_LINK_INFO *)pNode->GetNodeData();
//		assert(pDevLinkInfo != NULL);

		if(NULL != pDevLinkInfo)
		{
			delete pDevLinkInfo;
		}
	}

	return 0;
}

int CDeviceManager::GetRgnDevNodeCall(CTreeNode *pNode, void *pUser)
{
	list<DEV_RGN_NODE> *pList = (list<DEV_RGN_NODE> *)pUser;

	DEV_RGN_NODE devRgnNode;

	if(pNode->NodeType() == TREE_NODE_REGION)
	{
		strcpy(devRgnNode.devRgnName, pNode->ToRegion()->RegionName());
		devRgnNode.parentID = pNode->Parent()->NodeIndex();
		devRgnNode.selfID = pNode->NodeIndex();
		devRgnNode.nodeType = pNode->NodeType();
	}
	else if(pNode->NodeType() == TREE_NODE_DEVICE)
	{
		strcpy(devRgnNode.devRgnName, pNode->ToDevice()->DeviceName());
		devRgnNode.parentID = pNode->Parent()->NodeIndex();
		devRgnNode.selfID = pNode->NodeIndex();
		devRgnNode.nodeType = pNode->NodeType();
	}
	else
	{
		assert(FALSE);
	}
	
	pList->push_back(devRgnNode);

	return 0;
}

int CDeviceManager::FindNameCall(CTreeNode *pNode, void *pUser)
{
	FIND_NAME_HELPER *pFinder = (FIND_NAME_HELPER *)pUser; 

	if(TREE_NODE_REGION == pFinder->nodeType)
	{
		if(NULL != pNode->ToRegion())
		{
			if(strcmp(pNode->ToRegion()->RegionName(), pFinder->pName) == 0)
			{
				pFinder->bFound = TRUE;
			}
		}
	}
	else if(TREE_NODE_DEVICE == pFinder->nodeType)
	{
		if(NULL != pNode->ToDevice())
		{
			if(strcmp(pNode->ToDevice()->DeviceName(), pFinder->pName) == 0)
			{
				pFinder->bFound = TRUE;
			}
		}
	}

	return 0;
}

int CDeviceManager::FindNetposCall(CTreeNode *pNode, void *pUser)
{
	FIND_NETPOS_HELPER *pFinder = (FIND_NETPOS_HELPER *)pUser;

	if(NULL != pNode->ToDevice())
	{
		char dvrAddr[64];
		WORD wPort;
		pNode->ToDevice()->GetNetInfo(wPort, dvrAddr);
		if((strcmp(dvrAddr, pFinder->pDVRAddr) == 0) && (wPort == pFinder->wPort))
		{
			pFinder->bFound = TRUE;
		}
	}
	return 0;
}

RESULT WINAPI CDeviceManager::ConnectThread(void *pParam)
{
	CDeviceManager *pThis = reinterpret_cast<CDeviceManager *>(pParam);
	assert(pThis != NULL);

	pThis->ConnectProc();

	return 0;
}

void CDeviceManager::ConnectProc()
{
	int connectTimeout = 100;
	
	BOOL bNeedConnect = FALSE;
	char sDVRIP[64];
	unsigned short nPort;
	char sUsername[64];
	char sPassword[64];
	int channelNum;
	NET_SDK_DEVICEINFO deviceInfo;

	while(m_bConnecting)
	{
		list<DEV_RGN_NODE> listDevRgn;
		m_deviceLock.Lock();
		m_pRgnDevTree->Traverse(GetRgnDevNodeCall, &listDevRgn);
		m_deviceLock.UnLock();

		NET_SDK_SetConnectTime(connectTimeout, 1);
		
		for(list<DEV_RGN_NODE>::iterator it = listDevRgn.begin(); it != listDevRgn.end(); it++)
		{
			if(!m_bConnecting)
			{
				listDevRgn.clear();
				break;
			}

			if((*it).nodeType == TREE_NODE_DEVICE)
			{
				bNeedConnect = FALSE;

				m_deviceLock.Lock();
				CTreeNode *pDevice = m_pRgnDevTree->FindNode((*it).selfID);
				if(NULL == pDevice)
				{
					m_deviceLock.UnLock();
					continue;
				}
				if(NULL == pDevice->GetNodeData())
				{
					pDevice->ToDevice()->GetNetInfo(nPort, sDVRIP);
					pDevice->ToDevice()->GetUserInfo(sUsername, sPassword);
					channelNum = pDevice->ToDevice()->GetChannelNum();
					
					bNeedConnect = TRUE;
				}
				m_deviceLock.UnLock();
				
				if(bNeedConnect)
				{
					LONG lLoginID = NET_SDK_Login(sDVRIP, nPort, sUsername, sPassword, &deviceInfo);
                    printf("[%s] line:%d  lLoginID:%d, deviceID=%d,  deviceName=%s, localVideoInputNum=%d, videoInputNum=%d, videoOuputNum=%d\n", __FILE__, __LINE__, \
						lLoginID, deviceInfo.deviceID, deviceInfo.deviceName, deviceInfo.localVideoInputNum, deviceInfo.videoInputNum, deviceInfo.videoOuputNum);
					if(lLoginID > 0)
					{
						//printf("deviceID = %d,  deviceName = %s, localVideoInputNum = %d, videoInputNum = %d, videoOutputNum = %d", deviceInfo.deviceID, deviceInfo.deviceName, deviceInfo.localVideoInputNum, deviceInfo.videoInputNum, deviceInfo.videoOuputNum);
						m_deviceLock.Lock();
						CTreeNode *pDevice = m_pRgnDevTree->FindNode((*it).selfID);
						if(NULL == pDevice)
						{
							NET_SDK_Logout(lLoginID);
							m_deviceLock.UnLock();
							continue;
						}

						DEV_LINK_INFO *pDevLinkInfo = new DEV_LINK_INFO;
						pDevLinkInfo->lLoginID = lLoginID;
						pDevLinkInfo->dvrDeviceInfo = deviceInfo;
						pDevice->SetNodeData(pDevLinkInfo);
						m_deviceLock.UnLock();
					}
                    else
                    {
                        unsigned int ret = NET_SDK_GetLastError();
                        printf("[%s] line:%d  NET_SDK_GetLastError:%d,sDVRIP = %s\n", __FILE__, __LINE__, ret, sDVRIP);


                    }
				}
			}
		}

		listDevRgn.clear();

		if(connectTimeout < 6400)
		{
			connectTimeout *= 2;
		}

		//分多次Sleep，以便在线程退出时能快速完成
		int sleepedTime = 0;
		while(m_bConnecting)
		{
			PUB_Sleep(100);
			sleepedTime += 100;
			if(sleepedTime >= 2000)
			{
				break;
			}
		}
	}
}



unsigned int CDeviceManager::NetTimeToDVRTime(const DD_TIME &netTime)
{
	tm tmTime;
	memset(&tmTime, 0 ,sizeof(tm));

	tmTime.tm_year = netTime.year;
	tmTime.tm_mon  = netTime.month;
	tmTime.tm_mday = netTime.mday;
	tmTime.tm_hour = netTime.hour;
	tmTime.tm_min  = netTime.minute;
	tmTime.tm_sec  = netTime.second;

	return TmToDVRTime32(tmTime);
}

void CDeviceManager::DVRTimeToNetTime(unsigned int dvrTime, DD_TIME &netTime)
{
	tm tmTime = DVRTime32ToTm(dvrTime);

	netTime.year = tmTime.tm_year;
	netTime.month = tmTime.tm_mon;
	netTime.mday = tmTime.tm_mday;
	netTime.hour = tmTime.tm_hour;
	netTime.minute = tmTime.tm_min;
	netTime.second = tmTime.tm_sec;
}

LONG CDeviceManager::GetLoginID(LONG deviceID)
{
	m_deviceLock.Lock();

	CTreeNode *pDeviceNode = m_pRgnDevTree->FindNode(deviceID);
	if((NULL == pDeviceNode) || (NULL == pDeviceNode->ToDevice()) || (NULL == pDeviceNode->GetNodeData()))
	{
		m_deviceLock.UnLock();
		/*assert(FALSE);*/
		return -1;
	}

	DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pDeviceNode->GetNodeData();
	LONG loginID = pLinkInfo->lLoginID;

	m_deviceLock.UnLock();
	return loginID;
}

int CDeviceManager::GetDeviceID(CTreeNode *pNode, void *pUser)
{
	GET_DEVICEID *pGeter = (GET_DEVICEID *)pUser; 
	if(TREE_NODE_DEVICE == pNode->NodeType())
	{
		DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pNode->GetNodeData();
		if((pLinkInfo != NULL) && (pLinkInfo->lLoginID == pGeter->loginID))
		{
			pGeter->deviceID = pNode->NodeIndex();
		}
	}
	return 0;
}

LONG CDeviceManager::GetDeviceIDByLoginID(LONG loginID)
{

	GET_DEVICEID geter;
	geter.loginID = loginID;
	geter.deviceID = 0;

	m_deviceLock.Lock();
	if (NULL != m_pRgnDevTree)
	{
		m_pRgnDevTree->Traverse(GetDeviceID, &geter);
	}
	m_deviceLock.UnLock();

	return geter.deviceID;
}

BOOL CDeviceManager::GetNetTDvrDevice(LONG deviceID, LPNET_SDK_DEVICEINFO pdeviceInfo)
{
	m_deviceLock.Lock();

	printf("FindNode:\n");
	CTreeNode *pDeviceNode = m_pRgnDevTree->FindNode(deviceID);
	if((NULL == pDeviceNode) || (NULL == pDeviceNode->ToDevice()) || (NULL == pDeviceNode->GetNodeData()))
	{
		m_deviceLock.UnLock();
		assert(FALSE);
		return -1;
	}

	printf("GetNodeData:\n");
	DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pDeviceNode->GetNodeData();
	BOOL ret = NET_SDK_GetDeviceInfo(pLinkInfo->lLoginID, pdeviceInfo);
	if (ret)
	{
		m_deviceLock.UnLock();
		return ret;
	}

	m_deviceLock.UnLock();
	return FALSE;
}

int CDeviceManager::FileDeviceByIpPort(CTreeNode *pNode, void *pUser)
{
	FIND_DEV_BY_IP_PORT *pFinder = (FIND_DEV_BY_IP_PORT *)pUser; 

	if(TREE_NODE_DEVICE == pNode->NodeType())
	{
		char dvrAddr[64];
		WORD wPort;
		pNode->ToDevice()->GetNetInfo(wPort, dvrAddr);
		if((strcmp(dvrAddr, pFinder->deviceIp) == 0) && (wPort == pFinder->port))
		{
			pFinder->deviceID = pNode->NodeIndex();
		}
	}
	return 0;
}

LONG CDeviceManager::GetDeviceIDByIpPort(const char *sDVRIP, WORD wDVRPort)
{

	FIND_DEV_BY_IP_PORT finder;
	finder.deviceID = 0;
	finder.deviceIp = sDVRIP;
	finder.port = wDVRPort;

	m_deviceLock.Lock();
	if (NULL != m_pRgnDevTree)
	{
		m_pRgnDevTree->Traverse(FileDeviceByIpPort, &finder);
	}
	m_deviceLock.UnLock();

	return finder.deviceID;
}

BOOL CDeviceManager::GetDeviceTime(LONG deviceID)
{
	BOOL bRet = FALSE;
	CTreeNode *pNode = NULL;
	DEV_LINK_INFO *pLinkInfo = NULL;

	m_deviceLock.Lock();
	pNode = m_pRgnDevTree->FindNode(deviceID);
	if(NULL == pNode)
	{
		m_deviceLock.UnLock();
		assert(FALSE);
		return -1;
	}
	if(pNode->ToDevice() == NULL)
	{
		m_deviceLock.UnLock();
		assert(FALSE);
		return -1;
	}

	pLinkInfo = (DEV_LINK_INFO *)pNode->GetNodeData();
	if(pLinkInfo != NULL)
	{
		DD_TIME time = {0};
		int nTryTime = 10;
		while(nTryTime > 0)
		{
			if(NET_SDK_GetDeviceTime(pLinkInfo->lLoginID, &time))
			{
				bRet = TRUE;
				time.year += 1900; //年数需要加上1900偏移
				time.month += 1; //月份需要加上偏移
				char szLog[128] = {0};
				sprintf(szLog, "Device Time:%0dd-%02d-%02d %02d:%02d:%02d\n", time.year, time.month, time.mday, time.hour, time.minute, time.second);
#ifdef WIN32
				AfxMessageBox(szLog);
#else
				printf(szLog);
#endif
				break;
			}
			else
			{
				nTryTime--;
				sleep(1);
			}
		}

		m_deviceLock.UnLock();
		return bRet;
	}
	else
	{
		printf("plinkInfo == NULL!\n");
	}

	m_deviceLock.UnLock();

	return bRet;
}

LONG CDeviceManager::StartTalk(LONG deviceID)
{
	m_deviceLock.Lock();

	CTreeNode *pNode = m_pRgnDevTree->FindNode(deviceID);
	if(NULL == pNode)
	{
		m_deviceLock.UnLock();
		assert(FALSE);
		return -1;
	}
	if(pNode->ToDevice() == NULL)
	{
		m_deviceLock.UnLock();
		assert(FALSE);
		return -1;
	}

	DEV_LINK_INFO *pLinkInfo = (DEV_LINK_INFO *)pNode->GetNodeData();
	if(pLinkInfo != NULL)
	{
		int ret = NET_SDK_StartVoiceCom(pLinkInfo->lLoginID, FALSE, NULL, NULL);
		m_deviceLock.UnLock();
		return ret;
	}
	else
	{
		printf("plinkInfo == NULL!\n");
	}

	m_deviceLock.UnLock();
	return -1;

}

void CDeviceManager::StopTalk( LONG talkHandle )
{
	NET_SDK_StopVoiceCom(talkHandle);
}

//BOOL CDeviceManager::IsDevInReg(LONG deviceID, LONG regionID)
//{
//
//	FIND_DEV_IN_REG finder;
//	finder.bFound = FALSE;
//	finder.deviceID = deviceID;
//	finder.regionID = regionID;
//
//	m_deviceLock.Lock();
//	if (NULL != m_pRgnDevTree)
//	{
//		m_pRgnDevTree->Traverse(DeviceIsInRegion, &finder);
//	}
//	m_deviceLock.UnLock();
//
//	return finder.bFound;
//}
