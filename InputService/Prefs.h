#pragma once
#include "Defs.h"
#include "DatabaseManager.h"

class CPrefs
{
public:
	CPrefs(void);
	~CPrefs(void);

	void	LoadIniFile(CString sIniFile);
	BOOL	ConnectToDB(BOOL bLoadDBNames, CString &sErrorMessage);

	BOOL LoadPreferencesFromRegistry();

	int GetPublicationID(CDatabaseManager &DB, CString s);
	int GetPublicationID(CString s);
	CString GetPublicationName(CDatabaseManager &DB, int nID);
	CString GetPublicationName(int nID);
	CString GetPublicationNameNoReload(int nID);
	PUBLICATIONSTRUCT *GetPublicationStruct(int nID);
	PUBLICATIONSTRUCT *GetPublicationStructNoDbLookup(int nID);
	PUBLICATIONSTRUCT *GetPublicationStruct(CDatabaseManager &DB, int nID);
	CString  GetExtendedAlias(int nPublicationID);
	void FlushPublicationName(CString sName);

	void FlushAlias(CString sType, CString sLongName);

	CString GetEditionName(int nID);
	CString GetEditionName(CDatabaseManager &DB, int nID);
	int		GetEditionID(CString s);
	int		GetEditionID(CDatabaseManager &DB, CString s);
	CString GetSectionName(int nID);
	CString GetSectionName(CDatabaseManager &DB, int nID);
	int		GetSectionID(CString s);
	int		GetSectionID(CDatabaseManager &DB, CString s);
	
	CString GetPublisherName(int nID);
	CString GetPublisherName(CDatabaseManager &DB, int nID);

	CString  LookupAbbreviationFromName(CString sType, CString sLongName);
	CString  LookupNameFromAbbreviation(CString sType, CString sShortName);
	CString		GetChannelName(int nID);
	CHANNELSTRUCT	*GetChannelStruct(int nID);
	CHANNELSTRUCT	*GetChannelStruct(CDatabaseManager &DB, int nID);
	CHANNELGROUPSTRUCT	*GetChannelGroupStruct(int nID);
	CHANNELGROUPSTRUCT	*GetChannelGroupStruct(CDatabaseManager &DB, int nID);

	int		GetPageFormatIDFromPublication(int nPublicationID);
	int		GetPageFormatIDFromPublication(CDatabaseManager &DB, int nPublicationID, BOOL bForceReload);

	CString FormCCFilesName(int nPDFtype, int nMasterCopySeparationSet);
	CString FormCcdataFileNameLowres(CJobAttributes *pJob);
	CString FormCcdataFileNamePrint(CJobAttributes *pJob);
	CString FormCcdataFileNameHires(CJobAttributes *pJob);


	INPUTPROCESSLIST m_InputProcessList;
	
	CString m_setupXMLpath;
	CString m_setupEnableXMLpath;

	BOOL m_FieldExists_Log_FileName;
	BOOL m_FieldExists_Log_MiscString;

	CString	m_apppath;
	CString m_title;
	CString m_codeword;
	CString m_codeword2;

	CString     m_WeekDaysList;


	CString m_workfolder;
	CString m_workfolder2;
	BOOL m_bypasspingtest;
	BOOL m_bypassreconnect;
	int m_lockcheckmode;
	DWORD m_maxlogfilesize;


	BOOL		m_logtodatabase;
	CString		m_DBserver;
	CString		m_Database;
	CString		m_DBuser;
	CString		m_DBpassword;
	BOOL		m_IntegratedSecurity;
	int			m_databaselogintimeout;
	int			m_databasequerytimeout;
	int			m_nQueryRetries;
	int			m_QueryBackoffTime;


	int m_autoresetpollingstateinterval;

	BOOL		m_persistentconnection;
	int			m_logToFile;
	CString		m_logFileFolder;
	int			m_scripttimeout;
	BOOL		m_updatelog;
	int			m_maxfilesperscan;
	BOOL		m_setlocalfiletime;


	BOOL m_enableinstancecontrol;
	int	m_instancenumber;

	

	int			m_deleteerrorfilesdays;


	BOOL m_emailonfoldererror;
	BOOL m_emailonfileerror;


	CString m_emailsmtpserver;
	int m_mailsmtpport;
	CString m_mailsmtpserverusername;
	CString m_mailsmtpserverpassword;
	BOOL m_mailusessl;
	CString m_emailfrom;
	CString m_emailto;
	CString m_emailcc;
	CString m_emailsubject;
	int		m_emailtimeout;
	BOOL	m_emailpreventflooding;
	int		m_emailpreventfloodingdelay;

	
	int m_ftptimeout;
	BOOL m_bSortOnCreateTime;
	CString m_alivelogFilePath;
	int m_defaultpolltime;
	BOOL m_bypassdiskfreeteste;
	BOOL m_bypasscallback;


	
	

	BOOL m_firecommandonfolderreconnect;
	CString m_sFolderReconnectCommand;
	CString m_sDateFormat;
	

	int m_heartbeatMS;


	// Transmitter properties..
	CString m_lowresPDFPath;
	CString m_hiresPDFPath;		// == CCFiles ServerShare
	CString m_printPDFPath;

	CString m_previewPath;
	CString m_thumbnailPath;
	CString m_serverName;
	CString m_serverShare;
	CString m_readviewpreviewPath;
	CString m_configPath;
	CString m_errorPath;
	CString m_webPath;
	BOOL   m_copyToWeb;
	int		m_mainserverusecussrentuser;
	CString m_mainserverusername;
	CString m_mainserverpassword;

	PUBLICATIONLIST		m_PublicationList;
	ITEMLIST			m_EditionList;
	ITEMLIST			m_SectionList;
	FILESERVERLIST		m_FileServerList;
	ALIASLIST			m_AliasList;
	STATUSLIST			m_StatusList;
	STATUSLIST			m_ExternalStatusList;

//	CHANNELLIST			m_ChannelList;
//	CHANNELGROUPLIST	m_ChannelGroupList;
//	ITEMLIST			m_PublisherList;


	BOOL m_ccdatafilenamemode;

	CDatabaseManager m_DBmaint;

	// INput specific




	CString m_deletefileswithextension;
	CString m_deletefilessearchmask;
	int m_minMBfreeCCDATA;
	CString m_ignoreextension;
	BOOL m_useerrorloginformationfile;
	CString m_deleteextension;
	int   m_transmitretries;
	BOOL m_bypasspdfinspection;
	BOOL m_logPDFinfo;
	int   m_dayofweek;
	BOOL m_publicationlock;
	BOOL m_planlock;
//	BOOL m_respectallowunplannedforpublication;
	int m_MayAddPagesInputID;
	BOOL m_alwaysautoassignchannels;
	BOOL m_testforillegalpdfcolors;
	BOOL m_reloadpublicationdetails;

	BOOL m_reloadpageformatdetails;



	BOOL m_calcCRCaftermove;

	BOOL m_callcustomSP;
	BOOL m_updatemiscstring2;
	BOOL m_updatemiscstring3;
	BOOL m_updatereadytime;

	BOOL m_lookuppublicationwithoutaliasfirst;

	BOOL m_generatestatusmessage;
	BOOL m_messageonsuccess;
	BOOL m_messageonerror;
	CString m_messagetemplatesuccess;
	CString m_messagetemplateerror;
	CString m_messagefilenamemask;
	CString m_messagedateformat;
	CString m_messagedateformat2;
	BOOL m_messageusealiases;
	CString m_messageoutputfolder;
	BOOL m_usemessagecommand;
	CString m_messagecommand;
	CString m_messagepagenumberseparator;
	int m_messagepagenumbering;
	BOOL m_messagepagenumbersprefixsection;

	BOOL m_transmitafterproof;
	BOOL m_autoreloadaliases;

	BOOL m_ignorehiddenfiles;
};

