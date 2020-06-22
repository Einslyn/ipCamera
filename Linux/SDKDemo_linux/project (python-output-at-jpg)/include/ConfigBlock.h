/***********************************************************************
** Copyright (C) Tongwei Video Technology Co.,Ltd. All rights reserved.
** Author       : Ԭʯά
** Date         : 2009-06-02
** Name         : ConfigBlock.h
** Version      : 1.0
** Description  :
** Modify Record:
1:
***********************************************************************/
#ifndef _CONFIG_BLOCK_H_
#define _CONFIG_BLOCK_H_

#include "dvrdvsconfig.h"
#include "DDPublic.h"
#include <list>

class CConfigBlock
{
public:
	CConfigBlock();
	~CConfigBlock();
	
	//////////////////////////////////////////////////////////////////////////
	void CreateBlock(unsigned char *pData, unsigned int dataLen);
	void DestroyBlock();

	inline const unsigned char *Data() const{return m_pData;};
	inline unsigned char *Data(){return m_pData;};
	inline unsigned int	DataLen(){return m_dataLen;};
	//////////////////////////////////////////////////////////////////////////
	bool GetConfig(DD_CONFIG_ITEM_ID itemID, unsigned char **ppData, unsigned int &dataLen) const;
	void SetConfig(DD_CONFIG_ITEM_ID itemID, const unsigned char *pData, unsigned int dataLen);

	/////////////////////////////////////////////////////////////////////////
	bool GetSubItemCfg(DD_CONFIG_ITEM_ID itemID, unsigned char **ppData, unsigned int dataPos, unsigned int &dataLen);
	void SetSubItemCfg(DD_CONFIG_ITEM_ID itemID, const unsigned char *pData, unsigned int dataPos, unsigned int dataLen);

	void SetConfigEx(DD_CONFIG_ITEM_ID itemID, const unsigned char *pData, unsigned int dataLen, unsigned int oldLen);
	std::list<DD_CONFIG_ITEM_HEAD_EX> &GetConfigItemList();
	const std::list<DD_CONFIG_ITEM_HEAD_EX> &GetConfigItemList() const;
	int  GetItemCount() const;

protected:
	
private:
	unsigned char	*m_pData;
	unsigned int	m_dataLen;
	std::list<DD_CONFIG_ITEM_HEAD_EX> m_itemList;
};
#endif //_CONFIG_BLOCK_H_
