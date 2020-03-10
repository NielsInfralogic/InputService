#pragma once
#include <afxdb.h>

class CDatabaseManager
{
public:
	CDatabaseManager(void);
	virtual ~CDatabaseManager(void);

	BOOL	InitDB(CString sDBserver, CString sDatabase, CString sDBuser, CString sDBpassword, BOOL bIntegratedSecurity, CString &sErrorMessage);
	BOOL	InitDB(CString &sErrorMessage);
	void	ExitDB();

	BOOL	IsOpen();
	BOOL	LoadConfigIniFile(int nInstanceNumber, CString &sFileName, CString &sFileName2, CString &sFileName3, CString &sErrorMessage);
	BOOL	RegisterService(CString &sErrorMessage);
	BOOL	UpdateService(int nCurrentState, CString sCurrentJob, CString sLastError, CString &sErrorMessage);
	BOOL	UpdateService(int nCurrentState, CString sCurrentJob, CString sLastError, CString sAddedLogData, CString &sErrorMessage);
	BOOL	InsertLogEntry(int nEvent, CString sSource, CString sFileName, CString sMessage, int nMasterCopySeparationSet, int nVersion, int nMiscInt, CString sMiscString, CString &sErrorMessage);
	BOOL	UpdatePrepollStatus(int nMasterCopySeparationSet, int serviceType, int nEventCode, CString sMessage, CString &sErrorMessage);
	BOOL	LoadSTMPSetup(CString &sErrorMessage);

	BOOL	CallMapNamesCustomSP(int nInputID, CJobAttributes *pJob, CString &sErrorMessage);
	BOOL    LoadPublicationList(CString &sErrorMessage);
	BOOL	LoadEditionNameList(CString &sErrorMessage);
	BOOL	LoadSectionNameList(CString &sErrorMessage);
	BOOL	LoadAliasList(CString &sErrorMessage);
	BOOL    LoadStatusList(CString sIDtable, STATUSLIST &v, CString &sErrorMessage);
	BOOL	LoadPublicationNameExtension(int nID, CString &sErrorMessage);
	BOOL	LoadPublicationChannels(int nID, CString &sErrorMessage);
	BOOL	LoadFileServerList(CString &sErrorMessage);
//	int     LoadPublisherList(CString &sErrorMessage);
//	int		LoadChannelGroupList(CString &sErrorMessage);
//	int		LoadChannelList(CString &sErrorMessage);

	BOOL	LoadAllPrefs(CString &sErrorMessage);
	int		LoadGeneralPreferences(CString &sErrorMessage);

	int		LoadNewPublicationID(CString sName, CString &sErrorMessage);
	CString LoadNewPublicationName(int nID, CString &sErrorMessage);
	CString LoadNewEditionName(int nID, CString &sErrorMessage);
	int		LoadNewEditionID(CString sName, CString &sErrorMessage);
	CString LoadNewSectionName(int nID, CString &sErrorMessage);
	int		LoadNewSectionID(CString sName, CString &sErrorMessage);
//	BOOL    LoadNewChannel(int nChannelID, CString &sErrorMessage);
//	BOOL    LoadNewChannelGroup(int nID, CString &sErrorMessage);
//	CString LoadNewPublisherName(int nID, CString &sErrorMessage);

	BOOL	ReloadPublicationDetails(int nID, CString &sErrorMessage);

	BOOL	DecreaseVersion(int nMasterCopySeparationSet, CString &sErrorMessage);

	BOOL	CallCustomSP(int nMasterCopySeparationSet, int nCopySeparationSet, int nCustomFlag, CJobAttributes *pJob, CString &sErrorMessage);

	BOOL	UpdateVersion(int nMasterCopySeparationSet, int nVersion, CString &sErrorMessage);
	BOOL	UpdateInputTime(int nMasterCopySeparationSet,  CTime tInputTime, CString &sErrorMessage);
	BOOL	UpdateMiscString2(int nMasterCopySeparationSet, CString sMiscString2, CString &sErrorMessage);
	BOOL	UpdateMiscString3(int nMasterCopySeparationSet, CString sMiscString3, CString &sErrorMessage);
	BOOL	UpdateCRC(int nMasterCopySeparationSet, int nPDFtype, int nCRC, CString &sErrorMessage);
	BOOL	UpdateReadyTime(int nMasterCopySeparationSet, CString &sErrorMessage);
	BOOL	UpdateApproval(int nMasterCopySeparationSet, int nApproval, CString sUserName, CString &sErrorMessage);
	BOOL	UpdateRelease(int nMasterCopySeparationSet, BOOL bHold, CString &sErrorMessage);


	BOOL	LoadInputProcessList(CString &sErrorMessage);
	BOOL	LoadExpressionList(INPUTPROCESSSTRUCT *pInput, CString  &sErrorMessage);
	BOOL	ResetPollStatus(CString &sErrorMessage);

	CTime	GetDatabaseTime(CString &sErrorMessage);

	CString  LoadSpecificAlias(CString sType, CString sLongName, CString &sErrorMessage);

	BOOL	GetInputEnabledUpdate(INPUTPROCESSSTRUCT *pInput, CString &sErrorMessage);

	int		LookupInputJob(CJobAttributes *pJob, INPUTPROCESSSTRUCT *ip, CString sFileName, BOOL bAllowUnplannedSubEdition, CString &sErrorMessage);
	BOOL	GetJobDetails(CJobAttributes *pJob, int nCopySeparationSet, CString &sErrorMessage);
	
	int     GetChannelCRC(int nMasterCopySeparationSet, int nInputMode, CString &sErrorMessage);
	int		GetChannelStatus(int nMasterCopySeparationSet, int nChannelID, CString &sErrorMessage);
	BOOL	UpdateChannelStatus(int nMasterCopySeparationSet, int nProductionID, int nChannelID, int nStatus, CString sMessage, CString sFileName, CString &sErrorMessage);
	BOOL	UpdateChannelStatus2(int nMasterCopySeparationSet, int nProductionID, int nChannelID, int nStatus, CString sMessage, CString sFileName, CString &sErrorMessage);

	BOOL	PlanLock(int bRequestedPlanLock, int &bCurrentPlanLock, CString &sClientName, CString &sClientTime, CString &sErrorMessage);

	BOOL	PublicationPlanLock(int nPublicationID, CTime tPubDate, CTime  tPubDate2, int bRequestedPlanLock, 
												int &bCurrentPlanLock, CString &sClientName, CString &sClientTime, CString &sErrorMessage);

	int		CreateSubEditionPlate(CJobAttributes *pJob, BOOL bForceHold, BOOL bUseDefaultApproval, BOOL bFreeDevice, CString &sErrorMessage);
	int		InsertUnplannedPage(CJobAttributes *pJob, BOOL bMustApplyProduction, BOOL bStatusMissing, BOOL bForceVersion0, CString &sErrorMessage);
	int		UpdateStatusInput(int nInputID, int nMasterCopySeparationSet, int nPDFtype, int nStatus, CString sErr,
							CString sFileName, CString sComment, BOOL bReapproveUpdates, CString &sErrorMessage);

	int		StoredProcParameterExists(CString sSPname, CString sParameterName, CString &sErrorMessage);
	int		TableExists(CString sTableName, CString &sErrorMessage);
	int		FieldExists(CString sTableName, CString sFieldName , CString &sErrorMessage);


	CString TranslateDate(CTime tDate);
	CString TranslateTime(CTime tDate);
	void	LogprintfDB(const TCHAR *msg, ...);


private:
	BOOL	m_DBopen;
	CDatabase *m_pDB;

	BOOL	m_IntegratedSecurity;
	CString m_DBserver;
	CString m_Database;
	CString m_DBuser;
	CString m_DBpassword;
	BOOL	m_PersistentConnection;
};
