#include "stdafx.h"
#include "Defs.h"
#include "Prefs.h"
#include "Utils.h"
#include "Markup.h"
#include "InputService.h"
#include "Registry.h"


extern BOOL		g_BreakSignal;
extern CTime	g_lastSetupFileTime;
extern BOOL g_isstarted;
extern BOOL g_pollthreadrunning;
extern CTime g_stoptime;
extern BOOL		g_connected;
extern BOOL g_isconfiguring;
extern CUtils g_util;


CPrefs::CPrefs(void)
{
	m_codeword = _T("");
	m_codeword2 = _T("");
	m_title = _T("");

	m_enableinstancecontrol = FALSE;

	m_instancenumber = 0;
	m_transmitretries = 3;
	m_bypasspdfinspection = TRUE;
	m_logPDFinfo = FALSE;
	m_dayofweek = 1;

	m_workfolder = _T("c:\\temp");
	m_workfolder2 = _T("c:\\temp2");
	m_logFileFolder = "c:\\temp\\IputService.log";

	m_bypasspingtest = TRUE;
	m_bypassreconnect = FALSE;
	m_lockcheckmode = LOCKCHECKMODE_READWRITE;


	m_logtodatabase = FALSE;
	m_DBserver = _T("");
	m_Database = _T("ControlCenterPDF") ;
	m_DBuser = _T("sa");
	m_DBpassword = _T("infra");

	m_logToFile = FALSE;


	m_scripttimeout = 60;

	m_updatelog = FALSE;
	m_maxfilesperscan = 10;
	m_maxlogfilesize = 10 * 1024 * 1024;

	
	m_deleteerrorfilesdays = 0;
	m_emailsmtpserver = _T("");
	m_emailfrom = _T("");
	m_emailto = _T("");
	m_emailcc = _T("");
	m_emailsubject = _T("");
	m_emailpreventflooding = TRUE;
	m_emailpreventfloodingdelay = 10;
	m_mailsmtpport = 25;


	m_ftptimeout = 30;
	m_bSortOnCreateTime = TRUE;
	m_minMBfreeCCDATA = 500;
	
	m_defaultpolltime = 1;
	m_bypassdiskfreeteste = FALSE;
	m_bypasscallback = FALSE;


	m_autoresetpollingstateinterval = 10;
	m_deletefileswithextension = _T("");
	m_deletefilessearchmask = _T("");

	m_publicationlock = FALSE;

	m_MayAddPagesInputID = 46;
	m_planlock = TRUE;

	m_testforillegalpdfcolors = FALSE;
	m_alwaysautoassignchannels = TRUE;
	m_reloadpublicationdetails = TRUE;
	m_reloadpageformatdetails = TRUE;
	m_calcCRCaftermove = TRUE;
	m_callcustomSP = TRUE;
	m_updatemiscstring2 = FALSE;
	m_updatemiscstring3 = FALSE;
	m_updatereadytime = TRUE;

	m_generatestatusmessage = FALSE;
	m_messageonerror = FALSE;
	m_messagetemplateerror = _T("");
	m_messagetemplatesuccess = _T("");
	m_messagefilenamemask = _T("%F.jfm");
	m_messagedateformat = _T("DDMMYY");
	m_messagedateformat2 = _T("DDMMYY");
	m_messageusealiases = FALSE;
	m_messageoutputfolder = _T("c:\\temp");
	m_usemessagecommand = FALSE;
	m_messagecommand = _T("");
}


CPrefs::~CPrefs(void)
{
}




void CPrefs::LoadIniFile(CString sIniFile)
{
	TCHAR Tmp[MAX_PATH];
	TCHAR Tmp2[MAX_PATH];

	::GetPrivateProfileString("System", "ScriptTimeOut", "60", Tmp, _MAX_PATH, sIniFile);
	m_scripttimeout = _tstoi(Tmp);

	::GetPrivateProfileString("System", "LookupPublicationWithoutAliasFirst", "1", Tmp, _MAX_PATH, sIniFile);
	m_lookuppublicationwithoutaliasfirst = _tstoi(Tmp);

	GetPrivateProfileString("Setup", "LockCheckMode", "2", Tmp, 255, sIniFile);
	m_lockcheckmode = _tstoi(Tmp);

	GetPrivateProfileString("Setup", "IgnoreLockCheck", "0", Tmp, 255, sIniFile);
	if (_tstoi(Tmp) > 0)
		m_lockcheckmode = 0;

	::GetPrivateProfileString("System", "MinMBfreeCCDATA", "0", Tmp, _MAX_PATH, sIniFile);
	m_minMBfreeCCDATA = _tstoi(Tmp);

	::GetPrivateProfileString("System", "SortFilesOnCreateTime", "1", Tmp, _MAX_PATH, sIniFile);
	m_bSortOnCreateTime = _tstoi(Tmp);

	GetPrivateProfileString("Setup", "SetLocalFileTime", "0", Tmp, 255, sIniFile);
	m_setlocalfiletime = _tstoi(Tmp);

	sprintf(Tmp, "%s\\alivelogs", (LPCSTR)g_util.GetFilePath(m_logFileFolder));
	::GetPrivateProfileString("System", "AliveLogFolder", Tmp, m_alivelogFilePath.GetBuffer(_MAX_PATH), _MAX_PATH, sIniFile);
	m_alivelogFilePath.ReleaseBuffer();
	::CreateDirectory(m_alivelogFilePath, NULL);

	GetPrivateProfileString("Setup", "BypassPingTest", "1", Tmp, 255, sIniFile);
	m_bypasspingtest = _tstoi(Tmp);

	GetPrivateProfileString("Setup", "BypassReconnect", "0", Tmp, 255, sIniFile);
	m_bypassreconnect = _tstoi(Tmp);

	GetPrivateProfileString("Setup", "BypassDiskFreeTest", "0", Tmp, 255, sIniFile);
	m_bypassdiskfreeteste = _tstoi(Tmp);

	GetPrivateProfileString("Setup", "DefaultPolltime", "1", Tmp, 255, sIniFile);
	m_defaultpolltime = _tstoi(Tmp);

	::GetPrivateProfileString("System", "ReloadPublicationDetails", "0", Tmp, 255, sIniFile);
	m_reloadpublicationdetails = atoi(Tmp);

	::GetPrivateProfileString("System", "TestForIllegalPdfColors", "0", Tmp, 255, sIniFile);
	m_testforillegalpdfcolors = atoi(Tmp);

	GetPrivateProfileString("System", "UsePublicationLock", "0", Tmp, 255, sIniFile);
	m_publicationlock = atoi(Tmp);

	::GetPrivateProfileString("System", "BypassPdfInspection", "0", Tmp, 255, sIniFile);
	m_bypasspdfinspection = atoi(Tmp);

	::GetPrivateProfileString("System", "UsePlanLock", "1", Tmp, 255, sIniFile);
	m_planlock = atoi(Tmp);

	::GetPrivateProfileString("System", "UpdateMiscString2", "0", Tmp, 255, sIniFile);
	m_updatemiscstring2 = atoi(Tmp);

	::GetPrivateProfileString("System", "UpdateMiscString3", "0", Tmp, 255, sIniFile);
	m_updatemiscstring3 = atoi(Tmp);

	GetPrivateProfileString("Setup", "DeleteErrorFilesDays", "0", Tmp, 255, sIniFile);
	m_deleteerrorfilesdays = _tstoi(Tmp);

	::GetPrivateProfileString("System", "IgnoreHiddenFiles", "1", Tmp, 255, sIniFile);
	m_ignorehiddenfiles = atoi(Tmp);

	GetPrivateProfileString("Setup", "KeepConnection", "1", Tmp, 255, sIniFile);
	m_persistentconnection = _tstoi(Tmp);

	::GetPrivateProfileString("System", "DayOfWeek", "0", Tmp, 255, sIniFile);
	m_dayofweek = atoi(Tmp);


	GetPrivateProfileString("Setup", "UpdateLog", "0", Tmp, 255, sIniFile);
	m_updatelog = _tstoi(Tmp);

	::GetPrivateProfileString("System", "RecalculateCheckSumAfterMove", "0", Tmp, 255, sIniFile);
	m_calcCRCaftermove = _tstoi(Tmp);

	::GetPrivateProfileString("System", "UpdateReadyTime", "0", Tmp, 255, sIniFile);
	m_updatereadytime = _tstoi(Tmp);

	::GetPrivateProfileString("System", "CcdataFilenameMode", "0", Tmp, 255, sIniFile);
	m_ccdatafilenamemode = _tstoi(Tmp);

	::GetPrivateProfileString("Setup", "FireCommandOnFolderReconnect", "0", Tmp, 255, sIniFile);
	m_firecommandonfolderreconnect = _tstoi(Tmp);

	::GetPrivateProfileString("Setup", "FolderReconnectCommand", "", Tmp, 255, sIniFile);
	m_sFolderReconnectCommand = Tmp;


	::GetPrivateProfileString("Setup", "MaxFilesPerScan", "0", Tmp, 255, sIniFile);
	m_maxfilesperscan = _tstoi(Tmp);

	::GetPrivateProfileString("Setup", "WorkFolder", "c:\\temp", Tmp, _MAX_PATH, sIniFile);
	m_workfolder = Tmp;

	::GetPrivateProfileString("Setup", "WorkFolder2", "c:\\temp2", Tmp, _MAX_PATH, sIniFile);
	m_workfolder2 = Tmp;

	::GetPrivateProfileString("Setup", "MailOnFileError", "0", Tmp, _MAX_PATH, sIniFile);
	m_emailonfileerror = _tstoi(Tmp);

	::GetPrivateProfileString("Setup", "MailOnFolderError", "0", Tmp, _MAX_PATH, sIniFile);
	m_emailonfoldererror = _tstoi(Tmp);

	::GetPrivateProfileString("Setup", "MailSmtpServer", "", Tmp, _MAX_PATH, sIniFile);
	m_emailsmtpserver = Tmp;
	::GetPrivateProfileString("Setup", "MailSmtpPort", "25", Tmp, _MAX_PATH, sIniFile);
	m_mailsmtpport = _tstoi(Tmp);

	::GetPrivateProfileString("Setup", "MailSmtpUsername", "", Tmp, _MAX_PATH, sIniFile);
	m_mailsmtpserverusername = Tmp;

	::GetPrivateProfileString("Setup", "MailSmtpPassword", "", Tmp, _MAX_PATH, sIniFile);
	m_mailsmtpserverpassword = Tmp;

	::GetPrivateProfileString("Setup", "MailUseSSL", "0", Tmp, _MAX_PATH, sIniFile);
	m_mailusessl = _tstoi(Tmp);

	::GetPrivateProfileString("Setup", "MailTimeOut", "60", Tmp, _MAX_PATH, sIniFile);
	m_emailtimeout = _tstoi(Tmp);

	::GetPrivateProfileString("Setup", "MailFrom", "", Tmp, _MAX_PATH, sIniFile);
	m_emailfrom = Tmp;
	::GetPrivateProfileString("Setup", "MailTo", "", Tmp, _MAX_PATH, sIniFile);
	m_emailto = Tmp;
	::GetPrivateProfileString("Setup", "MailCc", "", Tmp, _MAX_PATH, sIniFile);
	m_emailcc = Tmp;
	::GetPrivateProfileString("Setup", "MailSubject", "", Tmp, _MAX_PATH, sIniFile);
	m_emailsubject = Tmp;

	::GetPrivateProfileString("Setup", "MailPreventFlooding", "1", Tmp, _MAX_PATH, sIniFile);
	m_emailpreventflooding = _tstoi(Tmp);

	::GetPrivateProfileString("Setup", "MainPreventFloodingDelay", "10", Tmp, _MAX_PATH, sIniFile);
	m_emailpreventfloodingdelay = _tstoi(Tmp);

	::GetPrivateProfileString("Setup", "MaxLogFileSize", "10485760", Tmp, _MAX_PATH, sIniFile);
	m_maxlogfilesize = _tstoi(Tmp);

	::GetPrivateProfileString("Setup", "FtpTimeout", "30", Tmp, _MAX_PATH, sIniFile);
	m_ftptimeout = _tstoi(Tmp);


	::GetPrivateProfileString("Setup", "DateFormat", "DDMMYYYY", Tmp, _MAX_PATH, sIniFile);
	m_sDateFormat = Tmp;


	::GetPrivateProfileString("System", "DeleteFilesWithExtension", "inf", Tmp, _MAX_PATH, sIniFile);
	m_deletefileswithextension = Tmp;


	::GetPrivateProfileString("System", "IgnoreExtension", "00T", Tmp, 255, sIniFile);
	m_ignoreextension = Tmp;

	::GetPrivateProfileString("System", "WeekDays", "Sunday, Monday, Tuesday, Wednesday, Thursday, Friday, Saturday", Tmp, _MAX_PATH, sIniFile);
	m_WeekDaysList = Tmp;

	::GetPrivateProfileString("System", "CallCustomSP", "1", Tmp, _MAX_PATH, sIniFile);
	m_callcustomSP = _tstoi(Tmp);

	::GetPrivateProfileString("System", "TransmitRetries", "3", Tmp, _MAX_PATH, sIniFile);
	m_transmitretries = _tstoi(Tmp);
	if (m_transmitretries < 1)
		m_transmitretries = 1;

	::GetPrivateProfileString("System", "AutoReloadAliases", "1", Tmp, _MAX_PATH, sIniFile);
	m_autoreloadaliases = _tstoi(Tmp);


	//::GetPrivateProfileString("System", "CallCustomRenamerSP","1", Tmp, 255, sIniFile);
//m_callcustomrenamerSP =  _tstoi(Tmp);


	::GetPrivateProfileString("Message", "GenerateMessage", "0", Tmp, 255, sIniFile);
	m_generatestatusmessage = atoi(Tmp);

	::GetPrivateProfileString("Message", "MessageOnSuccess", "1", Tmp, 255, sIniFile);
	m_messageonsuccess = atoi(Tmp);

	::GetPrivateProfileString("Message", "MessageOnError", "1", Tmp, 255, sIniFile);
	m_messageonerror = atoi(Tmp);

	::GetPrivateProfileString("Message", "UseCommand", "0", Tmp, 255, sIniFile);
	m_usemessagecommand = atoi(Tmp);

	::GetPrivateProfileString("Message", "PrefixPageNumberWithSection", "0", Tmp, 255, sIniFile);
	m_messagepagenumbersprefixsection = atoi(Tmp);

	::GetPrivateProfileString("Message", "UseAbbreviations", "0", Tmp, 255, sIniFile);
	m_messageusealiases = atoi(Tmp);

	::GetPrivateProfileString("Message", "PageNumberingMode", "2", Tmp, 255, sIniFile);
	m_messagepagenumbering = atoi(Tmp);

	::GetPrivateProfileString("Message", "OutputFolder", "c:\\temp", Tmp, 255, sIniFile);
	m_messageoutputfolder = Tmp;

	sprintf(Tmp2, "%s\\messagetemplates\\Success.jfm", (LPCSTR)m_apppath);
	::GetPrivateProfileString("Message", "TemplateOnSuccess", Tmp2, Tmp, 255, sIniFile);
	m_messagetemplatesuccess = Tmp;

	sprintf(Tmp2, "%s\\messagetemplates\\Error.jfm", (LPCSTR)m_apppath);
	::GetPrivateProfileString("Message", "TemplateOnError", Tmp2, Tmp, 255, sIniFile);
	m_messagetemplateerror = Tmp;

	::GetPrivateProfileString("Message", "Command", "", Tmp, 255, sIniFile);
	m_messagecommand = Tmp;
	if (m_messagecommand == "")
		m_usemessagecommand = FALSE;

	::GetPrivateProfileString("Message", "FileNameMask", "%P-%D-%E-%S-%N-%C.jfm", Tmp, 255, sIniFile);
	m_messagefilenamemask = Tmp;

	::GetPrivateProfileString("Message", "DateFormat", "YYYYMMDD", Tmp, 255, sIniFile);
	m_messagedateformat = Tmp;

	::GetPrivateProfileString("Message", "DateFormat2", "YYYY.MM.DD hh:mm:ss", Tmp, 255, sIniFile);
	m_messagedateformat2 = Tmp;

	::GetPrivateProfileString("Message", "PageNumberSeparator", "_", Tmp, 255, sIniFile);
	m_messagepagenumberseparator = Tmp;

	
}

BOOL CPrefs::ConnectToDB(BOOL bLoadDBNames, CString &sErrorMessage)
{

	g_connected = m_DBmaint.InitDB(m_DBserver, m_Database, m_DBuser, m_DBpassword, m_IntegratedSecurity, sErrorMessage);

	if (g_connected) {
		if (m_DBmaint.RegisterService(sErrorMessage) == FALSE) {
			g_util.Logprintf("ERROR: RegisterService() returned - %s", sErrorMessage);
		}
	}
	else
		g_util.Logprintf("ERROR: InitDB() returned - %s", sErrorMessage);

	if (bLoadDBNames) {
		if (m_DBmaint.LoadAllPrefs(sErrorMessage) == FALSE)
			g_util.Logprintf("ERROR: LoadAllPrefs() returned - %s", sErrorMessage);

	}

	return g_connected;
}

int CPrefs::GetPublicationID(CString s)
{
	return GetPublicationID(m_DBmaint, s);
}

int CPrefs::GetPublicationID(CDatabaseManager &DB, CString s)
{
	CString sErrorMessage;

	if (s.Trim() == _T(""))
		return 0;

	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (s.CompareNoCase(m_PublicationList[i].m_name) == 0)
			return m_PublicationList[i].m_ID;

		if (s.CompareNoCase(m_PublicationList[i].m_alias) == 0)
			return m_PublicationList[i].m_ID;
	}

	int nID = DB.LoadNewPublicationID(s, sErrorMessage);
	//if (nID > 0)
	//	DB.LoadSpecificAlias("Publication", s, sErrorMessage);

	return nID;
}

CString CPrefs::GetPublicationName(int nID)
{
	return GetPublicationName(m_DBmaint, nID);
}

CString CPrefs::GetPublicationName(CDatabaseManager &DB, int nID)
{
	CString sErrorMessage;

	if (nID == 0)
		return _T("");

	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (m_PublicationList[i].m_ID == nID)
			return m_PublicationList[i].m_name;
	}

	CString sPubName = DB.LoadNewPublicationName(nID, sErrorMessage);

	if (sPubName != "") 
		DB.LoadSpecificAlias("Publication", sPubName, sErrorMessage);

	return sPubName;
}

CString CPrefs::GetPublicationNameNoReload(int nID)
{
	CString sErrorMessage;

	if (nID == 0)
		return _T("");

	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (m_PublicationList[i].m_ID == nID)
			return m_PublicationList[i].m_name;
	}


	return _T("");
}


void CPrefs::FlushPublicationName(CString sName)
{
	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (sName.CompareNoCase(m_PublicationList[i].m_name) == 0 || sName.CompareNoCase(m_PublicationList[i].m_alias) == 0) {
			m_PublicationList.RemoveAt(i);
			return;
		}
	}
}

PUBLICATIONSTRUCT *CPrefs::GetPublicationStruct(int nID)
{
	return GetPublicationStruct(m_DBmaint, nID);
}

PUBLICATIONSTRUCT *CPrefs::GetPublicationStructNoDbLookup(int nID)
{
	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (m_PublicationList[i].m_ID == nID)
			return &m_PublicationList[i];
	}

	return NULL;
}


PUBLICATIONSTRUCT *CPrefs::GetPublicationStruct(CDatabaseManager &DB, int nID)
{
	if (nID == 0)
		return NULL;

	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (m_PublicationList[i].m_ID == nID)
			return &m_PublicationList[i];
	}

	CString sErrorMessage;

	CString sPubName = DB.LoadNewPublicationName(nID, sErrorMessage);

	if (sPubName != "")
		DB.LoadSpecificAlias("Publication", sPubName, sErrorMessage);

	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (m_PublicationList[i].m_ID == nID)
			return &m_PublicationList[i];
	}

	return NULL;
}

CString CPrefs::GetExtendedAlias(int nPublicationID)
{
	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (m_PublicationList[i].m_ID == nPublicationID)
			return m_PublicationList[i].m_extendedalias;
	}

	return GetPublicationName(nPublicationID);
}



void CPrefs::FlushAlias(CString sType, CString sLongName)
{
	for (int i = 0; i < m_AliasList.GetCount(); i++) {
		if (m_AliasList[i].sType == sType && m_AliasList[i].sLongName.CompareNoCase(sLongName) == 0) {
			m_AliasList.RemoveAt(i);
			return;
		}
	}
}


CString CPrefs::GetEditionName(int nID)
{
	return GetEditionName(m_DBmaint, nID);
}

CString CPrefs::GetEditionName(CDatabaseManager &db, int nID)
{
	CString sErrorMessage;

	if (nID == 0)
		return _T("");

	for (int i = 0; i < m_EditionList.GetCount(); i++) {
		if (m_EditionList[i].m_ID == nID)
			return m_EditionList[i].m_name;
	}

	CString sEdName = db.LoadNewEditionName(nID, sErrorMessage);

	if (sEdName != "")
		db.LoadSpecificAlias("Edition", sEdName, sErrorMessage);

	return sEdName;
}

int CPrefs::GetEditionID(CString s)
{
	return GetEditionID(m_DBmaint, s);
}

int CPrefs::GetEditionID(CDatabaseManager &db, CString s)
{
	CString sErrorMessage;

	if (s.Trim() == "")
		return 0;

	for (int i = 0; i < m_EditionList.GetCount(); i++) {
		if (s.CompareNoCase(m_EditionList[i].m_name) == 0)
			return m_EditionList[i].m_ID;
	}
	int nID = db.LoadNewEditionID(s, sErrorMessage);

	if (nID > 0)
		db.LoadSpecificAlias("Edition", s, sErrorMessage);

	return nID;
}

CString CPrefs::GetSectionName(int nID)
{
	return GetSectionName(m_DBmaint, nID);
}

CString CPrefs::GetSectionName(CDatabaseManager &db, int nID)
{
	CString sErrorMessage;

	if (nID == 0)
		return _T("");

	for (int i = 0; i < m_SectionList.GetCount(); i++) {
		if (m_SectionList[i].m_ID == nID)
			return m_SectionList[i].m_name;
	}

	CString sSecName = db.LoadNewSectionName(nID, sErrorMessage);

	if (sSecName != "")
		db.LoadSpecificAlias("Section", sSecName, sErrorMessage);

	return sSecName;
}


int CPrefs::GetSectionID(CString sName)
{
	return GetSectionID(m_DBmaint, sName);
}

int CPrefs::GetSectionID(CDatabaseManager &db, CString s)
{
	CString sErrorMessage;

	if (s.Trim() == "")
		return 0;

	for (int i = 0; i < m_SectionList.GetCount(); i++) {
		if (s.CompareNoCase(m_SectionList[i].m_name) == 0)
			return m_SectionList[i].m_ID;
	}

	int nID = db.LoadNewSectionID(s, sErrorMessage);

	if (nID > 0)
		db.LoadSpecificAlias("Section", s, sErrorMessage);

	return nID;
}

int CPrefs::GetPageFormatIDFromPublication(int nPublicationID)
{
	return GetPageFormatIDFromPublication(m_DBmaint, nPublicationID, false);
}

int CPrefs::GetPageFormatIDFromPublication(CDatabaseManager &db, int nPublicationID, BOOL bForceReload)
{
	CString sErrorMessage;

	if (nPublicationID == 0)
		return 0;

	if (bForceReload == FALSE) {

		for (int i = 0; i < m_PublicationList.GetCount(); i++) {
			if (nPublicationID == m_PublicationList[i].m_ID) {
				return m_PublicationList[i].m_PageFormatID;
			}
		}
	}

	db.ReloadPublicationDetails(nPublicationID, sErrorMessage);

	for (int i = 0; i < m_PublicationList.GetCount(); i++) {
		if (nPublicationID == m_PublicationList[i].m_ID) {
			return m_PublicationList[i].m_PageFormatID;
		}
	}

	return 0;
}


CString CPrefs::LookupAbbreviationFromName(CString sType, CString sLongName)
{
	if (sType == "Publication") {
		for (int i = 0; i < m_PublicationList.GetCount(); i++) {
			if (sLongName.CompareNoCase(m_PublicationList[i].m_name) == 0)
				return m_PublicationList[i].m_alias;
		}
	}

	for (int i = 0; i < m_AliasList.GetCount(); i++) {
		ALIASSTRUCT a =  m_AliasList[i];
		if (a.sType.CompareNoCase(sType) == 0 && a.sLongName.CompareNoCase(sLongName) == 0)
			return a.sShortName;
	}

	return sLongName;
}

CString CPrefs::LookupNameFromAbbreviation(CString sType, CString sShortName)
{
	
	if (sType == "Publication") {

		for (int i = 0; i < m_PublicationList.GetCount(); i++) {
			if (sShortName.CompareNoCase(m_PublicationList[i].m_alias) == 0)
				return m_PublicationList[i].m_name;
		}

		// reload....
		CString sErrorMessage = _T("");
		if (m_DBmaint.LoadNewPublicationID(sShortName, sErrorMessage)) {
			for (int i = 0; i < m_PublicationList.GetCount(); i++) {
				if (sShortName.CompareNoCase(m_PublicationList[i].m_alias) == 0)
					return m_PublicationList[i].m_name;
			}
		}
	}

	for (int i = 0; i < m_AliasList.GetCount(); i++) {
		ALIASSTRUCT a = m_AliasList[i];
		if (a.sType.CompareNoCase(sType) == 0 && a.sShortName.CompareNoCase(sShortName) == 0)
			return a.sLongName;
	}

	return sShortName;
}



BOOL CPrefs::LoadPreferencesFromRegistry()
{
	CRegistry pReg;

	// Set defaults
	m_logFileFolder= _T("c:\\Temp");

	m_DBserver = _T(".");
	m_Database = _T("PDFHUB");
	m_DBuser = _T("sa");
	m_DBpassword = _T("Infra2Logic");
	m_IntegratedSecurity = FALSE;
	m_logToFile = 1;

	m_databaselogintimeout = 20;
	m_databasequerytimeout = 10;
	m_nQueryRetries = 20;
	m_QueryBackoffTime = 500;
	m_instancenumber = 1;

	if (pReg.OpenKey(CRegistry::localMachine, "Software\\InfraLogic\\InputService\\Parameters")) {
		CString sVal = _T("");
		DWORD nValue;

		if (pReg.GetValue("InstanceNumber", nValue))
			m_instancenumber = nValue;

		if (pReg.GetValue("IntegratedSecurity", nValue))
			m_IntegratedSecurity = nValue;


		if (pReg.GetValue("DBServer", sVal))
			m_DBserver = sVal;

		if (pReg.GetValue("Database", sVal))
			m_Database = sVal;

		if (pReg.GetValue("DBUser", sVal))
			m_DBuser = sVal;

		if (pReg.GetValue("DBpassword", sVal))
			m_DBpassword = sVal;

		if (pReg.GetValue("DBLoginTimeout", nValue))
			m_databaselogintimeout = nValue;

		if (pReg.GetValue("DBQueryTimeout", nValue))
			m_databasequerytimeout = nValue;

		if (pReg.GetValue("DBQueryRetries", nValue))
			m_nQueryRetries = nValue > 0 ? nValue : 5;

		if (pReg.GetValue("DBQueryBackoffTime", nValue))
			m_QueryBackoffTime = nValue >= 500 ? nValue : 500;

		if (pReg.GetValue("Logging", nValue))
			m_logToFile = nValue;
		if (pReg.GetValue("LogFileFolder", sVal))
			m_logFileFolder = sVal;


		pReg.CloseKey();

		return TRUE;
	}

	return FALSE;
}

CString CPrefs::FormCCFilesName(int nPDFtype, int nMasterCopySeparationSet)
{
	CString sPath = _T("");
	if (nPDFtype == POLLINPUTTYPE_LOWRESPDF)
		sPath.Format("%s%d.pdf", m_lowresPDFPath, nMasterCopySeparationSet);
	else if (nPDFtype == POLLINPUTTYPE_HIRESPDF)
		sPath.Format("%s%d.pdf", m_hiresPDFPath, nMasterCopySeparationSet);
	else
		sPath.Format("%s%d.pdf", m_printPDFPath, nMasterCopySeparationSet);

	return sPath;
}


CString CPrefs::FormCcdataFileNameLowres(CJobAttributes *pJob)
{
	CString sDestPath;
	if (m_ccdatafilenamemode && pJob->m_ccdatafilename != _T(""))
		sDestPath.Format("%s\\%s#%d.%s", m_lowresPDFPath, pJob->m_ccdatafilename, pJob->m_mastercopyseparationset, pJob->m_colorname);
	else
		sDestPath.Format("%s\\%d.%s", m_lowresPDFPath, pJob->m_mastercopyseparationset, pJob->m_colorname);

	return sDestPath;
}

CString CPrefs::FormCcdataFileNameHires(CJobAttributes *pJob)
{
	CString sDestPath;
	if (m_ccdatafilenamemode && pJob->m_ccdatafilename != _T(""))
		sDestPath.Format("%s\\%s#%d.%s", m_hiresPDFPath, pJob->m_ccdatafilename, pJob->m_mastercopyseparationset, pJob->m_colorname);
	else
		sDestPath.Format("%s\\%d.%s", m_hiresPDFPath, pJob->m_mastercopyseparationset, pJob->m_colorname);

	return sDestPath;
}

CString CPrefs::FormCcdataFileNamePrint(CJobAttributes *pJob)
{
	CString sDestPath;
	if (m_ccdatafilenamemode && pJob->m_ccdatafilename != _T(""))
		sDestPath.Format("%s\\%s#%d.%s", m_printPDFPath, pJob->m_ccdatafilename, pJob->m_mastercopyseparationset, pJob->m_colorname);
	else
		sDestPath.Format("%s\\%d.%s", m_printPDFPath, pJob->m_mastercopyseparationset, pJob->m_colorname);

	return sDestPath;
}