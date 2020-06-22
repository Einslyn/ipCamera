
#include <assert.h>
#include "RegionDeviceLog.h"

CRegionDeviceLog::CRegionDeviceLog(void)
{
	m_pFile = NULL;
}

CRegionDeviceLog::~CRegionDeviceLog(void)
{
}

BOOL CRegionDeviceLog::GetLogInfo(CTreeNode *pRgnDevTree, vector<GROUP_INFO> &groupArray, CHANNEL_INFO *pChannelInfo, int channelNum, unsigned int &maxID)
{
	assert(pRgnDevTree != NULL);
	assert(pRgnDevTree->FirstChild() == NULL);
	assert(pChannelInfo != NULL);
	if((NULL == pRgnDevTree) || (NULL == pChannelInfo))
	{
		return FALSE;
	}
	if(pRgnDevTree->FirstChild() != NULL)
	{
		pRgnDevTree->RemoveAllChild();
	}
	groupArray.clear();
	maxID = 0;

	m_pFile = fopen(RGN_DEV_FILE_NAME, "rb");
	if(NULL == m_pFile)
	{
		//还没有创建文件
		if(!CreateLogFile())
		{
			return FALSE;
		}
		
		for(unsigned char channel = 0; channel < channelNum; channel++)
		{
			pChannelInfo[channel].deviceID = RGN_DEV_INVALID_ID;
		}

		maxID = 0;
	}
	else
	{
		if(1 != fread(&m_fileHeader, sizeof(m_fileHeader), 1, m_pFile))
		{
			fclose(m_pFile);
			m_pFile = NULL;
			return FALSE;
		}

		if(m_fileHeader.fileFlag != RGN_DEV_FILE_FLAG)
		{
			fclose(m_pFile);
			m_pFile = NULL;
			return FALSE;
		}

		channelNum = (m_fileHeader.channelNum < channelNum) ? m_fileHeader.channelNum : channelNum;
		if(1 != fread(pChannelInfo, channelNum*sizeof(CHANNEL_INFO), 1, m_pFile))
		{
			fclose(m_pFile);
			m_pFile = NULL;
			return FALSE;
		}

		int index = 0;
		//把所有区域节点添加到设备树中
		REGION_INFO regionInfo;
		for(index = 0; index < m_fileHeader.regionNum; index++)
		{
			if(1 != fread(&regionInfo, sizeof(REGION_INFO), 1, m_pFile))
			{
				pRgnDevTree->RemoveAllChild();
				fclose(m_pFile);
				m_pFile = NULL;
				return FALSE;
			}

			maxID = (regionInfo.regionID > maxID) ? regionInfo.regionID : maxID;

			CRegionNode regionNode(regionInfo.regionName, regionInfo.regionID);
			CTreeNode *pParent = pRgnDevTree->FindNode(regionInfo.parentID);
			pParent->InsertEndChild(regionNode);
		}

		//把所有设备节点添加到设备树中
		DEVICE_INFO deviceInfo;
		for(index = 0; index < m_fileHeader.deviceNum; index++)
		{
			if(1 != fread(&deviceInfo, sizeof(DEVICE_INFO), 1, m_pFile))
			{
				pRgnDevTree->RemoveAllChild();
				fclose(m_pFile);
				m_pFile = NULL;
				return FALSE;
			}

			maxID = (deviceInfo.deviceID > maxID) ? deviceInfo.deviceID : maxID;

			CDeviceNode deviceNode(deviceInfo.deviceName, deviceInfo.deviceID);
			deviceNode.SetNetInfo(deviceInfo.serverPort, deviceInfo.serverAddr);
			deviceNode.SetChannelNum(deviceInfo.channelNum);
			deviceNode.SetUserInfo(deviceInfo.userName, deviceInfo.password);
			CTreeNode *pParent = pRgnDevTree->FindNode(deviceInfo.parentID);
			pParent->InsertEndChild(deviceNode);
		}

		GROUP_INFO groupInfo;
		//读取出通道分组信息
		for(index = 0; index < m_fileHeader.groupNum; index++)
		{
			if(1 != fread(&groupInfo, sizeof(GROUP_INFO), 1, m_pFile))
			{
				pRgnDevTree->RemoveAllChild();
				groupArray.clear();
				fclose(m_pFile);
				m_pFile = NULL;
				return FALSE;
			}

			maxID = (groupInfo.groupID > maxID) ? groupInfo.groupID : maxID;

			groupArray.push_back(groupInfo);
		}

		fclose(m_pFile);
		m_pFile = NULL;
	}

	return TRUE;
}

BOOL CRegionDeviceLog::SetLogInfo(CTreeNode *pRgnDevTree, vector<GROUP_INFO> &groupArray, CHANNEL_INFO *pChannelInfo, int channelNum)
{
	assert(pRgnDevTree != NULL);
	assert(pChannelInfo != NULL);
	if((NULL == pRgnDevTree) || (NULL == pChannelInfo))
	{
		return FALSE;
	}

	memset(&m_fileHeader, 0, sizeof(m_fileHeader));
	m_fileHeader.fileFlag = RGN_DEV_FILE_FLAG;
	m_fileHeader.fileVer = RGN_DEV_LOG_VER;
	m_fileHeader.nameLen = RGN_DEV_NAME_LEN;
	m_fileHeader.channelNum = MAX_CHANNEL_NUM;

	m_pFile = fopen(RGN_DEV_FILE_NAME, "wb");
	if(NULL == m_pFile)
	{
		return FALSE;
	}

	//先分析设备树中有多少个区域节点，多少个设备节点
	pRgnDevTree->Traverse(CaculateNodeNum, this);
	m_fileHeader.channelNum = channelNum;
	m_fileHeader.groupNum = groupArray.size();

	if(1 != fwrite(&m_fileHeader, sizeof(m_fileHeader), 1, m_pFile))
	{
		fclose(m_pFile);
		m_pFile = NULL;
		return FALSE;
	}

	if(1 != fwrite(pChannelInfo, sizeof(CHANNEL_INFO)*channelNum, 1, m_pFile))
	{
		fclose(m_pFile);
		m_pFile = NULL;
		return FALSE;
	}

	//遍历设备树写区域信息
	pRgnDevTree->Traverse(WriteRegion, this);

	if(NULL == m_pFile)
	{
		return FALSE;
	}

	//遍历设备树写设备信息
	pRgnDevTree->Traverse(WriteDevice, this);

	if(NULL == m_pFile)
	{
		return FALSE;
	}

	//写分组信息
	GROUP_INFO groupInfo;
	//读取出通道分组信息
	for(int index = 0; index < m_fileHeader.groupNum; index++)
	{
		groupInfo = groupArray[index];
		if(1 != fwrite(&groupInfo, sizeof(GROUP_INFO), 1, m_pFile))
		{
			fclose(m_pFile);
			m_pFile = NULL;
			return FALSE;
		}
	}

	fclose(m_pFile);
	m_pFile = NULL;

	return TRUE;
}

BOOL CRegionDeviceLog::CreateLogFile()
{
	RGN_DEV_FILE_HEADER fileHeader;
	m_pFile = fopen(RGN_DEV_FILE_NAME, "wb");
	if(NULL == m_pFile)
	{
		return FALSE;
	}

	fileHeader.fileFlag = RGN_DEV_FILE_FLAG;
	fileHeader.fileVer = RGN_DEV_LOG_VER;
	fileHeader.nameLen = RGN_DEV_NAME_LEN;
	fileHeader.channelNum = MAX_CHANNEL_NUM;
	fileHeader.regionNum = 0;
	fileHeader.groupNum = 0;
	fileHeader.deviceNum = 0;

	if(1 != fwrite(&fileHeader, sizeof(fileHeader), 1, m_pFile))
	{
		fclose(m_pFile);
		m_pFile = NULL;
		return FALSE;
	}

	char *pTemp = new char[sizeof(CHANNEL_INFO)*MAX_CHANNEL_NUM];
	memset(pTemp, 0, sizeof(CHANNEL_INFO)*MAX_CHANNEL_NUM);

	if(1 != fwrite(pTemp, sizeof(CHANNEL_INFO)*MAX_CHANNEL_NUM, 1, m_pFile))
	{
		delete [] pTemp;
		fclose(m_pFile);
		m_pFile = NULL;
		return FALSE;
	}

	delete [] pTemp;
	fclose(m_pFile);
	m_pFile = NULL;
	return TRUE;
}

int CRegionDeviceLog::CaculateNodeNum(CTreeNode *pNode, void *pUser)
{
	CRegionDeviceLog *pThis = reinterpret_cast<CRegionDeviceLog *>(pUser);

	if(NULL == pThis)
	{
		assert(FALSE);
		return 0;
	}
	
	pThis->CaculateNodeNum(pNode);
	return 0;
}

void CRegionDeviceLog::CaculateNodeNum(CTreeNode *pNode)
{
	if(pNode->NodeType() == TREE_NODE_REGION)
	{
		m_fileHeader.regionNum += 1;
	}
	else if(pNode->NodeType() == TREE_NODE_DEVICE)
	{
		m_fileHeader.deviceNum += 1;
	}
	else
	{
		assert(FALSE);
	}
}

int CRegionDeviceLog::WriteRegion(CTreeNode *pNode, void *pUser)
{
	CRegionDeviceLog *pThis = (CRegionDeviceLog *)pUser;

	if(NULL == pThis->m_pFile)
	{
		return 0;
	}

	if(pNode->NodeType() == TREE_NODE_REGION)
	{
		REGION_INFO regionInfo;
		memset(&regionInfo, 0, sizeof(regionInfo));
		regionInfo.parentID = pNode->Parent()->NodeIndex();
		regionInfo.regionID = pNode->NodeIndex();
		strcpy(regionInfo.regionName, pNode->ToRegion()->RegionName());
		if(1 != fwrite(&regionInfo, sizeof(REGION_INFO), 1, pThis->m_pFile))
		{
			fclose(pThis->m_pFile);
			pThis->m_pFile = NULL;
		}
	}

	return 0;
}

int CRegionDeviceLog::WriteDevice(CTreeNode *pNode, void *pUser)
{
	CRegionDeviceLog *pThis = (CRegionDeviceLog *)pUser;

	if(NULL == pThis->m_pFile)
	{
		return 0;
	}

	if(pNode->NodeType() == TREE_NODE_DEVICE)
	{
		DEVICE_INFO deviceInfo;
		memset(&deviceInfo, 0, sizeof(deviceInfo));

		//从设备树中得到该设备的所有相关信息
		deviceInfo.parentID = pNode->Parent()->NodeIndex();
		deviceInfo.deviceID = pNode->NodeIndex();
		strcpy(deviceInfo.deviceName, pNode->ToDevice()->DeviceName());
		pNode->ToDevice()->GetNetInfo(deviceInfo.serverPort, deviceInfo.serverAddr);
		pNode->ToDevice()->GetUserInfo(deviceInfo.userName, deviceInfo.password);
		deviceInfo.channelNum = pNode->ToDevice()->GetChannelNum();

		//把设备信息写入文件 
		if(1 != fwrite(&deviceInfo, sizeof(DEVICE_INFO), 1, pThis->m_pFile))
		{
			fclose(pThis->m_pFile);
			pThis->m_pFile = NULL;
		}
	}

	return 0;
}


