#include "StdAfx.h"
#include "Defs.h"
#include "DatabaseManager.h"
#include "Utils.h"
#include "Prefs.h"

extern CPrefs g_prefs;
extern CUtils g_util;
extern bool g_pollthreadrunning;

CDatabaseManager::CDatabaseManager(void)
{
	m_DBopen = FALSE;
	m_pDB = NULL;

	m_DBserver = _T(".");
	m_Database = _T("ControlCenter");
	m_DBuser = "saxxxxxxxxx";
	m_DBpassword = "xxxxxx";
	m_IntegratedSecurity = FALSE;
	m_PersistentConnection = FALSE;

}

CDatabaseManager::~CDatabaseManager(void)
{
	ExitDB();
	if (m_pDB != NULL)
		delete m_pDB;
}

BOOL CDatabaseManager::InitDB(CString sDBserver, CString sDatabase, CString sDBuser, CString sDBpassword, BOOL bIntegratedSecurity, CString &sErrorMessage)
{
	m_DBserver = sDBserver;
	m_Database = sDatabase;
	m_DBuser = sDBuser;
	m_DBpassword = sDBpassword;
	m_IntegratedSecurity = bIntegratedSecurity;

	return InitDB(sErrorMessage);
}


int CDatabaseManager::InitDB(CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (m_pDB) {
		if (m_pDB->IsOpen() == FALSE) {
			try {
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
		}
	}

	if (!m_PersistentConnection)
		ExitDB();

	if (m_DBopen)
		return TRUE;

	if (m_DBserver == _T("") || m_Database == _T("") || m_DBuser == _T("")) {
		sErrorMessage = _T("Empty server, database or username not allowed");
		return FALSE;
	}

	if (m_pDB == NULL)
		m_pDB = new CDatabase;

	m_pDB->SetLoginTimeout(g_prefs.m_databaselogintimeout);
	m_pDB->SetQueryTimeout(g_prefs.m_databasequerytimeout);

	CString sConnectStr = _T("Driver={SQL Server}; Server=") + m_DBserver + _T("; ") +
		_T("Database=") + m_Database + _T("; ");

	if (m_IntegratedSecurity)
		sConnectStr += _T(" Integrated Security=True;");
	else
		sConnectStr += _T("USER=") + m_DBuser + _T("; PASSWORD=") + m_DBpassword + _T(";");

	try {
		if (!m_pDB->OpenEx((LPCTSTR)sConnectStr, CDatabase::noOdbcDialog)) {
			sErrorMessage.Format(_T("Error connecting to database with connection string '%s'"), (LPCSTR)sConnectStr);
			return FALSE;
		}
	}
	catch (CDBException* e) {
		sErrorMessage.Format(_T("Error connecting to database - %s (%s)"), (LPCSTR)e->m_strError, (LPCSTR)sConnectStr);
		e->Delete();
		return FALSE;
	}

	m_DBopen = TRUE;
	return TRUE;
}

void CDatabaseManager::ExitDB()
{
	if (!m_DBopen)
		return;

	if (m_pDB)
		m_pDB->Close();

	m_DBopen = FALSE;

	return;
}

BOOL CDatabaseManager::IsOpen()
{
	return m_DBopen;
}

//
// SERVICE RELATED METHODS
//

BOOL CDatabaseManager::LoadConfigIniFile(int nInstanceNumber, CString &sFileName, CString &sFileName2, CString &sFileName3, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;

	CRecordset Rs(m_pDB);
	CString sSQL, s;
	sSQL.Format("SELECT ConfigData,ConfigData2,ConfigData3 FROM ServiceConfigurations WHERE ServiceName='InputService' AND InstanceNumber=%d", nInstanceNumber);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format(_T("Query failed - %s"), (LPCSTR)sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, sFileName);
			Rs.GetFieldValue((short)1, sFileName2);
			Rs.GetFieldValue((short)2, sFileName3);
	
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format(_T("ERROR (DATABASEMGR): Query failed - %s"), (LPCSTR)e->m_strError);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;

}


BOOL CDatabaseManager::RegisterService(CString &sErrorMessage)
{
	CString sSQL, s;
	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return -1;
	
	CRecordset Rs(m_pDB);

	CString sComputer = g_util.GetComputerName();

	sSQL.Format("{CALL spRegisterService ('InputService', %d, %d, '%s',-1,'','','')}", g_prefs.m_instancenumber, SERVICETYPE_FILEIMPORT, sComputer);

	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Insert failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}

BOOL CDatabaseManager::InsertLogEntry(int nEvent, CString sSource,  CString sFileName, CString sMessage, int nMasterCopySeparationSet, int nVersion, int nMiscInt, CString sMiscString, CString &sErrorMessage)
{
	CString sSQL;
	
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;
	
	if (sMessage == "")
		sMessage = "Ok";

	sSQL.Format("{CALL [spAddServiceLogEntry] ('%s',%d,%d, '%s','%s', '%s',%d,%d,%d,'%s')}",
		_T("InputService"), g_prefs.m_instancenumber, nEvent, sSource, sFileName, sMessage, nMasterCopySeparationSet, nVersion, nMiscInt, sMiscString);

	try {
		m_pDB->ExecuteSQL( sSQL );
	}
	catch( CDBException* e ) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Insert failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch( CDBException* e ) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::UpdateService(int nCurrentState, CString sCurrentJob, CString sLastError, CString &sErrorMessage)
{
	return UpdateService(nCurrentState, sCurrentJob, sLastError, _T(""), sErrorMessage);
}

BOOL CDatabaseManager::UpdateService(int nCurrentState, CString sCurrentJob, CString sLastError, CString sAddedLogData, CString &sErrorMessage)
{
	CString sSQL;

	CString sComputer = g_util.GetComputerName();

	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;
	sSQL.Format("{CALL spRegisterService ('InputService', %d, 2, '%s',%d,'%s','%s','%s')}", g_prefs.m_instancenumber, sComputer, nCurrentState, sCurrentJob, sLastError, sAddedLogData);

	try {
		m_pDB->ExecuteSQL( sSQL );
	}
	catch( CDBException* e ) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch( CDBException* e ) {
			// So what..! Go on!;
		}
		return FALSE;
	}
 
	return TRUE;
}


BOOL CDatabaseManager::UpdatePrepollStatus(int nMasterCopySeparationSet, int serviceType, int nEventCode, CString sMessage, CString &sErrorMessage)
{
	CUtils util;
	CString sSQL;
	sErrorMessage = _T("");

	sSQL.Format("{ CALL spAddPrepollLogEntry (%d,%d,%d,%d,'%s') }", nMasterCopySeparationSet, g_prefs.m_instancenumber, serviceType, nEventCode, sMessage);
	//	util.Logprintf("INFO:  UpdatePrepollStatus - %s", sSQL);

	BOOL bSuccess = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;

	while (!bSuccess && nRetries-- > 0) {
		if (InitDB(sErrorMessage) == FALSE) {
			Sleep(g_prefs.m_QueryBackoffTime);
			continue;
		}

		try {
			m_pDB->ExecuteSQL(sSQL);
			bSuccess = TRUE;
			break;
		}

		catch (CDBException* e) {
			sErrorMessage.Format("DB Update failed - %s", e->m_strError);
			LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
			e->Delete();

			try {
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
		}
	}
	return bSuccess;
}


BOOL CDatabaseManager::LoadSTMPSetup(CString &sErrorMessage)
{
	sErrorMessage = _T("");

	CString sSQL = _T("SELECT TOP 1 SMTPServer,SMTPPort, SMTPUserName,SMTPPassword,UseSSL,SMTPConnectionTimeout,SMTPFrom,SMTPCC,SMTPTo,SMTPSubject FROM SMTPPreferences");
	CString s;
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			CString s;
			int fld = 0;
			Rs.GetFieldValue((short)fld++, g_prefs.m_emailsmtpserver);
			Rs.GetFieldValue((short)fld++, s);
			g_prefs.m_mailsmtpport = atoi(s);

			Rs.GetFieldValue((short)fld++, g_prefs.m_mailsmtpserverusername);
			Rs.GetFieldValue((short)fld++, g_prefs.m_mailsmtpserverpassword);
			Rs.GetFieldValue((short)fld++, s);
			
			g_prefs.m_mailusessl = atoi(s);
			
			Rs.GetFieldValue((short)fld++, s);
			g_prefs.m_emailtimeout = atoi(s);

			Rs.GetFieldValue((short)fld++, g_prefs.m_emailfrom);
			Rs.GetFieldValue((short)fld++, g_prefs.m_emailcc);
			Rs.GetFieldValue((short)fld++, g_prefs.m_emailto);
			Rs.GetFieldValue((short)fld++, g_prefs.m_emailsubject);

		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("Query failed - %s (%s)", e->m_strError, sSQL);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
			return FALSE;
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;

}

//
// TX-TRIGGER RELATED METHODS
//

BOOL CDatabaseManager::CallMapNamesCustomSP(int nInputID, CJobAttributes *pJob, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");

	BOOL bSuccess = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;
	
	sSQL.Format("{CALL spInputCustomRenamer (%d,'%s',%d,%d,%d,'%s',%d, %d, %d,%d,'%s')}",
		pJob->m_publicationID,
		TranslateDate(pJob->m_pubdate),
		pJob->m_pressrunID,
		pJob->m_editionID,
		pJob->m_sectionID,
		pJob->m_pagename,
		5, //g_prefs.GetColorID(pJob->m_colorname),
		pJob->m_locationID,
		nInputID,
		pJob->m_pagination,
		pJob->m_miscstring2);
	

	g_util.Logprintf("INFO:  Calling spInputCustomRenamer: %s", sSQL);


	CRecordset *Rs = NULL;

	while (!bSuccess && nRetries-- > 0) {

		try {
			if (InitDB(sErrorMessage) == FALSE) {
				Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				continue;
			}

			Rs = new CRecordset(m_pDB);

			if (!Rs->Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
				Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				delete Rs;
				Rs = NULL;
				continue;
			}
			int nFields = Rs->GetODBCFieldCount();
			if (!Rs->IsEOF()) {

				CString s;
				int f = 0;
				Rs->GetFieldValue((short)f++, s);
				pJob->m_publicationID = atoi(s);
				CDBVariant DBvar;
				Rs->GetFieldValue((short)f++, DBvar, SQL_C_TIMESTAMP);
				CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
				pJob->m_pubdate = t;

				Rs->GetFieldValue((short)f++, s);
				pJob->m_pressrunID = atoi(s);
				Rs->GetFieldValue((short)f++, s);
				pJob->m_editionID = atoi(s);
				Rs->GetFieldValue((short)f++, s);
				pJob->m_sectionID = atoi(s);
				Rs->GetFieldValue((short)f++, s);
				pJob->m_pagename = s;
				Rs->GetFieldValue((short)f++, s);
				pJob->m_colorname = _T("PDF");
				Rs->GetFieldValue((short)f++, s);
				pJob->m_locationID = atoi(s);

				if (nFields > f) {
					Rs->GetFieldValue((short)f++, s);
					pJob->m_pagination = atoi(s);
				}
				if (nFields > f) {
					Rs->GetFieldValue((short)f++, s);
					pJob->m_miscstring2 = s;
				}


				if (atoi(pJob->m_pagename) > 0)
					pJob->m_pageindex = atoi(pJob->m_pagename);
				g_util.Logprintf("INFO:  spInputCustomRenamer returned: PubID=%d,PubDate=%.2d.%.2d.%.2d, IssueID=%d, EditionID=%d, SectionID=%d, Pagename=%s, Pagination=%d  Color=%s,LocationID=%d, MiscString2=%s", pJob->m_publicationID, pJob->m_pubdate.GetYear(), pJob->m_pubdate.GetMonth(), pJob->m_pubdate.GetDay(), pJob->m_pressrunID, pJob->m_editionID, pJob->m_sectionID, pJob->m_pagename, pJob->m_pagination, pJob->m_colorname, pJob->m_locationID, pJob->m_miscstring2);
			}
			Rs->Close();
			bSuccess = TRUE;
			break;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();
			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
		}
	}

	if (Rs != NULL)
		delete Rs;
	return bSuccess;
}


BOOL CDatabaseManager::LoadAllPrefs(CString &sErrorMessage)
{
	if (LoadGeneralPreferences(sErrorMessage) <= 0)
		return FALSE;

	LoadSTMPSetup(sErrorMessage);

	if (LoadPublicationList(sErrorMessage) == FALSE)
		return FALSE;

	if (LoadSectionNameList(sErrorMessage) == FALSE)
		return FALSE;

	if (LoadEditionNameList(sErrorMessage) == FALSE)
		return FALSE;


	if (LoadFileServerList(sErrorMessage) == FALSE)
		return FALSE;


	if (LoadAliasList(sErrorMessage) == FALSE)
		return FALSE;


	g_prefs.m_StatusList.RemoveAll();
	if (LoadStatusList(_T("StatusCodes"), g_prefs.m_StatusList, sErrorMessage) == FALSE)
		return FALSE;

	g_prefs.m_ExternalStatusList.RemoveAll();
	if (LoadStatusList(_T("ExternalStatusCodes"), g_prefs.m_ExternalStatusList, sErrorMessage) == FALSE)
		return FALSE;

	/*	if (LoadPublisherList(sErrorMessage) == -1)
		return FALSE;

	g_util.Logprintf("INFO: Loading LoadChannelGroupList.."); 
	if (LoadChannelGroupList(sErrorMessage) == -1)
		return FALSE;

	if (LoadChannelList(sErrorMessage) == -1)
		return FALSE;
*/
	g_util.Logprintf("INFO: Loading LoadInputProcessList..");
	if (LoadInputProcessList(sErrorMessage) == FALSE)
		return FALSE;
	g_util.Logprintf("INFO: LoadedLoadInputProcessList..");
	return TRUE;
}


// Returns : -1 error
//			  1 found data
//            0 No data (fatal)
int	CDatabaseManager::LoadGeneralPreferences(CString &sErrorMessage)
{
	CUtils util;
	BOOL foundData = FALSE;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;

	CRecordset Rs(m_pDB);

	CString sSQL = _T("SELECT TOP 1 ServerName,ServerShare,ServerFilePath,ServerPreviewPath,ServerThumbnailPath,ServerLogPath,ServerConfigPath,ServerErrorPath,CopyProofToWeb,WebProofPath,ServerUseCurrentUser,ServerUserName,ServerPassword FROM GeneralPreferences");
	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return -1;
		}

		if (!Rs.IsEOF()) {
			CString s;
			int fld = 0;
			Rs.GetFieldValue((short)fld++, g_prefs.m_serverName);
			Rs.GetFieldValue((short)fld++, g_prefs.m_serverShare);

			Rs.GetFieldValue((short)fld++, g_prefs.m_hiresPDFPath);
			if (g_prefs.m_hiresPDFPath.Right(1) != "\\")
				g_prefs.m_hiresPDFPath += _T("\\");

			g_prefs.m_lowresPDFPath = g_prefs.m_hiresPDFPath;
			g_prefs.m_lowresPDFPath.MakeUpper();
			g_prefs.m_lowresPDFPath.Replace("CCFILESHIRES", "CCFILESLOWRES");

			g_prefs.m_printPDFPath = g_prefs.m_hiresPDFPath;
			g_prefs.m_printPDFPath.MakeUpper();
			g_prefs.m_printPDFPath.Replace("CCFILESHIRES", "CCFILESPRINT");


			Rs.GetFieldValue((short)fld++, g_prefs.m_previewPath);
			if (g_prefs.m_previewPath.Right(1) != "\\")
				g_prefs.m_previewPath += _T("\\");

			g_prefs.m_readviewpreviewPath = g_prefs.m_previewPath;
			g_prefs.m_readviewpreviewPath.MakeUpper();
			g_prefs.m_readviewpreviewPath.Replace("CCPREVIEWS", "CCreadviewpreviews");



			Rs.GetFieldValue((short)fld++, g_prefs.m_thumbnailPath);
			if (g_prefs.m_thumbnailPath.Right(1) != "\\")
				g_prefs.m_thumbnailPath += _T("\\");
			Rs.GetFieldValue((short)fld++, s);
		
			Rs.GetFieldValue((short)fld++, g_prefs.m_configPath);
			if (g_prefs.m_configPath.Right(1) != "\\")
				g_prefs.m_configPath += _T("\\");
			Rs.GetFieldValue((short)fld++, g_prefs.m_errorPath);
			if (g_prefs.m_errorPath.Right(1) != "\\")
				g_prefs.m_errorPath += _T("\\");


			Rs.GetFieldValue((short)fld++, s);
			g_prefs.m_copyToWeb = atoi(s);

			Rs.GetFieldValue((short)fld++, g_prefs.m_webPath);
			if (g_prefs.m_webPath.Right(1) != "\\")
				g_prefs.m_webPath += _T("\\");

			Rs.GetFieldValue((short)fld++, s);
			g_prefs.m_mainserverusecussrentuser = atoi(s);

			Rs.GetFieldValue((short)fld++, g_prefs.m_mainserverusername);
			Rs.GetFieldValue((short)fld++, g_prefs.m_mainserverpassword);



			foundData = TRUE;
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("Query failed - %s (%s)", e->m_strError, sSQL);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
			return -1;
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return -1;
	}

	if (foundData == 0)
		sErrorMessage = _T("FATAL ERROR: No data found in GeneralPrefences table");
	return foundData;
}


BOOL CDatabaseManager::LoadPublicationList(CString &sErrorMessage)
{
	sErrorMessage =  _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	CString sSQL, s;
	g_prefs.m_PublicationList.RemoveAll();

	sSQL.Format("SELECT DISTINCT PublicationID,Name,PageFormatID,TrimToFormat,LatestHour,DefaultProofID,DefaultHardProofID,DefaultApprove,DefaultCopies FROM PublicationNames ORDER BY Name");

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			LogprintfDB("Query failed : %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			PUBLICATIONSTRUCT *pItem = new PUBLICATIONSTRUCT;
			Rs.GetFieldValue((short)0, s);
			pItem->m_ID = atoi(s);

			Rs.GetFieldValue((short)1, pItem->m_name);
			Rs.GetFieldValue((short)2, s);
			pItem->m_PageFormatID = atoi(s);
			Rs.GetFieldValue((short)3, s);
			pItem->m_TrimToFormat = atoi(s);
			Rs.GetFieldValue((short)4, s);
			pItem->m_LatestHour = atof(s);
			Rs.GetFieldValue((short)5, s);
			pItem->m_ProofID = atoi(s);
			Rs.GetFieldValue((short)6, s);
			pItem->m_HardProofID = atoi(s);
			Rs.GetFieldValue((short)7, s);
			pItem->m_Approve = atoi(s);
			Rs.GetFieldValue((short)8, s);
			int n = atoi(s);
			pItem->m_allowunplanned = (n & 0x01) > 0 ? TRUE : FALSE;
			pItem->m_autoassignchannelstounplanned = (n & 0x02) > 0 ? TRUE : FALSE;

			g_prefs.m_PublicationList.Add(*pItem);

			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	for (int i = 0; i < g_prefs.m_PublicationList.GetCount(); i++)
		LoadPublicationNameExtension(g_prefs.m_PublicationList[i].m_ID, sErrorMessage);
	
//	for (int i = 0; i < g_prefs.m_PublicationList.GetCount(); i++)
//		g_prefs.ResetPublicationPressDefaults(g_prefs.m_PublicationList[i].m_ID);

//	for (int i = 0; i < g_prefs.m_PublicationList.GetCount(); i++)
//		RetrievePublicationPressDefaults(&g_prefs.m_PublicationList[i], szErrorMessage2);

	for (int i = 0; i < g_prefs.m_PublicationList.GetCount(); i++)
		LoadPublicationChannels(g_prefs.m_PublicationList[i].m_ID, sErrorMessage);

	return TRUE;
}

/*
int CDatabaseManager::LoadChannelList(CString &sErrorMessage)
{

	int foundJob = 0;
	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return -1;

	g_prefs.m_ChannelList.RemoveAll();
	CRecordset Rs(m_pDB);

	CString sSQL = _T("SELECT ChannelID,Name,Enabled,UseReleaseTime,ReleaseTime,ISNULL(ChannelGroupID,0),ISNULL(PublisherID,0) FROM ChannelNames WHERE ChannelID>0 AND Name<>''  ORDER BY Name");
	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return -1;
		}

		while (!Rs.IsEOF()) {
			CString s;
			int fld = 0;
			CHANNELSTRUCT *pItem = new CHANNELSTRUCT();

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_channelID = atoi(s);

			Rs.GetFieldValue((short)fld++, pItem->m_channelname);

			Rs.GetFieldValue((short)fld++, s); // Enabled
			pItem->m_enabled = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_usereleasetime = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			int n = atoi(s);
			pItem->m_releasetimehour = n / 100;
			pItem->m_releasetimeminute = n - pItem->m_releasetimehour;



			Rs.GetFieldValue((short)fld++, s);
			pItem->m_channelgroupID = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_publisherID = atoi(s);

			g_prefs.m_ChannelList.Add(*pItem);
			foundJob++;

			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		sErrorMessage.Format("Query failed - %s (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
			return -1;
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return -1;
	}

	return foundJob;
}
*/




BOOL CDatabaseManager::LoadPublicationChannels(int nID, CString &sErrorMessage)
{
	sErrorMessage = _T("");

	PUBLICATIONSTRUCT *pItem = g_prefs.GetPublicationStruct(nID);
	if (pItem == NULL)
		return FALSE;

	pItem->m_channelIDList = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	CString sSQL, s;

	CString sList = _T("");
	sSQL.Format("SELECT DISTINCT ChannelID FROM PublicationChannels WHERE PublicationID=%d", nID);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {

			Rs.GetFieldValue((short)0, s);
			if (atoi(s) > 0) {
				if (sList != "")
					sList += ",";
				sList += s;
			}
			Rs.MoveNext();
		}
		pItem->m_channelIDList = sList;
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}


BOOL CDatabaseManager::LoadPublicationNameExtension(int nID, CString &sErrorMessage)
{
	sErrorMessage = _T("");

	PUBLICATIONSTRUCT *pItem = g_prefs.GetPublicationStructNoDbLookup(nID);
	if (pItem == NULL)
		return FALSE;

	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	CString sSQL, s;

	sSQL.Format("SELECT ISNULL(CustomerID,0), ISNULL(AutoPurgeKeepDays,0), ISNULL(EmailRecipient,''), ISNULL(EmailCC,''), ISNULL(EmailSubject,''), ISNULL(EmailBody,''), ISNULL(UploadFolder,''),ISNULL(DefaultTemplateID,0),ISNULL(DefaultCopies,0),ISNULL(InputAlias,''),ISNULL(ExtendedAlias,''),ISNULL(PublisherID,0) FROM PublicationNames WHERE PublicationID=%d", nID);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {

			Rs.GetFieldValue((short)0, s);
			pItem->m_customerID = atoi(s);
			Rs.GetFieldValue((short)1, s);
			pItem->m_autoPurgeKeepDays = atoi(s);
			Rs.GetFieldValue((short)2, pItem->m_emailrecipient);
			Rs.GetFieldValue((short)3, pItem->m_emailCC);
			Rs.GetFieldValue((short)4, pItem->m_emailSubject);
			Rs.GetFieldValue((short)5, pItem->m_emailBody);
			Rs.GetFieldValue((short)6, pItem->m_uploadfolder);

			pItem->m_annumtext = _T("");
			pItem->m_releasedays = 0;
			pItem->m_releasetimehour = 0;
			pItem->m_releasetimeminute = 0;

			pItem->m_pubdatemove = FALSE;
			pItem->m_pubdatemovedays = 0;

			int q = pItem->m_uploadfolder.Find(";");
			if (q != -1) {
				CStringArray sArr;
				q = g_util.StringSplitter(pItem->m_uploadfolder, ";", sArr);
				if (sArr.GetCount() > 0)
					pItem->m_uploadfolder = sArr[0];
				if (sArr.GetCount() > 1)
					pItem->m_annumtext = sArr[1];
				if (sArr.GetCount() > 2)
					pItem->m_releasedays = atoi(sArr[2]);
				if (sArr.GetCount() > 3)
					pItem->m_releasetimehour = atoi(sArr[3]);
				if (sArr.GetCount() > 4)
					pItem->m_releasetimeminute = atoi(sArr[4]);

				if (sArr.GetCount() > 7)
					pItem->m_pubdatemove = atoi(sArr[7]);
				if (sArr.GetCount() > 8)
					pItem->m_pubdatemovedays = atoi(sArr[8]);

				if (pItem->m_pubdatemove == FALSE)
					pItem->m_pubdatemovedays = 0;
			}


			Rs.GetFieldValue((short)7, s);
			pItem->m_tempdefafaultpressID = atoi(s);

			Rs.GetFieldValue((short)8, s); // defaultcopies
			int n = atoi(s);
			pItem->m_allowunplanned = (n & 0x01) > 0 ? TRUE : FALSE;
			pItem->m_autoassignchannelstounplanned = (n & 0x02) > 0 ? TRUE : FALSE;

			Rs.GetFieldValue((short)9, s);
			pItem->m_alias = s;

			Rs.GetFieldValue((short)10, s);
			pItem->m_extendedalias = s;

			Rs.GetFieldValue((short)11, s);
			pItem->m_publisherID = atoi(s);


		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

/*	pItem->m_extendedalias = _T("");
	sSQL.Format("SELECT TOP 1 ISNULL(Alias,'') FROM PublicationExtendedAliases WHERE PublicationID=%d", nID);
	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {

			Rs.GetFieldValue((short)0, pItem->m_extendedalias);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}*/
	return TRUE;
}

int CDatabaseManager::LoadNewPublicationID(CString sName, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return 0;

	CRecordset Rs(m_pDB);

	CString sSQL, s;
	int nID = 0;
	PUBLICATIONSTRUCT *pItem = NULL;
	sSQL.Format("SELECT PublicationID,PageFormatID,TrimToFormat,LatestHour,DefaultProofID,DefaultHardProofID,DefaultApprove,InputAlias FROM PublicationNames WHERE ([Name]='%s' OR InputAlias='%s')", sName, sName);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return 0;
		}

		if (!Rs.IsEOF()) {
			pItem = new PUBLICATIONSTRUCT;
			Rs.GetFieldValue((short)0, s);
			nID = atoi(s);
			pItem->m_ID = nID;
			pItem->m_name = sName;
			Rs.GetFieldValue((short)1, s);
			pItem->m_PageFormatID = atoi(s);
			Rs.GetFieldValue((short)2, s);
			pItem->m_TrimToFormat = atoi(s);
			Rs.GetFieldValue((short)3, s);
			pItem->m_LatestHour = atof(s);

			Rs.GetFieldValue((short)4, s);
			pItem->m_ProofID = atoi(s);
			Rs.GetFieldValue((short)5, s);
			pItem->m_HardProofID = atoi(s);
			Rs.GetFieldValue((short)6, s);
			pItem->m_Approve = atoi(s);

			Rs.GetFieldValue((short)7, pItem->m_alias);

			g_prefs.FlushPublicationName(sName);
			g_prefs.m_PublicationList.Add(*pItem);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return 0;
	}

	if (nID > 0 && pItem != NULL) 
		LoadPublicationNameExtension(nID, sErrorMessage);
	
	return nID;
}


CString CDatabaseManager::LoadNewPublicationName(int nID, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return 0;

	CRecordset Rs(m_pDB);

	CString sSQL, s;
	CString sName = _T("");
	PUBLICATIONSTRUCT *pItem = g_prefs.GetPublicationStructNoDbLookup(nID);
	BOOL bNewEntry = (pItem == NULL);
	sSQL.Format("SELECT [Name],PageFormatID,TrimToFormat,LatestHour,DefaultProofID,DefaultHardProofID,DefaultApprove,InputAlias FROM PublicationNames WHERE PublicationID=%d", nID);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return _T("");
		}

		if (!Rs.IsEOF()) {
			if (bNewEntry)
				pItem = new PUBLICATIONSTRUCT;
			pItem->m_ID = nID;

			Rs.GetFieldValue((short)0, sName);
			pItem->m_name = sName;
			
			Rs.GetFieldValue((short)1, s);
			pItem->m_PageFormatID = atoi(s);
			Rs.GetFieldValue((short)2, s);
			pItem->m_TrimToFormat = atoi(s);
			Rs.GetFieldValue((short)3, s);
			pItem->m_LatestHour = atof(s);

			Rs.GetFieldValue((short)4, s);
			pItem->m_ProofID = atoi(s);
			Rs.GetFieldValue((short)5, s);
			pItem->m_HardProofID = atoi(s);
			Rs.GetFieldValue((short)6, s);
			pItem->m_Approve = atoi(s);
			Rs.GetFieldValue((short)7, s);
			pItem->m_alias = s;

			if (bNewEntry)
				g_prefs.m_PublicationList.Add(*pItem);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return _T("");
	}

	if (nID > 0 && pItem != NULL)
		LoadPublicationNameExtension(nID, sErrorMessage);

	return sName;
}


BOOL CDatabaseManager::ReloadPublicationDetails(int nID, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	PUBLICATIONSTRUCT *pItem = g_prefs.GetPublicationStructNoDbLookup(nID);
	if (pItem == NULL)
		return FALSE;

	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	CString sSQL, s;

	sSQL.Format("SELECT PageFormatID,TrimToFormat,LatestHour,DefaultProofID,DefaultHardProofID,DefaultApprove,ISNULL(CustomerID,0), ISNULL(AutoPurgeKeepDays,0), ISNULL(EmailRecipient,''), ISNULL(EmailCC,''), ISNULL(EmailSubject,''), ISNULL(EmailBody,''), ISNULL(UploadFolder,''),ISNULL(DefaultTemplateID,0),ISNULL(InputAlias,'')  FROM PublicationNames WHERE PublicationID=%d", nID);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			pItem->m_PageFormatID = atoi(s);
			Rs.GetFieldValue((short)1, s);
			pItem->m_TrimToFormat = atoi(s);
			Rs.GetFieldValue((short)2, s);
			pItem->m_LatestHour = atof(s);

			Rs.GetFieldValue((short)3, s);
			pItem->m_ProofID = atoi(s);
			Rs.GetFieldValue((short)4, s);
			pItem->m_HardProofID = atoi(s);
			Rs.GetFieldValue((short)5, s);
			pItem->m_Approve = atoi(s);

			Rs.GetFieldValue((short)6, s);
			pItem->m_customerID = atoi(s);
			Rs.GetFieldValue((short)7, s);
			pItem->m_autoPurgeKeepDays = atoi(s);
			Rs.GetFieldValue((short)8, pItem->m_emailrecipient);
			Rs.GetFieldValue((short)9, pItem->m_emailCC);
			Rs.GetFieldValue((short)10, pItem->m_emailSubject);
			Rs.GetFieldValue((short)11, pItem->m_emailBody);
			Rs.GetFieldValue((short)12, pItem->m_uploadfolder);

			pItem->m_annumtext = _T("");
			pItem->m_releasedays = 0;
			pItem->m_releasetimehour = 0;
			pItem->m_releasetimeminute = 0;

			int q = pItem->m_uploadfolder.Find(";");
			if (q != -1) {
				CStringArray sArr;
				q = g_util.StringSplitter(pItem->m_uploadfolder, ";", sArr);
				if (sArr.GetCount() > 0)
					pItem->m_uploadfolder = sArr[0];
				if (sArr.GetCount() > 1)
					pItem->m_annumtext = sArr[1];
				if (sArr.GetCount() > 2)
					pItem->m_releasedays = atoi(sArr[2]);
				if (sArr.GetCount() > 3)
					pItem->m_releasetimehour = atoi(sArr[3]);
				if (sArr.GetCount() > 4)
					pItem->m_releasetimeminute = atoi(sArr[4]);
			}

			Rs.GetFieldValue((short)13, s);
			pItem->m_tempdefafaultpressID = atoi(s);

			Rs.GetFieldValue((short)14, s);
			pItem->m_alias = s;

		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}



	return nID;
}

CString CDatabaseManager::LoadNewEditionName(int nID, CString &sErrorMessage)
{
	sErrorMessage = _T( "");
	if (InitDB(sErrorMessage) == FALSE)
		return "";

	CRecordset Rs(m_pDB);

	CString sSQL;
	CString sEd = "";
	sSQL.Format("SELECT Name FROM EditionNames WHERE EditionID=%d", nID);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return "";
		}

		if (!Rs.IsEOF()) {
			ITEMSTRUCT *pItem = new ITEMSTRUCT();
			Rs.GetFieldValue((short)0, sEd);
			pItem->m_ID = nID;
			pItem->m_name = sEd;		
			g_prefs.m_EditionList.Add(*pItem);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return "";
	}

	return sEd;
}

int CDatabaseManager::LoadNewEditionID(CString sName, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return 0;

	CRecordset Rs(m_pDB);

	CString sSQL, s;
	int nID = 0;
	sSQL.Format("SELECT EditionID FROM EditionNames WHERE Name='%s'", sName);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return 0;
		}

		if (!Rs.IsEOF()) {
			ITEMSTRUCT *pItem = new ITEMSTRUCT();
			Rs.GetFieldValue((short)0, s);
			nID = atoi(s);
			pItem->m_ID = nID;
			pItem->m_name = sName;
			g_prefs.m_EditionList.Add(*pItem);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return 0;
	}

	return nID;
}


CString CDatabaseManager::LoadNewSectionName(int nID, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return "";

	CRecordset Rs(m_pDB);

	CString sSQL;
	CString sEd = "";
	sSQL.Format("SELECT Name FROM SectionNames WHERE SectionID=%d", nID);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return "";
		}

		if (!Rs.IsEOF()) {
			ITEMSTRUCT *pItem = new ITEMSTRUCT();
			Rs.GetFieldValue((short)0, sEd);
			pItem->m_ID = nID;
			pItem->m_name = sEd;
			g_prefs.m_SectionList.Add(*pItem);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return "";
	}

	return sEd;
}

int CDatabaseManager::LoadNewSectionID(CString sName, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return 0;

	CRecordset Rs(m_pDB);

	CString sSQL,s;
	int nID = 0;
	sSQL.Format("SELECT SectionID FROM SectionNames WHERE Name='%s'", sName);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return 0;
		}

		if (!Rs.IsEOF()) {
			ITEMSTRUCT *pItem = new ITEMSTRUCT();
			Rs.GetFieldValue((short)0, s);
			nID = atoi(s);
			pItem->m_ID = nID;
			pItem->m_name = sName;
			g_prefs.m_SectionList.Add(*pItem);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return 0;
	}

	return nID;
}

/*
CString CDatabaseManager::LoadNewPublisherName(int nID, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return "";

	CRecordset Rs(m_pDB);

	CString sSQL;
	CString sName = "";
	sSQL.Format("SELECT PublisherName FROM PublisherNames WHERE PublisherID=%d", nID);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return "";
		}

		if (!Rs.IsEOF()) {
			ITEMSTRUCT *pItem = new ITEMSTRUCT();
			Rs.GetFieldValue((short)0, sName);
			pItem->m_ID = nID;
			pItem->m_name = sName;
			g_prefs.m_PublisherList.Add(*pItem);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return "";
	}

	return sName;
}
*/


BOOL CDatabaseManager::LoadEditionNameList(CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	g_prefs.m_EditionList.RemoveAll();
	CString sSQL;
	CString s;

	sSQL = _T("SELECT EditionID,Name FROM EditionNames");

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			ITEMSTRUCT *pItem = new ITEMSTRUCT;
			Rs.GetFieldValue((short)0, s);
			pItem->m_ID = atoi(s);
			Rs.GetFieldValue((short)1, pItem->m_name);
			g_prefs.m_EditionList.Add(*pItem);
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}

BOOL CDatabaseManager::LoadSectionNameList(CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	g_prefs.m_SectionList.RemoveAll();
	CString sSQL;
	CString s;

	sSQL = _T("SELECT SectionID,Name FROM SectionNames");

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			ITEMSTRUCT *pItem = new ITEMSTRUCT;
			Rs.GetFieldValue((short)0, s);
			pItem->m_ID = atoi(s);
			Rs.GetFieldValue((short)1, pItem->m_name);
			g_prefs.m_SectionList.Add(*pItem);
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}


BOOL CDatabaseManager::LoadFileServerList(CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	CString sSQL, s;

	sSQL.Format("SELECT [Name],ServerType,CCdatashare,Username,password,IP,Locationid,uselocaluser FROM FileServers ORDER BY ServerType");

	g_prefs.m_FileServerList.RemoveAll();
	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			FILESERVERSTRUCT *pItem = new FILESERVERSTRUCT();

			Rs.GetFieldValue((short)0, pItem->m_servername);

			Rs.GetFieldValue((short)1, s);
			pItem->m_servertype = atoi(s);

			Rs.GetFieldValue((short)2, pItem->m_sharename);

			Rs.GetFieldValue((short)3, pItem->m_username);

			Rs.GetFieldValue((short)4, pItem->m_password);

			Rs.GetFieldValue((short)5, pItem->m_IP);

			pItem->m_IP.Trim();

			Rs.GetFieldValue((short)6, s);
			pItem->m_locationID = atoi(s);

			Rs.GetFieldValue((short)7, s);
			pItem->m_uselocaluser = atoi(s);

			g_prefs.m_FileServerList.Add(*pItem);
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}


BOOL CDatabaseManager::LoadStatusList(CString sIDtable, STATUSLIST &v, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	sSQL.Format("SELECT * FROM %s", sIDtable);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			STATUSSTRUCT *pItem = new STATUSSTRUCT;
			CString s;
			Rs.GetFieldValue((short)0, s);
			pItem->m_statusnumber = atoi(s);
			Rs.GetFieldValue((short)1, pItem->m_statusname);
			Rs.GetFieldValue((short)2, pItem->m_statuscolor);
			v.Add(*pItem);
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}




BOOL CDatabaseManager::LoadAliasList(CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	g_prefs.m_AliasList.RemoveAll();

	CString sSQL;
	sSQL = _T("SELECT Type,Longname,Shortname FROM InputAliases");

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			ALIASSTRUCT *pItem = new ALIASSTRUCT();
			CString s;
			Rs.GetFieldValue((short)0, s);
			pItem->sType = s;
			Rs.GetFieldValue((short)1, s);
			pItem->sLongName = s;
			Rs.GetFieldValue((short)2, s);
			pItem->sShortName = s;
			g_prefs.m_AliasList.Add(*pItem);
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}

CString CDatabaseManager::LoadSpecificAlias(CString sType, CString sLongName, CString &sErrorMessage)
{

	CString sShortName = sLongName;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return sShortName;

	CRecordset Rs(m_pDB);

	CString sSQL;
	sSQL.Format("SELECT Shortname FROM InputAliases WHERE Type='%s' AND LongName='%s'", sType, sLongName);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, sShortName);

			ALIASSTRUCT *pItem = new ALIASSTRUCT();
			pItem->sType = sType;
			pItem->sLongName = sLongName;
			pItem->sShortName = sShortName;
			g_prefs.FlushAlias(sType, sLongName);
			g_prefs.m_AliasList.Add(*pItem);
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return sShortName;
	}
	return sShortName;
}


BOOL CDatabaseManager::LoadInputProcessList(CString &sErrorMessage)
{

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	g_prefs.m_InputProcessList.RemoveAll();

	CString sSQL, s;
	CStringArray sArr;
	int fld = 0;
	sSQL.Format("SELECT InputID, InputName,Enabled,LocationID,InputPath,SearchMask,StableTime,PollTime,UseRegExp,NamingMask,MaskType,DefaultPublicationID,DefaultSectionID,DefaultEditionID,DefaultIssueID,DefaultTemplateID,DefaultProofTemplateID,DefaultLocationID,DefaultCopies,MayAddColor,MayAddPages,ApprovalRequired,PutOnHold,Priority,UseLatestDate,GuessSection,GuessEdition,GuessIssue,GuessLocation,FTPdownload,FTPserver,FTPusername,FTPpassword,FTPfolder,FTPport,PDFzip,PDFsplit,JBIGcompress,CopyEditions,CopyIssues,UseCurrentUser,UserName,PassWord,FolderScanOrder,CheckTiff,KillEmptySeparations,CheckImageSize,MinPixelWidth,MinPixelHeight,MaxPixelWidth,MaxPixelHeight,CallScript,ScriptName,LocationEnableList,LocationTemplateList,MakeCopy,Separators,PubDatePlus,RollOverTime,MarkGroupFlags,MarkGroupSymbolList,MarkGroupList,CopyFolder,FTPPasv,FTPXCRC,FTPTLS, InputType,SendAckFile,AckFileFolder,AckFlagValue FROM InputConfigurations");
	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return FALSE;
		}

		while (!Rs.IsEOF()) {
			INPUTPROCESSSTRUCT *pItem = new INPUTPROCESSSTRUCT();
			fld = 0;
			Rs.GetFieldValue((short)fld++, s);
			pItem->m_ID = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_inputname);
			Rs.GetFieldValue((short)fld++, s);
			pItem->m_enabled = atoi(s);
			Rs.GetFieldValue((short)fld++, s);
			pItem->m_locationID = atoi(s);

			Rs.GetFieldValue((short)fld++, pItem->m_inputpath);
			Rs.GetFieldValue((short)fld++, pItem->m_searchmask);
			Rs.GetFieldValue((short)fld++, s);
			pItem->m_stabletime = atoi(s);
			Rs.GetFieldValue((short)fld++, s);
			pItem->m_polltime = atoi(s);
			Rs.GetFieldValue((short)fld++, s);
			pItem->m_useregexp = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_namingmask);
			Rs.GetFieldValue((short)fld++, s);
			pItem->m_masktype = atoi(s);
			Rs.GetFieldValue((short)fld++, s);
			pItem->m_defaultpublicationID = atoi(s);
			Rs.GetFieldValue((short)fld++, s);
			pItem->m_defaultsectionID = atoi(s);
			Rs.GetFieldValue((short)fld++, s);
			pItem->m_defaulteditionID = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_defaulttemplateID = atoi(s);
			Rs.GetFieldValue((short)fld++, s);
			pItem->m_defaultrunID = atoi(s);
			Rs.GetFieldValue((short)fld++, s);
			pItem->m_defaultprooftemplateID = atoi(s);

//			g_util.Logprintf("INFO:   m_defaultprooftemplateID=%d for Input ID %d",pItem->m_defaultprooftemplateID,pItem->m_ID);

			Rs.GetFieldValue((short)fld++, s);

			DWORD x = (DWORD)atoi(s);
			pItem->m_defaultlocationID = x & 0xFF;
			int nDefaultPressGroupID = x >> 16;
			//pItem->m_defaultpressgroup = g_prefs.GetPressGroupName(nDefaultPressGroupID);
			pItem->m_defaultpressgroup = _T("");



			Rs.GetFieldValue((short)fld++, s);
			pItem->m_defaultcopies = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_mayaddcolor = atoi(s);
			Rs.GetFieldValue((short)fld++, s);
			pItem->m_mayaddpage = atoi(s);
			Rs.GetFieldValue((short)fld++, s);
			pItem->m_approvalrequired = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_putonhold = atoi(s);
			Rs.GetFieldValue((short)fld++, s);
			pItem->m_priority = atoi(s);
			Rs.GetFieldValue((short)fld++, s);
			pItem->m_uselatestdate = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_guesssection = atoi(s);
			Rs.GetFieldValue((short)fld++, s);
			pItem->m_guessedition = atoi(s);
			Rs.GetFieldValue((short)fld++, s);
			pItem->m_guessrun = atoi(s);
			Rs.GetFieldValue((short)fld++, s);
			x = (DWORD)atoi(s);
			pItem->m_guesslocation = 0x01 & x;
			pItem->m_fixedpressgroup = (0x02 & x) > 0 ? TRUE : FALSE;

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_useFTP = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_FTPserver);
			Rs.GetFieldValue((short)fld++, pItem->m_FTPusername);
			Rs.GetFieldValue((short)fld++, pItem->m_FTPpassword);
			Rs.GetFieldValue((short)fld++, pItem->m_FTPfolder);
			Rs.GetFieldValue((short)fld++, s);
			pItem->m_FTPport = atoi(s);

			Rs.GetFieldValue((short)fld++, s);  //zip
			x = (DWORD)atoi(s);
			pItem->m_zipPDF = 0x01 & x;
			pItem->m_overruleproofer = (0x02 & x) > 0 ? TRUE : FALSE;
			pItem->m_overruledproofconfigurationID = x >> 2;
			pItem->m_overruledproofconfigurationID = pItem->m_overruledproofconfigurationID & 0xFF;


			Rs.GetFieldValue((short)fld++, s);  //PDFsplit

			x = (DWORD)atoi(s);
			pItem->m_splitPDF = 0x01 & x;
			pItem->m_registerunknownedition = (0x02 & x) > 0 ? TRUE : FALSE;
			pItem->m_registerunknownissues = (0x04 & x) > 0 ? TRUE : FALSE;
			pItem->m_registerunknownpublications = (0x08 & x) > 0 ? TRUE : FALSE;
			pItem->m_registerunknownsections = (0x10 & x) > 0 ? TRUE : FALSE;
			pItem->m_registerunknowncolors = (0x20 & x) > 0 ? TRUE : FALSE;
			pItem->m_flowdrive = (0x40 & x) > 0 ? TRUE : FALSE;
			pItem->m_txafterapproval = (0x80 & x) > 0 ? TRUE : FALSE;
			pItem->m_transmitoriginalname = (0x100 & x) > 0 ? TRUE : FALSE;
			pItem->m_onlygraymononame = (0x200 & x) > 0 ? TRUE : FALSE;
			pItem->m_callrenamessp = (0x400 & x) > 0 ? TRUE : FALSE;
			pItem->m_unplannedrequiresapply = (0x800 & x) > 0 ? TRUE : FALSE;
			pItem->m_saveoldversions = (0x1000 & x) > 0 ? TRUE : FALSE;
			pItem->m_makecopy = (0x2000 & x) > 0 ? TRUE : FALSE;
			pItem->m_autoinvert = (0x4000 & x) > 0 ? TRUE : FALSE;
			pItem->m_makecopylocationname = (0x8000 & x) > 0 ? TRUE : FALSE;
			pItem->m_ftppasv = (0x10000 & x) > 0 ? TRUE : FALSE;
			pItem->m_makecopyalways = (0x40000 & x) > 0 ? TRUE : FALSE;


			Rs.GetFieldValue((short)fld++, s);  //jbig
			x = (DWORD)atoi(s);
			pItem->m_detectpanorama = 0x01 & x;
			pItem->m_overruledrelease_putonhold = (0x04 & x) > 0 ? TRUE : FALSE;
			pItem->m_overruledapproval_approvalrequired = (0x08 & x) > 0 ? TRUE : FALSE;
			pItem->m_overruleapproval = (0x10 & x) > 0 ? TRUE : FALSE;
			pItem->m_overrulerelease = (0x20 & x) > 0 ? TRUE : FALSE;
			pItem->m_rejectidentical = (0x40 & x) > 0 ? TRUE : FALSE;
			pItem->m_allowpastpubdate = (0x80 & x) > 0 ? TRUE : FALSE;

			pItem->m_ignoreupdatesonalreadyapproved = (0x100 & x) > 0 ? TRUE : FALSE;
			pItem->m_allowtimededitions = (0x200 & x) > 0 ? TRUE : FALSE;
			pItem->m_rejectifnoregularexpressionmatch = (0x2000 & x) > 0 ? TRUE : FALSE;
			pItem->m_retryunknownfiles = (0x10000 & x) > 0 ? 1 : 0;
			pItem->m_lowpriorityfolder = (0x20000 & x) > 0 ? 1 : 0;
			pItem->m_transmitoriginalpdf = (0x40000 & x) > 0 ? TRUE : FALSE;

			Rs.GetFieldValue((short)fld++, s);		//copyeditions
			x = (DWORD)atoi(s);
			pItem->m_copyeditions = 0x01 & x;

			pItem->m_overrulepageformat = (0x10000 & x) > 0 ? TRUE : FALSE;
			pItem->m_overruledpageformatID = x >> 17;
			pItem->m_overruledpageformatID = pItem->m_overruledpageformatID & 0x0FFF;

			Rs.GetFieldValue((short)fld++, s);		//copyissue
			x = (DWORD)atoi(s);
			pItem->m_useweekreference = 0x01 & x;
			pItem->m_reapproveaction = x >> 1;
			pItem->m_reapproveaction = pItem->m_reapproveaction & 0x03;

			Rs.GetFieldValue((short)fld++, s);		// usecurrentuser
			pItem->m_usecurrentuser = atoi(s);
			Rs.GetFieldValue((short)fld++, pItem->m_username);
			Rs.GetFieldValue((short)fld++, pItem->m_password);

			Rs.GetFieldValue((short)fld++, s);		// m_folderscanorder
			x = (DWORD)atoi(s);
			pItem->m_folderscanorder = 0xFF & x;




			Rs.GetFieldValue((short)fld++, s);		// checktiff
			x = (DWORD)atoi(s);
			pItem->m_autogenerateplate = (0x02 & x) > 0 ? TRUE : FALSE;
			pItem->m_autogenerateplatehold = (0x04 & x) > 0 ? TRUE : FALSE;
			pItem->m_autogenerateplateapproval = (0x08 & x) > 0 ? TRUE : FALSE;
			pItem->m_autogenerateplatefreedevice = (0x10 & x) > 0 ? TRUE : FALSE;

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_killemptyseps = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_checkimagesize = atoi(s);

			Rs.GetFieldValue((short)fld++, s);	//minpixelwidth

			Rs.GetFieldValue((short)fld++, s);	//minpixelheight

			Rs.GetFieldValue((short)fld++, s);	//maxpixelw

			Rs.GetFieldValue((short)fld++, s);	//maxpixelh

			Rs.GetFieldValue((short)fld++, s);	// callscript
			x = (DWORD)atoi(s);
			pItem->m_callscript = 0x01 & x;
			pItem->m_callconditionalcopyscript = (0x02 & x) > 0 ? TRUE : FALSE;

			Rs.GetFieldValue((short)fld++, s);	//scriptname
			pItem->m_conditionalcopysp = _T("");
			int nn = s.Find(';');
			if (nn != -1 && nn>0) {
				//pItem->m_conditionalcopysp = s.Mid(nn + 1);
				pItem->m_scriptfile = s.Left(nn);
			}
			else
				pItem->m_scriptfile = s;



			Rs.GetFieldValue((short)fld++, s);  //locationenablelist
			
			pItem->m_locationenable[0] = TRUE;
//			int n = g_util.StringSplitter(s, ",;", sArr);
//			for (int i = 0; i < n; i++)
//				pItem->m_locationenable[i] = atoi(sArr[i]);

			Rs.GetFieldValue((short)fld++, s);	//locationtemplatelist
			pItem->m_locationtemplate[0] = _T("");
//			n = g_util.StringSplitter(s, ",;", sArr);
//			for (int i = 0; i < n; i++)
//				pItem->m_locationtemplate[i] = sArr[i];

			Rs.GetFieldValue((short)fld++, s);	// MakeCopy
			//pItem->m_changeifgray = atoi(s);
			pItem->m_makecopy = atoi(s);

			Rs.GetFieldValue((short)fld++, pItem->m_separators); // Separat

			Rs.GetFieldValue((short)fld++, s);	//pubdateplus
			pItem->m_pubdateplus = atoi(s);


			CDBVariant DBvar;
			Rs.GetFieldValue((short)fld++, DBvar, SQL_C_TIMESTAMP);//roollovertime
			if (DBvar.m_pdate->year == 0)
				pItem->m_rollovertime = CTime(1975, 1, 1, 0, 0, 0, 0);
			else {
				CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
				pItem->m_rollovertime = t;
			}
			Rs.GetFieldValue((short)fld++, s);	
//			n = g_util.StringSplitter(s, ",", sArr);
//			if (n > 4)
//				n = 4;
//			for (int i = 0; i < n; i++)
//				pItem->m_markgroupflags[i] = atoi(sArr[i]);

			Rs.GetFieldValue((short)fld++, s);
//			n = g_util.StringSplitter(s, ",", sArr);
//			if (n > 4)
//				n = 4;
//			for (int i = 0; i < n; i++)
//			pItem->m_markgroupsymbols[i] = sArr[i];

			Rs.GetFieldValue((short)fld++, s);
	//		n = g_util.StringSplitter(s, ",", sArr);
	//		if (n > 4)
	//			n = 4;
//			for (int i = 0; i < n; i++)
//				pItem->m_markgroups[i] = sArr[i];

			pItem->m_graycolorname = _T("Gray");
			Rs.GetFieldValue((short)fld++, pItem->m_copyfolder);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_ftppasv = atoi(s);
			Rs.GetFieldValue((short)fld++, s);
			pItem->m_ftpxcrc = atoi(s);
			Rs.GetFieldValue((short)fld++, s);
			pItem->m_ftptls = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_inputtype = atoi(s);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_sendackfile = atoi(s);

			Rs.GetFieldValue((short)fld++, pItem->m_ackfilefolder);

			Rs.GetFieldValue((short)fld++, s);
			pItem->m_ackflagvalue = atoi(s);


			g_prefs.m_InputProcessList.Add(*pItem);
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	for (int i = 0; i < g_prefs.m_InputProcessList.GetCount(); i++)
		LoadExpressionList(&g_prefs.m_InputProcessList[i], sErrorMessage);

	return TRUE;
}



BOOL CDatabaseManager::LoadExpressionList(INPUTPROCESSSTRUCT *pInput, CString  &sErrorMessage)
{

	int foundJob = 0;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	if (pInput == NULL)
		return FALSE;


	CString sSQL;
	sSQL.Format("SELECT UseExpression,MatchExpression,FormatExpression,PartialMatch,Rank,Comment FROM RegularExpressions WHERE InputID=%d AND PartialMatch<2 ORDER BY Rank", pInput->m_ID);

	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", sSQL);
			return FALSE;
		}
		pInput->m_NumberOfRegExps = 0;
		while (!Rs.IsEOF()) {
			CString s;
			int fld = 0;

			Rs.GetFieldValue((short)fld++, s);
			pInput->m_RegExpList[pInput->m_NumberOfRegExps].m_useExpression = atoi(s);
			Rs.GetFieldValue((short)fld++, pInput->m_RegExpList[pInput->m_NumberOfRegExps].m_matchExpression);
			Rs.GetFieldValue((short)fld++, pInput->m_RegExpList[pInput->m_NumberOfRegExps].m_formatExpression);
			Rs.GetFieldValue((short)fld++, s);
			pInput->m_RegExpList[pInput->m_NumberOfRegExps].m_partialmatch = atoi(s);
			Rs.GetFieldValue((short)fld++, pInput->m_RegExpList[pInput->m_NumberOfRegExps].m_comment);
			pInput->m_NumberOfRegExps++;
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format( "Query failed - %s (%s)", e->m_strError, sSQL);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
			return FALSE;
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	sSQL.Format("SELECT MatchExpression,FormatExpression,Rank,Comment FROM RegularExpressions WHERE InputID=%d AND PartialMatch=2 ORDER BY Rank", pInput->m_ID);
	try {
		if (!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format( "Query failed - %s", sSQL);
			return FALSE;
		}
		pInput->m_SortNumberOfRegExps = 0;
		CString s;
		while (!Rs.IsEOF()) {

			Rs.GetFieldValue((short)0, pInput->m_SortRegExpList[pInput->m_SortNumberOfRegExps].m_matchExpression);
			Rs.GetFieldValue((short)1, pInput->m_SortRegExpList[pInput->m_SortNumberOfRegExps].m_formatExpression);
			Rs.GetFieldValue((short)2, s); // Dummy read
			Rs.GetFieldValue((short)3, pInput->m_SortRegExpList[pInput->m_SortNumberOfRegExps].m_comment);
			
			pInput->m_SortNumberOfRegExps++;
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("Query failed - %s (%s)", e->m_strError, sSQL);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
			return FALSE;
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}

CTime CDatabaseManager::GetDatabaseTime(CString &sErrorMessage)
{
	CString sSQL;
	CTime tm(1975, 1, 1, 0, 0, 0);
	CDBVariant DBvar;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return tm;

	CRecordset Rs(m_pDB);

	sSQL.Format("SELECT GETDATE()");

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			CDBVariant DBvar;
			Rs.GetFieldValue((short)0, DBvar, SQL_C_TIMESTAMP);
			CTime tPubTime(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
			tm = tPubTime;
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return tm;
	}
	return tm;
}

BOOL CDatabaseManager::PlanLock(int bRequestedPlanLock, int &bCurrentPlanLock, CString &sClientName, CString &sClientTime, CString &sErrorMessage)
{
	CString sSQL;
	sClientTime = _T("");
	sClientName = _T("");
	bCurrentPlanLock = PLANLOCK_UNKNOWN;

	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	TCHAR buf[260];
	DWORD len = 260;
	::GetComputerName(buf, &len);
	sSQL.Format("{CALL spPlanningLock (%d,'INPUTCENTER %s')}", bRequestedPlanLock, buf);
	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			CString s;
			Rs.GetFieldValue((short)0, s);
			bCurrentPlanLock = atoi(s);
			Rs.GetFieldValue((short)1, sClientName);
			CDBVariant DBvar;
			Rs.GetFieldValue((short)2, DBvar, SQL_C_TIMESTAMP);
			sClientTime.Format("%.4d-%.2d-%.2d %.2d:%.2d:%.2d", DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		e->GetErrorMessage(buf, 260);
		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}
	return TRUE;
}


BOOL CDatabaseManager::PublicationPlanLock(int nPublicationID, CTime tPubDate, CTime  tPubDate2, int bRequestedPlanLock, int &bCurrentPlanLock, CString &sClientName, CString &sClientTime, CString &sErrorMessage)
{
	CUtils util;
	CString sSQL;
	sClientTime = _T("");
	sClientName = _T( "");
	bCurrentPlanLock = PLANLOCK_UNKNOWN;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);

	TCHAR buf[260];
	DWORD len = 260;
	::GetComputerName(buf, &len);
	sSQL.Format("{CALL spPublicationLock (%d, '%s', %d,'INPUTCENTER %s')}", nPublicationID, TranslateDate(tPubDate), bRequestedPlanLock, buf);
	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			CString s;
			Rs.GetFieldValue((short)0, s);
			bCurrentPlanLock = atoi(s);
			Rs.GetFieldValue((short)1, sClientName);

			CDBVariant DBvar;
			Rs.GetFieldValue((short)2, DBvar, SQL_C_TIMESTAMP);
			sClientTime.Format("%.4d-%.2d-%.2d %.2d:%.2d:%.2d", DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		e->GetErrorMessage(buf, 260);
		e->Delete();
		Rs.Close();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	if (bRequestedPlanLock == PLANLOCK_LOCK && bCurrentPlanLock == PLANLOCK_LOCK)
		util.Logprintf("INFO:  Publication lock applied on %.2d-%.2d-%.4d %s", tPubDate.GetDay(), tPubDate.GetMonth(), tPubDate.GetYear(), g_prefs.GetPublicationNameNoReload(nPublicationID));
	else if (bRequestedPlanLock == PLANLOCK_LOCK && bCurrentPlanLock != PLANLOCK_LOCK)
		util.Logprintf("WARNING: Publication lock on %.2d-%.2d-%.4d %s rejected - lock in use by %s", tPubDate.GetDay(), tPubDate.GetMonth(), tPubDate.GetYear(), g_prefs.GetPublicationNameNoReload(nPublicationID), sClientName);
	else if (bRequestedPlanLock == PLANLOCK_UNLOCK)
		util.Logprintf("INFO:  Publication unlock on %.2d-%.2d-%.4d %s", tPubDate.GetDay(), tPubDate.GetMonth(), tPubDate.GetYear(), g_prefs.GetPublicationNameNoReload(nPublicationID));
	// I fwe successfully locked on pubdate - try locking on pubdate2 also..

	if ((bRequestedPlanLock == PLANLOCK_LOCK && tPubDate2.GetYear() > 2000 && bCurrentPlanLock == PLANLOCK_LOCK) ||
		(bRequestedPlanLock == PLANLOCK_UNLOCK && tPubDate2.GetYear() > 2000)) {
		sSQL.Format("{CALL spPublicationLock (%d, '%s', %d,'INPUTCENTER %s')}", nPublicationID, TranslateDate(tPubDate2), bRequestedPlanLock, buf);
		try {
			if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
				return FALSE;
			}

			if (!Rs.IsEOF()) {
				CString s;
				Rs.GetFieldValue((short)0, s);
				bCurrentPlanLock = atoi(s);
				Rs.GetFieldValue((short)1, sClientName);


				CDBVariant DBvar;
				Rs.GetFieldValue((short)2, DBvar, SQL_C_TIMESTAMP);
				sClientTime.Format("%.4d-%.2d-%.2d %.2d:%.2d:%.2d", DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
			}
			Rs.Close();
		}
		catch (CDBException* e) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
			e->GetErrorMessage(buf, 260);
			e->Delete();
			Rs.Close();
			try {
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			return FALSE;
		}

		if (bRequestedPlanLock == PLANLOCK_LOCK && bCurrentPlanLock == PLANLOCK_LOCK)
			util.Logprintf("INFO:  Publication lock applied on %.2d-%.2d-%.4d %s", tPubDate2.GetDay(), tPubDate2.GetMonth(), tPubDate.GetYear(), g_prefs.GetPublicationNameNoReload(nPublicationID));
		else if (bRequestedPlanLock == PLANLOCK_LOCK && bCurrentPlanLock != PLANLOCK_LOCK)
			util.Logprintf("WARNING: Publication lock on %.2d-%.2d-%.4d %s rejected - lock in use by %s", tPubDate2.GetDay(), tPubDate.GetYear(), tPubDate2.GetMonth(), g_prefs.GetPublicationNameNoReload(nPublicationID), sClientName);
		else if (bRequestedPlanLock == PLANLOCK_UNLOCK)
			util.Logprintf("INFO:  Publication unlock on %.2d-%.2d-%.4d %s", tPubDate2.GetDay(), tPubDate2.GetMonth(), tPubDate.GetYear(), g_prefs.GetPublicationNameNoReload(nPublicationID));



	}

	return TRUE;
}


BOOL CDatabaseManager::ResetPollStatus(CString &sErrorMessage)
{
	CString sSQL;
	sErrorMessage = _T("");

	sSQL.Format("UPDATE PageTable SET Status=0 WHERE (Status=5 OR Status=6) AND (InputProcessID=%d OR InputProcessID=0)", g_prefs.m_instancenumber);

	int nRetries = 3; //g_prefs.m_nQueryRetries;
	BOOL bSuccess = FALSE;
	while (!bSuccess && nRetries-- > 0) {
		if (InitDB(sErrorMessage) == FALSE) {
			Sleep(g_prefs.m_QueryBackoffTime);
			continue;
		}

		try {
			m_pDB->ExecuteSQL(sSQL);
			bSuccess = TRUE;
		}
		catch (CDBException* e) {
			sErrorMessage.Format(_T("Update failed - %s"), e->m_strError);
			LogprintfDB(_T("Non-fatal Error : Retry %d - %s   (%s)"), nRetries, e->m_strError, sSQL);
			try {
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
		}
	}

	return bSuccess;
}


BOOL CDatabaseManager::DecreaseVersion(int nMasterCopySeparationSet, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");
	BOOL bSuccess = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;

	while (!bSuccess && nRetries-- > 0) {

		if (InitDB(sErrorMessage) == FALSE) {
			Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
			continue;
		}

		sSQL.Format("UPDATE PageTable SET Version=Version-1 WHERE MasterCopySeparationSet=%d AND Version > 0", nMasterCopySeparationSet);
		try {
			m_pDB->ExecuteSQL(sSQL);
			bSuccess = TRUE;
			break;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();
			try {
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
		}
	}

	return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////
// Input thread query functions
//
// Returns copyseparationset > 0 if sep.-set in system (any colors)
// Device is locked to same device as other colors
//
// Returns version >= 0 if color in system too.
// Returns SeparationSet if color not in system
//
// Assumes connection already opened!
//
////////////////////////////////////////////////////////////////////////////////

int CDatabaseManager::LookupInputJob(CJobAttributes *pJob, INPUTPROCESSSTRUCT *ip, CString sFileName, BOOL bAllowUnplannedSubEdition, CString &sErrorMessage)
{
	pJob->m_copyseparationset = 0;
	pJob->m_version = -1;

	int nInputMode = 0;
	sErrorMessage = _T( "");

	CString sSQL, s;

	CRecordset *Rs = NULL;

	BOOL bMayAddColor = ip->m_mayaddcolor || ip->m_mayaddpage;

	sSQL.Format("{CALL spInputLookupNextInputJob (%d,'%s',%d,%d,%d,%d,%d,'%s',%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,'%s',%d,%d,%d,%d,%d,%d,%d,%d)}",
		pJob->m_publicationID,
		TranslateDate(pJob->m_pubdate),
		pJob->m_pressrunID,
		pJob->m_editionID,
		pJob->m_sectionID,
		pJob->m_locationID,
		pJob->m_status,
		pJob->m_pagename,
		pJob->m_layer,
		5, //g_prefs.GetColorID(pJob->m_colorname),
		pJob->m_guesssection,	//1
		pJob->m_guessedition,	//2
		pJob->m_guessrun,		//3
		pJob->m_guesslocation,	//4
		0,		//5
		bMayAddColor,		//6
		0,	//7
		ip->m_putonhold,
		ip->m_approvalrequired,
		FALSE,
		ip->m_mayaddpage,
		ip->m_defaultcopies,
		ip->m_defaulttemplateID,
		ip->m_defaultprooftemplateID,
		50,
		sFileName,
		pJob->m_pagination > 0 ? pJob->m_pagination : -1 * pJob->m_pageindex,
		bAllowUnplannedSubEdition,
		pJob->m_weekreference,
		0,
		ip->m_ignoreupdatesonalreadyapproved,
		ip->m_allowtimededitions,
		ip->m_inputtype,
		ip->m_ID);


	BOOL bSuccess = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;

	CUtils util;
	g_util.Logprintf("INFO:  Input query:%s", sSQL);
	while (!bSuccess && nRetries-- > 0) {

		if (InitDB(sErrorMessage) == FALSE) {
			Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
			continue;
		}

		Rs = new CRecordset(m_pDB);

		try {
			if (!Rs->Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
				Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				delete Rs;
				Rs = NULL;
				continue;
			}

			int nFields = Rs->GetODBCFieldCount();

			if (!Rs->IsEOF()) {

				CString s;
				int f = 0;
				Rs->GetFieldValue((short)f++, s);
				nInputMode = atoi(s);
				util.Logprintf("INFO:  Input query return code: %d", nInputMode);
				if (nInputMode == POLLDBSTATUS_GOT_TIMED_EDITION)
				{
					Rs->GetFieldValue((short)f++, s);
					pJob->m_editionID = atoi(s);
				}
				else  if (nInputMode > 0 && nInputMode < POLLDBSTATUS_PAGEINPROGRESS && nInputMode != POLLDBSTATUS_NEWPAGE && nInputMode != POLLDBSTATUS_ILLEGALCOLORPAGE) {
					Rs->GetFieldValue((short)f++, s);
					pJob->m_mastercopyseparationset = atoi(s);
					Rs->GetFieldValue((short)f++, s);
					pJob->m_copyseparationset = atoi(s);

					Rs->GetFieldValue((short)f++, s);
					pJob->m_sectionID = atoi(s);

					Rs->GetFieldValue((short)f++, s);
					pJob->m_editionID = atoi(s);

					Rs->GetFieldValue((short)f++, s);
					pJob->m_pressrunID = atoi(s);

					CDBVariant DBvar;
					Rs->GetFieldValue((short)f++, DBvar, SQL_C_TIMESTAMP);
					CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
					pJob->m_pubdate = t;

					Rs->GetFieldValue((short)f++, s);
					pJob->m_pagename = s;

					Rs->GetFieldValue((short)f++, s);
					pJob->m_locationID = atoi(s);

					Rs->GetFieldValue((short)f++, s);
					pJob->m_currentmarkgroup = s;

					Rs->GetFieldValue((short)f++, s);
					pJob->m_approved = atoi(s);

					if (nFields > f) {
						Rs->GetFieldValue((short)f++, s);
						pJob->m_previousstatus = atoi(s) & 0xff;
						pJob->m_previousactive = atoi(s) >> 8;
					}
					if (nFields > f) {
						Rs->GetFieldValue((short)f++, s);
						pJob->m_pagetype = atoi(s);
					}
					if (nFields > f) {
						Rs->GetFieldValue((short)f++, s);
						pJob->m_pagination = atoi(s);
					}
					if (nFields > f) {
						Rs->GetFieldValue((short)f++, s);
						pJob->m_pageindex = atoi(s);
					}
					if (nFields > f) {
						Rs->GetFieldValue((short)f++, s);
						pJob->m_version = atoi(s);			// 0 for fisrt file!
					}

					pJob->m_originalmastercopyseparationset = 0;

					if (nFields > f) {
						Rs->GetFieldValue((short)f++, s);
						pJob->m_originalmastercopyseparationset = atoi(s);
					}

				}
			}
			Rs->Close();
			bSuccess = TRUE;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);

			e->Delete();
			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
		}
	}
	if (Rs != NULL)
		delete Rs;
	return bSuccess ? nInputMode : -1;
}

int CDatabaseManager::InsertUnplannedPage(CJobAttributes *pJob, BOOL bMustApplyProduction, BOOL bStatusMissing, BOOL bForceVersion0, CString &sErrorMessage)
{
	sErrorMessage = _T("");

	int nStatus = 30; //g_prefs.m_LocationList.GetCount() == 1 ? 30 : 20;


	CString sSQL, s;

	CRecordset *Rs = NULL;

		sSQL.Format("{CALL spInputInsertNewPage ('%s', %d,%d,%d,%d,%d,  '%s',%d,%d,%d,%d,  %d,%d,%d,'%s',%d ,%d, %d,%d, 0, 0,%d,%d,'%s',%d,'%s','%s')}",
			TranslateDate(pJob->m_pubdate),

			pJob->m_publicationID,
			pJob->m_pressrunID,
			pJob->m_editionID,
			pJob->m_sectionID,
			pJob->m_locationID,

			pJob->m_pagename,
			5, //g_prefs.GetColorID(pJob->m_colorname),
			pJob->m_copies,
			pJob->m_approved,	// Actually Approval required from pub default
			pJob->m_hold,

			pJob->m_templateID,
			pJob->m_prooftemplateID,
			pJob->m_priority,
			pJob->m_filename,
			pJob->m_pagination,
			g_prefs.m_instancenumber,
			pJob->m_mastercopyseparationset,
			pJob->m_priority,
			pJob->m_weekreference,
			bMustApplyProduction,
			pJob->m_markgroup,
			bStatusMissing ? STATUSNUMBER_MISSING : nStatus,
			TranslateDate(pJob->m_deadline),
			pJob->m_miscstring3);
	BOOL bSuccess = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;
	BOOL bInsertRestricted = FALSE;

	CUtils util;
	util.Logprintf("INFO:  Input query:%s", sSQL);

	while (!bSuccess && nRetries-- > 0) {

		try {
			if (InitDB(sErrorMessage) == FALSE) {
				Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				continue;
			}

			Rs = new CRecordset(m_pDB);

			if (!Rs->Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
				Sleep(g_prefs.m_QueryBackoffTime);
				delete Rs;
				Rs = NULL;
				continue;
			}

			if (!Rs->IsEOF()) {
				Rs->GetFieldValue((short)0, s);
				if (atoi(s) < 0)
					bInsertRestricted = TRUE;
				pJob->m_copyseparationset = atoi(s);
				Rs->GetFieldValue((short)1, s);
				pJob->m_mastercopyseparationset = atoi(s);

			}
			Rs->Close();
			bSuccess = TRUE;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();
			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
		}
	}


	if (Rs != NULL)
		delete Rs;

	if (bSuccess && bForceVersion0) {
		sSQL.Format("UPDATE PageTable SET Version=0 WHERE MasterCopySeparationSet=%d", pJob->m_mastercopyseparationset);

		bSuccess = FALSE;
		nRetries = g_prefs.m_nQueryRetries;

		while (!bSuccess && nRetries-- > 0) {
			if (InitDB(sErrorMessage) == FALSE) {
				Sleep(g_prefs.m_QueryBackoffTime);
				continue;
			}

			try {
				m_pDB->ExecuteSQL(sSQL);
				bSuccess = TRUE;
				break;
			}

			catch (CDBException* e) {
				sErrorMessage.Format("DB Update failed - %s", e->m_strError);
				LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
				e->Delete();

				try {
					m_DBopen = FALSE;
					m_pDB->Close();
				}
				catch (CDBException* e) {
					// So what..! Go on!;
				}
				::Sleep(g_prefs.m_QueryBackoffTime);
			}
		}
	}

	return bInsertRestricted ? 2 : bSuccess;
}

int CDatabaseManager::CreateSubEditionPlate(CJobAttributes *pJob, BOOL bForceHold, BOOL bUseDefaultApproval, BOOL bFreeDevice, CString &sErrorMessage)
{
	CString sSQL, s;
	CUtils util;
	sErrorMessage = _T("");

	int nCopySeparationSet = 0;

	BOOL bSuccess = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;

	sSQL.Format("{CALL spInputCreateNewPlateSet (%d,'%s',%d,%d,'%s',%d,%d,%d,%d,%d,%d) }",
		pJob->m_publicationID,
		TranslateDate(pJob->m_pubdate),
		pJob->m_editionID,
		pJob->m_sectionID,
		pJob->m_pagename,
		5, //g_prefs.GetColorID(pJob->m_colorname),
		pJob->m_locationID,
		bForceHold,
		bUseDefaultApproval,
		bFreeDevice,
		pJob->m_pagination);


	CRecordset *Rs = NULL;

	util.Logprintf("INFO: Querying %s", sSQL);
	while (!bSuccess && nRetries-- > 0) {

		try {
			if (InitDB(sErrorMessage) == FALSE) {
				Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				continue;
			}

			Rs = new CRecordset(m_pDB);

			if (!Rs->Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
				Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				delete Rs;
				Rs = NULL;
				continue;
			}

			if (!Rs->IsEOF()) {

				Rs->GetFieldValue((short)0, s);
				nCopySeparationSet = atoi(s);

			}
			Rs->Close();
			bSuccess = TRUE;
			break;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();
			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
		}
	}

	if (Rs != NULL)
		delete Rs;
	return bSuccess ? nCopySeparationSet : -1;
}

// Update Input Status using ColorSet or CopyColorSet (for input/proof threads uses CopyColorSet)
// Assumes connection already opened!
int CDatabaseManager::UpdateStatusInput(int nInputID, int nMasterCopySeparationSet, int nPDFtype, int nStatus, CString sErr,
														CString sFileName, CString sComment,  BOOL bReapproveUpdates, CString &sErrorMessage)
{
	CString sSQL, s;
	CUtils util;

	sErrorMessage = _T("");
	BOOL bSuccess = FALSE;
	BOOL bStatusSet = FALSE;

	int nRetries = g_prefs.m_nQueryRetries;

	CString sFileServer = g_prefs.m_serverName;

	int nID = g_prefs.m_instancenumber + (nInputID << 8);

	sSQL.Format("{CALL spInputUpdateStatus (%d,%d,%d,'%s','%s','%s','%s','%s','%s',%d,%d)}", nMasterCopySeparationSet, nPDFtype, nStatus, sErr, sFileName, _T(""), sComment, _T(""), sFileServer, nID, bReapproveUpdates);

	util.Logprintf("INFO:  Input query:%s", sSQL);

	CRecordset *Rs = NULL;

	while (!bSuccess && !bStatusSet && nRetries-- > 0) {

		try {
			if (InitDB(sErrorMessage) == FALSE) {
				Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				continue;
			}

			if (Rs == NULL)
				Rs = new CRecordset(m_pDB);

			if (!Rs->Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
				Sleep(g_prefs.m_QueryBackoffTime);
				delete Rs;
				Rs = NULL;
				continue;
			}

			if (!Rs->IsEOF()) {
				Rs->GetFieldValue((short)0, s);
				bStatusSet = atoi(s);
			}
			else
				bStatusSet = TRUE; //Backward comp.

			util.Logprintf("INFO:  UpdateStatusInput() OK");

			Rs->Close();
			bSuccess = TRUE;
		}

		catch (CDBException* e) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();
			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			::Sleep(g_prefs.m_QueryBackoffTime);
		}
	}


	if (Rs != NULL)
		delete Rs;

	return bStatusSet && bSuccess ? 1 : (bSuccess ? 0 : -1);

}




BOOL CDatabaseManager::CallCustomSP(int nMasterCopySeparationSet, int nCopySeparationSet, int nCustomFlag, CJobAttributes *pJob, CString &sErrorMessage)
{
	CString sSQL;

	int nFutureOption2 = 0;
	CString sFutureOptionString1 = _T("");
	CString sFutureOptionString2 = _T("");

	if (pJob->m_locationgroupIDList != "")
		sFutureOptionString1 = pJob->m_locationgroupIDList;

	sErrorMessage = _T("");

	BOOL bSuccess = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;

	CUtils util;
	sSQL.Format("{CALL spInputCustom (%d,%d,%d,'%s',%d,%d,%d,'%s', %d,%d,%d,%d,%d,'%s','%s')}",
		nMasterCopySeparationSet,
		nCopySeparationSet,
		pJob->m_publicationID,
		TranslateDate(pJob->m_pubdate),
		pJob->m_pressrunID,
		pJob->m_editionID,
		pJob->m_sectionID,
		pJob->m_pagename,
		_T("PDF"), //g_prefs.GetColorID(pJob->m_colorname),
		pJob->m_locationID,
		pJob->m_pressID,
		nCustomFlag,
		nFutureOption2,
		sFutureOptionString1,
		sFutureOptionString2

	);

	g_util.Logprintf("INFO: Calling %s", sSQL);

	while (!bSuccess && nRetries-- > 0) {

		if (InitDB(sErrorMessage) == FALSE) {
			Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
			continue;
		}


		try {
			m_pDB->ExecuteSQL(sSQL);
			bSuccess = TRUE;
			break;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();
			try {
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			::Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
		}
	}

	return bSuccess;
}


BOOL CDatabaseManager::UpdateCRC(int nMasterCopySeparationSet, int nPDFtype, int nCRC, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");

	BOOL bSuccess = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;

	if (nPDFtype == POLLINPUTTYPE_LOWRESPDF)
		sSQL.Format("UPDATE PageTable SET CheckSumPdfLowres=%d WHERE MasterCopySeparationSet=%d", nCRC, nMasterCopySeparationSet);
	else if (nPDFtype == POLLINPUTTYPE_PRINTPDF)
		sSQL.Format("UPDATE PageTable SET CheckSumPdfPrint=%d WHERE MasterCopySeparationSet=%d", nCRC, nMasterCopySeparationSet);
	else // if (nPDFtype == POLLINPUTTYPE_HIRESPDF)
		sSQL.Format("UPDATE PageTable SET CheckSumPdfHighRes=%d WHERE MasterCopySeparationSet=%d", nCRC, nMasterCopySeparationSet);


	while (!bSuccess && nRetries-- > 0) {

		if (InitDB(sErrorMessage) == FALSE) {
			Sleep(g_prefs.m_QueryBackoffTime);
			continue;
		}
		try {
			m_pDB->ExecuteSQL(sSQL);
			bSuccess = TRUE;
			break;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("Update failed - %s", e->m_strError);
			LogprintfDB("Error - try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			try {
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
		}
	}

	return bSuccess;

}

BOOL CDatabaseManager::UpdateReadyTime(int nMasterCopySeparationSet, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");
	BOOL bSuccess = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;

	while (!bSuccess && nRetries-- > 0) {

		if (InitDB(sErrorMessage) == FALSE) {
			Sleep(g_prefs.m_QueryBackoffTime);
			continue;
		}
		sSQL.Format("{CALL spInputSetReadyTime (%d,5)}", nMasterCopySeparationSet);
		try {
			m_pDB->ExecuteSQL(sSQL);
			bSuccess = TRUE;
			break;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();
			try {
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
		}
	}
	return bSuccess;
}

BOOL CDatabaseManager::UpdateMiscString2(int nMasterCopySeparationSet, CString sMiscString2, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");
	BOOL bSuccess = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;

	while (!bSuccess && nRetries-- > 0) {

		if (InitDB(sErrorMessage) == FALSE) {
			Sleep(g_prefs.m_QueryBackoffTime);
			continue;
		}
		sSQL.Format("UPDATE PageTable SET MiscString2='%s' WHERE MasterCopySeparationSet=%d", sMiscString2, nMasterCopySeparationSet);
		try {
			m_pDB->ExecuteSQL(sSQL);
			bSuccess = TRUE;
			break;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();
			try {
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
		}
	}
	return bSuccess;
}


BOOL CDatabaseManager::UpdateMiscString3(int nMasterCopySeparationSet, CString sMiscString3, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");
	BOOL bSuccess = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;

	while (!bSuccess && nRetries-- > 0) {

		if (InitDB(sErrorMessage) == FALSE) {
			Sleep(g_prefs.m_QueryBackoffTime);
			continue;
		}
		sSQL.Format("UPDATE PageTable SET MiscString3='%s' WHERE MasterCopySeparationSet=%d", sMiscString3, nMasterCopySeparationSet);
		try {
			m_pDB->ExecuteSQL(sSQL);
			bSuccess = TRUE;
			break;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();
			try {
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
		}
	}
	return bSuccess;
}

BOOL CDatabaseManager::UpdateVersion(int nMasterCopySeparationSet,  int nVersion, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");
	BOOL bSuccess = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;

	while (!bSuccess && nRetries-- > 0) {

		if (InitDB(sErrorMessage) == FALSE) {
			Sleep(g_prefs.m_QueryBackoffTime);
			continue;
		}
		sSQL.Format("UPDATE PageTable SET Version=%d WHERE MasterCopySeparationSet=%d", nVersion, nMasterCopySeparationSet);
		try {
			m_pDB->ExecuteSQL(sSQL);
			bSuccess = TRUE;
			break;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", e->m_strError);

			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();
			try {
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
		}
	}
	return bSuccess;
}

BOOL CDatabaseManager::UpdateInputTime(int nMasterCopySeparationSet,  CTime tInputTime, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");
	BOOL bSuccess = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;

	while (!bSuccess && nRetries-- > 0) {

		if (InitDB(sErrorMessage) == FALSE) {
			Sleep(g_prefs.m_QueryBackoffTime);
			continue;
		}
		sSQL.Format("UPDATE PageTable SET InputTime='%s' WHERE MasterCopySeparationSet=%d", TranslateTime(tInputTime), nMasterCopySeparationSet);
		try {
			m_pDB->ExecuteSQL(sSQL);
			bSuccess = TRUE;
			break;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();
			try {
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
		}
	}
	return bSuccess;
}

BOOL CDatabaseManager::UpdateApproval(int nMasterCopySeparationSet, int nApproval, CString sUserName, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");

	BOOL bSuccess = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;

	while (!bSuccess && nRetries-- > 0) {

		if (InitDB(sErrorMessage) == FALSE) {
			Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
			continue;
		}

		sSQL.Format("{CALL spUpdateApproval (%d,%d,'%s')}", nMasterCopySeparationSet, nApproval, sUserName);
		try {
			m_pDB->ExecuteSQL(sSQL);
			bSuccess = TRUE;
			break;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();
			try {
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
		}
	}

	return bSuccess;
}

BOOL CDatabaseManager::UpdateRelease(int nMasterCopySeparationSet, BOOL bHold, CString &sErrorMessage)
{
	CString sSQL;

	sErrorMessage = _T("");

	BOOL bSuccess = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;

	while (!bSuccess && nRetries-- > 0) {

		if (InitDB(sErrorMessage) == FALSE) {
			Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
			continue;
		}

		sSQL.Format("{CALL spUpdateHold (%d,%d,%d)}", nMasterCopySeparationSet, 5, bHold);
		try {
			m_pDB->ExecuteSQL(sSQL);
			bSuccess = TRUE;
			break;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Update failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();
			try {
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
		}
	}

	return bSuccess;
}

BOOL CDatabaseManager::GetJobDetails(CJobAttributes *pJob, int nCopySeparationSet,  CString &sErrorMessage)
{
	CUtils util;
	CString sSQL, s;

	sSQL.Format("SELECT TOP 1 PublicationID,PubDate,EditionID,SectionID,PageName,Pagination,PageIndex,ColorID,Version,Comment,MiscString1,MiscString2,MiscString3,FileName,LocationID,ProductionID FROM PageTable WITH (NOLOCK) WHERE CopySeparationSet=%d AND Dirty=0 ORDER BY PageIndex", nCopySeparationSet);


	BOOL bSuccess = FALSE;
	CRecordset *Rs = NULL;

	pJob->m_copyflatseparationset = nCopySeparationSet;

	sErrorMessage = _T("");
	int nRetries = g_prefs.m_nQueryRetries;

	while (bSuccess == FALSE && nRetries-- > 0) {
		try {
			if (InitDB(sErrorMessage) == FALSE) {
				Sleep(g_prefs.m_QueryBackoffTime); // Time between retries
				continue;
			}

			Rs = new CRecordset(m_pDB);

			if (!Rs->Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
				sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
				Sleep(g_prefs.m_QueryBackoffTime);
				delete Rs;
				Rs = NULL;
				continue;
			}

			int nFields = Rs->GetODBCFieldCount();

			if (!Rs->IsEOF()) {
				int fld = 0;
				Rs->GetFieldValue((short)fld++, s);
				pJob->m_publicationID = atoi(s);


				CDBVariant DBvar;
				try {
					Rs->GetFieldValue((short)fld++, DBvar, SQL_C_TIMESTAMP);
					CTime t(DBvar.m_pdate->year, DBvar.m_pdate->month, DBvar.m_pdate->day, DBvar.m_pdate->hour, DBvar.m_pdate->minute, DBvar.m_pdate->second);
					pJob->m_pubdate = t;
				}
				catch (CException *ex)
				{
				}

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_editionID = atoi(s);

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_sectionID = atoi(s);

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_pagename = s;

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_pagination = atoi(s);

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_pageindex = atoi(s);

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_colorname = _T("PDF");		// HARDCODED

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_version = atoi(s);


				Rs->GetFieldValue((short)fld++, s);
				pJob->m_comment = s;

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_miscstring1 = s;

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_miscstring2 = s;

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_miscstring3 = s;

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_filename = s;

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_locationID = atoi(s);

				Rs->GetFieldValue((short)fld++, s);
				pJob->m_productionID = atoi(s);
			}
			Rs->Close();
			bSuccess = TRUE;
		}
		catch (CDBException* e) {
			sErrorMessage.Format("Query failed - %s", e->m_strError);
			LogprintfDB("Error try %d : %s   (%s)", nRetries, e->m_strError, sSQL);
			e->Delete();

			try {
				if (Rs->IsOpen())
					Rs->Close();
				delete Rs;
				Rs = NULL;
				m_DBopen = FALSE;
				m_pDB->Close();
			}
			catch (CDBException* e) {
				// So what..! Go on!;
			}
			Sleep(g_prefs.m_QueryBackoffTime);
		}
	}

	if (Rs != NULL)
		delete Rs;

	return bSuccess;
}

int CDatabaseManager::GetChannelCRC(int nMasterCopySeparationSet, int nInputMode, CString &sErrorMessage)
{
	sErrorMessage =  _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;

	CRecordset Rs(m_pDB);
	CString sSQL, s;

	int nCRC = 0;

	sSQL.Format("SELECT TOP 1 CheckSumPdfPrint FROM PageTable WITH (NOLOCK) WHERE MasterCopySeparationSet=%d", nMasterCopySeparationSet);
	
	if (nInputMode == POLLINPUTTYPE_HIRESPDF)
		sSQL.Format("SELECT TOP 1 CheckSumPdfHighRes FROM PageTable WITH (NOLOCK) WHERE MasterCopySeparationSet=%d", nMasterCopySeparationSet);
	else if (nInputMode == POLLINPUTTYPE_LOWRESPDF)
		sSQL.Format("SELECT TOP 1 CheckSumPdfLowRes FROM PageTable WITH (NOLOCK) WHERE MasterCopySeparationSet=%d", nMasterCopySeparationSet);

	

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return -1;
		}

		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			nCRC = atoi(s);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return -1;
	}

	return nCRC;
}

int CDatabaseManager::GetChannelStatus(int nMasterCopySeparationSet, int nChannelID, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;

	CRecordset Rs(m_pDB);
	CString sSQL, s;

	int nStatus = -1;


	sSQL.Format("SELECT TOP 1 Status FROM ChannelStatus WITH (NOLOCK) WHERE MasterCopySeparationSet=%d AND ChannelID=%d", nMasterCopySeparationSet, nChannelID);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return -1;
		}

		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			nStatus = atoi(s);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return -1;
	}

	return nStatus;
}

BOOL CDatabaseManager::GetInputEnabledUpdate(INPUTPROCESSSTRUCT *pInput, CString &sErrorMessage)
{
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	CRecordset Rs(m_pDB);
	CString sSQL, s;

	sSQL.Format("SELECT TOP 1 [Enabled] FROM InputConfigurations WITH (NOLOCK) WHERE InputID=%d", pInput->m_ID);

	try {
		if (!Rs.Open(CRecordset::snapshot | CRecordset::forwardOnly, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", sSQL);
			return FALSE;
		}

		if (!Rs.IsEOF()) {
			Rs.GetFieldValue((short)0, s);
			pInput->m_enabled = atoi(s);
		}
		Rs.Close();
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", e->m_strError);
		LogprintfDB("Error : %s   (%s)", e->m_strError, sSQL);
		e->Delete();
		Rs.Close();

		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}

BOOL CDatabaseManager::UpdateChannelStatus(int nMasterCopySeparationSet, int nProductionID, int nChannelID, int nStatus, CString sMessage, CString sFileName, CString &sErrorMessage)
{
	sErrorMessage = _T("");

	BOOL ret = FALSE;
	int nRetries = g_prefs.m_nQueryRetries;

	while (ret == FALSE && nRetries-- > 0) {

		ret = UpdateChannelStatus2(nMasterCopySeparationSet, nProductionID, nChannelID, nStatus, sMessage, sFileName, sErrorMessage);
		if (ret == FALSE)
			Sleep(g_prefs.m_QueryBackoffTime);
	}
	return ret;
}

BOOL CDatabaseManager::UpdateChannelStatus2(int nMasterCopySeparationSet, int nProductionID, int nChannelID, int nStatus, CString sMessage, CString sFileName, CString &sErrorMessage)
{

	CString sSQL;

	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return FALSE;

	sSQL.Format("{ CALL spUpdateChannelStatus (%d,%d,%d,%d,'%s','%s') }", nMasterCopySeparationSet, nProductionID, nChannelID, nStatus, sMessage, sFileName);

	try {
		m_pDB->ExecuteSQL(sSQL);
	}
	catch (CDBException* e) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Insert/Update failed - %s", e->m_strError);
		LogprintfDB("Error: %s   (%s)", e->m_strError, sSQL);

		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch (CDBException* e) {
			// So what..! Go on!;
		}
		return FALSE;
	}

	return TRUE;
}




int CDatabaseManager::FieldExists(CString sTableName, CString sFieldName , CString &sErrorMessage)
{
	CString sSQL, s;
	BOOL bFound = FALSE;
	sErrorMessage = _T("");

	if (InitDB(sErrorMessage) == FALSE)
		return -1;
	
	CRecordset Rs(m_pDB);
	sSQL.Format("{ CALL sp_columns ('%s') }", (LPCSTR)sTableName);

	try {		
		if(!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", (LPCSTR)sSQL);
			return FALSE;
		}
		while (!Rs.IsEOF()) {
			
			Rs.GetFieldValue((short)3, s);

			if (s.CompareNoCase(sFieldName) == 0) {
				bFound = TRUE;
				break;
			}
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch( CDBException* e ) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch( CDBException* e ) {
			// So what..! Go on!;
		}
		return -1;
	}

	return bFound ? 1 : 0;
}

int CDatabaseManager::TableExists(CString sTableName, CString &sErrorMessage)
{
	CString sSQL, s;
	BOOL bFound = FALSE;
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;
	
	CRecordset Rs(m_pDB);
	sSQL.Format("{ CALL sp_tables ('%s') }", (LPCSTR)sTableName);

	try {		
		if(!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", (LPCSTR)sSQL);
			return FALSE;
		}
		if (!Rs.IsEOF()) {
			
			bFound = TRUE;
		}
		Rs.Close();
	}
	catch( CDBException* e ) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch( CDBException* e ) {
			// So what..! Go on!;
		}
		return -1;
	}

	return bFound ? 1 : 0;
}

int CDatabaseManager::StoredProcParameterExists(CString sSPname, CString sParameterName, CString &sErrorMessage)
{
	CString sSQL, s;
	BOOL bFound = FALSE;
	sErrorMessage = _T("");
	if (InitDB(sErrorMessage) == FALSE)
		return -1;
	
	CRecordset Rs(m_pDB);
	sSQL.Format("{ CALL sp_sproc_columns ('%s') }", (LPCSTR)sSPname);

	try {		
		if(!Rs.Open(CRecordset::snapshot, sSQL, CRecordset::readOnly)) {
			sErrorMessage.Format("Query failed - %s", (LPCSTR)sSQL);
			return FALSE;
		}
		while (!Rs.IsEOF()) {
			
			Rs.GetFieldValue((short)3, s);

			if (s.CompareNoCase(sParameterName) == 0) {
				bFound = TRUE;
				break;
			}
			Rs.MoveNext();
		}
		Rs.Close();
	}
	catch( CDBException* e ) {
		sErrorMessage.Format("ERROR (DATABASEMGR): Query failed - %s", (LPCSTR)e->m_strError);
		e->Delete();
		try {
			m_DBopen = FALSE;
			m_pDB->Close();
		}
		catch( CDBException* e ) {
			// So what..! Go on!;
		}
		return -1;
	}

	return bFound ? 1 : 0;
}

/////////////////////////////////////////////////
// Translate CTime to SQL Server DATETIME type
/////////////////////////////////////////////////

CString CDatabaseManager::TranslateDate(CTime tDate)
{
	CString dd,mm,yy,yysmall;

	dd.Format("%.2d",tDate.GetDay());
	mm.Format("%.2d",tDate.GetMonth());
	yy.Format("%.4d",tDate.GetYear());
	yysmall.Format("%.2d",tDate.GetYear());

	return mm + "/" + dd + "/" + yy;

}

CString CDatabaseManager::TranslateTime(CTime tDate)
{
	CString dd,mm,yy,yysmall, hour, min, sec;

	dd.Format("%.2d",tDate.GetDay());
	mm.Format("%.2d",tDate.GetMonth());
	yy.Format("%.4d",tDate.GetYear());
	yysmall.Format("%.2d",tDate.GetYear());

	hour.Format("%.2d",tDate.GetHour());
	min.Format("%.2d",tDate.GetMinute());
	sec.Format("%.2d",tDate.GetSecond());

	return mm + "/" + dd + "/" + yy + " " + hour + ":" + min + ":" + sec;
}

void CDatabaseManager::LogprintfDB(const TCHAR *msg, ...)
{
	TCHAR	szLogLine[16000], szFinalLine[16000];
	va_list	ap;
	DWORD	n, nBytesWritten;
	SYSTEMTIME	ltime;


	HANDLE hFile = ::CreateFile(g_prefs.m_logFileFolder + _T("\\InputServiceDBerror.log"), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;

	// Seek to end of file
	::SetFilePointer(hFile, 0, NULL, FILE_END);

	va_start(ap, msg);
	n = vsprintf(szLogLine, msg, ap);
	va_end(ap);
	szLogLine[n] = '\0';

	::GetLocalTime(&ltime);
	_stprintf(szFinalLine, "[%.2d-%.2d %.2d:%.2d:%.2d.%.3d] %s\r\n", (int)ltime.wDay, (int)ltime.wMonth, (int)ltime.wHour, (int)ltime.wMinute, (int)ltime.wSecond, (int)ltime.wMilliseconds, szLogLine);

	::WriteFile(hFile, szFinalLine, (DWORD)_tcsclen(szFinalLine), &nBytesWritten, NULL);

	::CloseHandle(hFile);

#ifdef _DEBUG
	TRACE(szFinalLine);
#endif
}

