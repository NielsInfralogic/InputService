#include "stdafx.h"
#include <afxmt.h>
#include <afxtempl.h> 
#include <direct.h>
#include "Defs.h"
#include "DatabaseManager.h"
#include "PollThread.h"
#include "Utils.h"
#include "Prefs.h"
#include "FtpClient.h"
#include "SFtpClient.h"
#include "GoogleDriveClient.h"
#include "PDFutils.h"
#include "EmailClient.h"
#include "IMAPClient.h"

extern CPrefs g_prefs;
extern CUtils g_util;
extern BOOL g_isstarted;
extern BOOL g_BreakSignal;
extern BOOL g_pollthreadrunning;
BOOL g_breaktransfer = FALSE;;

CWinThread *g_pPollThread = NULL;

BOOL StartAllThreads()
{
	// Ini file IS loaded - startup all threads..
	CString sErrorMessage;

	// The thread loads database names etc..
	//g_pPollThread = AfxBeginThread(PollThread, (LPVOID)NULL);

	return TRUE;
}



//UINT PollThread(LPVOID param)
void WorkerLoop()
{
	CFTPClient ftp_in;
	CSFTPClient sftp_in;
	CGoogleDriveClient google_in;

	CJobAttributes Job;

	CDatabaseManager m_pollDB;
	CString sMsg, sErrorMessage = _T("");

	FILEINFOSTRUCT aFilesReady[MAXFILES];
	FILEINFOSTRUCT aFilesReadyFTP[MAXFILES];

	int	bDoBreak = FALSE;
	int bReConnect = FALSE;
	int	bFirstTime = TRUE;
	int	nFiles = 0;
	int nThisSpoolFolder = -1;

	ftp_in.InitSession();
	sftp_in.InitSession();
	ftp_in.SetHeartheatMS(g_prefs.m_heartbeatMS);
	sftp_in.SetHeartheatMS(g_prefs.m_heartbeatMS);

	::CreateDirectory(g_util.GetTempFolder() + _T("\\Scripted"), NULL);

	// First time db connection in thread..
	BOOL m_connected = m_pollDB.InitDB(g_prefs.m_DBserver, g_prefs.m_Database, g_prefs.m_DBuser, g_prefs.m_DBpassword,
		g_prefs.m_IntegratedSecurity, sErrorMessage);
	g_util.Logprintf("INFO: Reading db-preferences..");
	if (m_connected) {

		if (m_pollDB.LoadAllPrefs(sErrorMessage) == FALSE) {
			g_util.Logprintf("ERROR: LoadAllPrefs() returned - %s", sErrorMessage);
		}
		if (m_pollDB.RegisterService(sErrorMessage) == FALSE) {
			g_util.Logprintf("ERROR: RegisterService() returned - %s", sErrorMessage);
		}
	}
	else
		g_util.Logprintf("ERROR: InitDB() returned - %s", sErrorMessage);

	if (g_util.CheckFolder(g_prefs.m_hiresPDFPath) == FALSE) {
		g_util.Logprintf("ERROR: Pollthread cannot connect to %s", g_prefs.m_hiresPDFPath);

		m_pollDB.UpdateService(SERVICESTATUS_HASERROR, _T(""), _T("Storage folder connection error"), sErrorMessage);

		g_pollthreadrunning = FALSE;
		//return;
	}

	g_util.Logprintf("INFO: Number of input folders %d", g_prefs.m_InputProcessList.GetCount());

	// Create error folders.
	for (int i = 0; i < g_prefs.m_InputProcessList.GetCount(); i++) {
		INPUTPROCESSSTRUCT *pInput = &g_prefs.m_InputProcessList[i];
		CString sFullErrorPath;
		sFullErrorPath.Format("%s%d", g_prefs.m_errorPath, pInput->m_ID);
		if (g_util.CheckFolder(sFullErrorPath) == FALSE) {
			_mkdir((LPCTSTR)sFullErrorPath.GetBuffer(255));
			sFullErrorPath.ReleaseBuffer();
		}
	}


	BOOL bReconnect = FALSE;
	BOOL bDBReconnect = FALSE;

	DWORD dwLoopCount = 0;

	CString sLastError = _T("");
	CString sErrorPath;


	BOOL bRetryFolder = FALSE;
	BOOL bHaveRetriedFolder = FALSE;

	int nNumberOfSpoolers = (int)g_prefs.m_InputProcessList.GetCount();


	g_util.Logprintf("INFO: Pollthread entering loop..");

	do {
		if (g_BreakSignal)
			break;

		if (bDBReconnect) {

			for (int i = 0; i < 16; i++) {
				::Sleep(250);
				if (g_BreakSignal)
					break;
			}
			if (g_BreakSignal)
				break;

			bDBReconnect = FALSE;

			m_connected = m_pollDB.InitDB(g_prefs.m_DBserver, g_prefs.m_Database, g_prefs.m_DBuser, g_prefs.m_DBpassword,
				g_prefs.m_IntegratedSecurity, sErrorMessage);

			if (m_connected == FALSE) {

				g_util.Logprintf("ERROR: Database connection error");

				g_util.SendMail(MAILTYPE_DBERROR, sErrorMessage);
			}
		}

		if (bRetryFolder == FALSE) {
			bHaveRetriedFolder = FALSE;
			nThisSpoolFolder++;
		}
		else {
			bHaveRetriedFolder = TRUE;
			bRetryFolder = FALSE;
		}

		if (nThisSpoolFolder == nNumberOfSpoolers)
			nThisSpoolFolder = 0;

		INPUTPROCESSSTRUCT *pInput = &g_prefs.m_InputProcessList[nThisSpoolFolder];

		if (pInput == NULL)
			continue;

		if (m_pollDB.GetInputEnabledUpdate(pInput, sErrorMessage) == FALSE)
			g_util.Logprintf("ERROR: GetInputEnabledUpdate() %s", sErrorMessage);


		if (pInput->m_enabled == FALSE) {
			Sleep(100);
			continue;
		}

		if (g_BreakSignal)
			break;

		int nPollTime = pInput->m_polltime;
		if (nPollTime > 0) {

			for (int tm = 0; tm < 100; tm++) {

				::Sleep(10 * nPollTime);
				if (g_BreakSignal)
					break;
			}
		}
		else {

			Sleep(100);
		}

		if (g_BreakSignal)
			break;


		// Reset faulty 'polling' states every 10th minute...

		if (g_prefs.m_autoresetpollingstateinterval > 0) {
			if ((dwLoopCount) % g_prefs.m_autoresetpollingstateinterval == 0) {
				m_pollDB.ResetPollStatus(sErrorMessage);
			}
		}

		if ((dwLoopCount) % 5 == 0)
			m_pollDB.UpdateService(SERVICESTATUS_RUNNING, _T(""), _T(""), sErrorMessage);

		if (g_BreakSignal)
			break;


		// Check acccess to input folder..
		if (bFirstTime || bReConnect) {
			bFirstTime = FALSE;
			if (pInput->m_useFTP == NETWORKTYPE_FTP) {

				pInput->m_inputpath.Format("%s\\%d", g_prefs.m_workfolder, pInput->m_ID);
				::CreateDirectory(pInput->m_inputpath, NULL);
				CString sConnectFailReason = _T("");
				int bRet = ftp_in.OpenConnection(pInput->m_FTPserver, pInput->m_FTPusername, pInput->m_FTPpassword, pInput->m_ftppasv,
					pInput->m_FTPfolder, pInput->m_FTPport, g_prefs.m_ftptimeout, sErrorMessage, sConnectFailReason);

				CString sFtpFolder;
				if (bRet == FALSE) {
					CString s;
					sFtpFolder.Format(_T("ftp://%s:%d/%s"), pInput->m_FTPserver, pInput->m_FTPport, pInput->m_FTPfolder);

					g_util.Logprintf("ERROR: Unable to connect to FTP server %s - %s", sFtpFolder, sConnectFailReason);
					bReConnect = TRUE;

					ftp_in.Disconnect();

					m_pollDB.UpdateService(SERVICESTATUS_HASERROR, _T(""), _T("FTP connection error ") + sConnectFailReason, sErrorMessage);

					bReConnect = TRUE;
					continue;
				}
			}
			else if (pInput->m_useFTP == NETWORKTYPE_SHARE) {
				BOOL bInputPingOK = g_util.CheckFolderWithPing(pInput->m_inputpath);

				if (bInputPingOK == FALSE && g_prefs.m_firecommandonfolderreconnect && g_prefs.m_sFolderReconnectCommand != "") {
					g_util.DoCreateProcess(g_prefs.m_sFolderReconnectCommand);
					::Sleep(500);
					bInputPingOK = g_util.CheckFolderWithPing(pInput->m_inputpath);
				}
				if (bInputPingOK) {
					if (g_util.Reconnect(pInput->m_inputpath, pInput->m_usecurrentuser ? "" : pInput->m_username, pInput->m_usecurrentuser ? "" : pInput->m_password) == FALSE) {
						CString sConnectFailReason = g_util.GetLastWin32Error();
						g_util.Logprintf("ERROR: Unable to access folder (1) %s - %s", pInput->m_inputpath, sConnectFailReason);
						bReConnect = TRUE;
						m_pollDB.UpdateService(SERVICESTATUS_HASERROR, _T(""), _T("SMB connection error (reconnect) ") + sConnectFailReason, sErrorMessage);
						continue;
					}
				}
				else {
					CString s;
					g_util.Logprintf("ERROR: Unable to ping folder (2) %s", pInput->m_inputpath);
					bReConnect = TRUE;
					m_pollDB.UpdateService(SERVICESTATUS_HASERROR, _T(""), _T("SMB connection error (ping test)"), sErrorMessage);
					continue;

				}
			}
		}

		if (g_BreakSignal)
			break;

		if (g_prefs.m_minMBfreeCCDATA > 0) {
			DWORD dwFreeMB = g_prefs.m_minMBfreeCCDATA + 1;
			if (g_util.GetCCDATAFreeSpaceMB(dwFreeMB) == TRUE) {
				if ((int)dwFreeMB < g_prefs.m_minMBfreeCCDATA) {
					m_pollDB.UpdateService(SERVICESTATUS_HASERROR, _T(""), _T("DISK FULL - poll waiting.."), sErrorMessage);
					g_util.Logprintf("ERROR: Disk nearly full - Limit %d MB, Free %d MB", (int)g_prefs.m_minMBfreeCCDATA, (int)dwFreeMB);
					Sleep(500);
					continue;
				}
			}
		}

		if (pInput->m_useFTP == NETWORKTYPE_SHARE) {
			if (g_prefs.m_deletefileswithextension != "")
				g_util.DeleteFiles(pInput->m_inputpath, _T("*.") + g_prefs.m_deletefileswithextension);

			if (g_prefs.m_deletefilessearchmask != _T("")) {
				CStringArray aFilesToDelete;
				if (g_util.StringSplitter(g_prefs.m_deletefilessearchmask, ",", aFilesToDelete) > 0) {
					for (int splt = 0; splt < aFilesToDelete.GetCount(); splt++)
						g_util.DeleteFiles(pInput->m_inputpath, aFilesToDelete[splt].Trim());
				}
			}

			// Here is the actual folder scan for SMB
			nFiles = g_util.ScanDir(pInput->m_inputpath, pInput->m_searchmask,
				aFilesReady, MAXFILES, _T("*.tmp"), TRUE, FALSE);
			if (nFiles < 0) {
				g_util.Logprintf("ERROR: Share ScanDir() - %s ", sErrorMessage);
				CString sErrorMessage2 = sErrorMessage;
				m_pollDB.UpdateService(SERVICESTATUS_HASERROR, _T(""), _T("Input folder error ") + sErrorMessage2, sErrorMessage);

			}

		}
		else if (pInput->m_useFTP == NETWORKTYPE_FTP) {
			CStringArray sArr;
			int nFilesFTP = ftp_in.ScanDir(_T("*.*"), aFilesReadyFTP, MAXFILESPERTHREAD, _T("*.tmp"), sErrorMessage);
			if (nFilesFTP == -1) {
				bReConnect = TRUE;

				g_util.Logprintf("ERROR: FTP ScanDir() - %s", sErrorMessage);
				ftp_in.Disconnect();
				CString sErrorMessage2 = sErrorMessage;
				m_pollDB.UpdateService(SERVICESTATUS_HASERROR, _T(""), _T("FTP Error ") + sErrorMessage2, sErrorMessage);

				::Sleep(1000);
				continue;
			}
			// Get the files to a temporary local storage

			nFiles = 0;
			CTime tNow = CTime::GetCurrentTime();
			int nFilesTransferred = 0;
			int nFilesFTPFiltered = nFilesFTP;
			for (int fl = 0; fl < nFilesFTP; fl++) {
				CString sThisFile = aFilesReadyFTP[fl].sFileName;

				if (sThisFile == _T("."))
					continue;

				if (sThisFile == _T(".."))
					continue;

				if (ftp_in.FileExists(sThisFile, sErrorMessage) == FALSE) {
					g_util.Logprintf("WARNING: FTP  FileExists() return FALSE on file '%s' - error code '%s'", sThisFile, sErrorMessage);
					continue;
				}

				// do the stable time wait once
				if (pInput->m_stabletime > 0 && fl == 0) {
					::Sleep(pInput->m_stabletime * 1000);
				}
				if (pInput->m_stabletime > 0) {
					int nSizeNow = ftp_in.GetFileSize(sThisFile, sErrorMessage);
					if (nSizeNow < 0) {
						g_util.Logprintf("WARNING: FTP  GetFileSize() return FALSE on file '%s' - error code '%s'", sThisFile, sErrorMessage);
						continue;
					}

					if (nSizeNow != aFilesReadyFTP[fl].nFileSize) {
						g_util.Logprintf("WARNING: FTP  File %s still growing after stabletime.. (%d -> %d bytes)", sThisFile, aFilesReadyFTP[fl].nFileSize, nSizeNow);
						continue;
					}
				}

				pInput->m_inputpath.Format("%s\\%d", g_prefs.m_workfolder, pInput->m_ID);
				::CreateDirectory(pInput->m_inputpath, NULL);
				CString sTempName = pInput->m_inputpath + _T("\\") + sThisFile;

				::DeleteFile(sTempName);
				CString sProgressText = "";

				if (ftp_in.GetFile(sThisFile, sTempName, FALSE, sErrorMessage) == FALSE) {
					bReConnect = TRUE;
					g_util.Logprintf("ERROR: FTP GetFile() - %s", sErrorMessage);
					ftp_in.Disconnect();
					CString sErrorMessage2 = sErrorMessage;
					m_pollDB.UpdateService(SERVICESTATUS_HASERROR, _T(""), _T("FTP Error ") + sErrorMessage2, sErrorMessage);

					break;
					//continue;
				}
				else {
					if (g_util.FileExist(sTempName) == FALSE) {

						bReConnect = TRUE;
						g_util.Logprintf("ERROR:  Post FTP GetFile() - %s does not exist", sTempName);
						continue;
					}
					else {
						aFilesReady[nFiles].sFileName = aFilesReadyFTP[fl].sFileName;
						aFilesReady[nFiles].nFileSize = aFilesReadyFTP[fl].nFileSize;
						aFilesReady[nFiles].sFolder = _T("");
						aFilesReady[nFiles].tJobTime = aFilesReadyFTP[fl].tJobTime;
						aFilesReady[nFiles].tWriteTime = aFilesReadyFTP[fl].tWriteTime;
						nFiles++;

						if (g_prefs.m_logToFile > 1)
							g_util.Logprintf("FTP INFO: GetFile(%s -> %s) OK", sThisFile, sTempName);
					}
				}
				if (g_prefs.m_maxfilesperscan > 0 && nFiles >= g_prefs.m_maxfilesperscan)
					break;
			}

		} else if(pInput->m_useFTP == NETWORKTYPE_EMAIL) {
			pInput->m_inputpath.Format("%s\\%d", g_prefs.m_workfolder, pInput->m_ID);
			::CreateDirectory(pInput->m_inputpath, NULL);

			nFiles = g_util.GetMailAttachments(pInput->m_mailuseimap, pInput->m_mailserver, pInput->m_mailport, pInput->m_mailusername, pInput->m_mailpassword, pInput->m_mailusessl,
						pInput->m_mailfromlistfilter, TRUE, pInput->m_inputpath, aFilesReady, MAXFILES, pInput->m_mailimapinbox, sErrorMessage);
			if (nFiles < 0) {
				g_util.Logprintf("ERROR: GetMailAttachments() - %s ", sErrorMessage);
				CString sErrorMessage2 = sErrorMessage;
				m_pollDB.UpdateService(SERVICESTATUS_HASERROR, _T(""), _T("EMAIL Error ") + sErrorMessage2, sErrorMessage);

			}
			// will report later...
		}

		if (nFiles == FALSE) {	
			::Sleep(500);
			continue;
		}
		
		if (nFiles < 0) {
			bReConnect = TRUE;
			if (pInput->m_useFTP == NETWORKTYPE_SHARE) {
				g_util.Logprintf("ERROR: ScanDir failed for folder %s - cannot be accessed (2)", pInput->m_inputpath);
				if (g_prefs.m_firecommandonfolderreconnect && g_prefs.m_sFolderReconnectCommand != "") {
					g_util.DoCreateProcess(g_prefs.m_sFolderReconnectCommand);
					::Sleep(500);
				}

			}
			::Sleep(1000);
			continue;
				
		}
		
		// Files are now local in input folder...
		BOOL ignoremaxfile = FALSE;
		int filesProcessed = 0;
		for (int it = 0; it < nFiles; it++) {
		
			if (g_BreakSignal)
				break;

			if (g_prefs.m_maxfilesperscan > 0 && filesProcessed + 1 > g_prefs.m_maxfilesperscan)
				break;

			if (pInput->m_lowpriorityfolder && filesProcessed >= 1)
				break;

			FILEINFOSTRUCT fi = aFilesReady[it];
			
			CString sFullInputPath = pInput->m_inputpath + "\\" + fi.sFileName;
			if (pInput->m_useFTP == NETWORKTYPE_SHARE) {
				if (g_util.FileExist(sFullInputPath) == FALSE)
					continue;

				// Still tampered with??
				if (g_util.LockCheck(sFullInputPath) == FALSE) {
					if (g_prefs.m_logToFile > 3)
						g_util.Logprintf("DEBUG: Lockcheck failed on %s", sFullInputPath);
					continue;
				}
			}

			CString sExt = g_util.GetExtension(fi.sFileName);
			if (sExt != "") {
				if (g_prefs.m_useerrorloginformationfile) {
					if (sExt.CompareNoCase(_T("log")) == 0)
						continue;
				}

				if (g_prefs.m_ignoreextension != "") {
					if (sExt.CompareNoCase(g_prefs.m_ignoreextension) == 0)
						continue;
				}
				if (g_prefs.m_deletefileswithextension != "") {
					if (sExt.CompareNoCase(g_prefs.m_deletefileswithextension) == 0) {
						::DeleteFile(sFullInputPath);
						continue;
					}
				}
			}

			DWORD dwInitialSize = fi.nFileSize;
			CTime tInitialWriteTime = fi.tWriteTime;

			// Still file growth??


			if (pInput->m_stabletime > 0) {
				if (it == 0) {
					for (int j = 0; j < (int)pInput->m_stabletime * 10; j++) {
						Sleep(100);
						if (g_BreakSignal)
							break;
					}
				}

				if (pInput->m_useFTP == NETWORKTYPE_SHARE) {

					if (g_util.GetFileSize(sFullInputPath) != dwInitialSize) {
						if (g_prefs.m_logToFile > 3)
							g_util.Logprintf("DEBUG: File size growth failed on %s", sFullInputPath);
						continue;
					}
					if (g_util.GetWriteTime(sFullInputPath) != tInitialWriteTime) {
						if (g_prefs.m_logToFile > 3)
							g_util.Logprintf("DEBUG: File write time check failed on %s", sFullInputPath);
						continue;
					}
				}
			}

			if (ignoremaxfile == FALSE)
				filesProcessed++;
			ignoremaxfile = FALSE;

			BOOL bOverrideInputTime = FALSE;
			CTime tOverrideInputTime(1975, 1, 1, 0, 0, 0);
			CString sLogFileName = pInput->m_inputpath + _T(".log");
			if (g_prefs.m_useerrorloginformationfile) {

				g_util.Logprintf("INFO: Testing existing of error log file %s ...", sLogFileName);

				if (g_util.FileExist(sLogFileName)) {

					bOverrideInputTime = TRUE;
					tOverrideInputTime = g_util.GetLogFileTime(sLogFileName);
					if (tOverrideInputTime.GetYear() < 2000)
						tOverrideInputTime = fi.tWriteTime;

					g_util.Logprintf("INFO: Found error log file. Ajdusting InputTime to %s", g_util.Time2String(tOverrideInputTime));

				}
			}
			::DeleteFile(sLogFileName);


			BOOL bHasCopiedFile = FALSE;
			if (pInput->m_makecopy && pInput->m_copyfolder != _T("") && pInput->m_makecopyalways) {

				CString sCopyDestPath = pInput->m_copyfolder + _T("\\") + fi.sFileName;
				bHasCopiedFile = ::CopyFile(sFullInputPath, sCopyDestPath, FALSE);
			}

			//////////////////////////////////////
			// process this job (fi.sFileName)
			//////////////////////////////////////

			CString sErrorPath;
			sErrorPath.Format("%s%d\\%s", g_prefs.m_errorPath, pInput->m_ID, fi.sFileName);

			CString sErrorLogPath = sErrorPath + ".log";
			CTime logTime = m_pollDB.GetDatabaseTime(sErrorMessage);

			// Report only..

			g_util.Logprintf("INFO:  New file %s", sFullInputPath);
			g_util.ErrorFilePrintf(sErrorLogPath, "%s", g_util.Time2String(logTime));
			g_util.ErrorFilePrintf(sErrorLogPath, "New file found: %s", fi.sFileName);
		
			BOOL bPDFOK = IsPDF(sFullInputPath);

			int nPageCount = 0;
			double fPdfWidth = 0.0;
			double fPdfHeight = 0.0;
			BOOL bPDFTestOK = FALSE;
			if (bPDFOK)
				bPDFTestOK = GetPDFinfo(sFullInputPath, nPageCount, fPdfWidth, fPdfHeight);

			if (bPDFTestOK && (fPdfWidth == 0.0 || fPdfHeight == 0.0))
				bPDFTestOK = FALSE;

			BOOL bColorPDFPage = FALSE;
			CString sPdfReport = "";
			if (bPDFOK && bPDFTestOK) {
				if (LogPDFinfo(sFullInputPath, bColorPDFPage, sPdfReport) == TRUE)
					g_util.Logprintf("DEBUG: PDF information dump:\r\n %s", sPdfReport);
				if (sPdfReport.GetLength() > 0 && g_prefs.m_logPDFinfo) {
					if (sPdfReport.GetLength() > 1023)
						sPdfReport = sPdfReport.Left(1023);
					m_pollDB.UpdatePrepollStatus(Job.m_mastercopyseparationset, SERVICETYPE_FILEIMPORT, EVENTCODE_PDFINFO, sPdfReport, sErrorMessage);
				}
			}
			else {
				g_util.Logprintf("WARNING: Potential corrupt file %s - re-checking...", fi.sFileName);
				::Sleep(1000);
				bPDFOK = IsPDF(sFullInputPath);

				if (bPDFOK)
					bPDFTestOK = GetPDFinfo(sFullInputPath, nPageCount, fPdfWidth, fPdfHeight);

				if (bPDFTestOK && (fPdfWidth == 0.0 || fPdfHeight == 0.0))
					bPDFTestOK = FALSE;

				if (bPDFOK == FALSE || bPDFTestOK == FALSE) {
					CString s;

					s = fi.sFileName + " - File not a PDF file";
					g_util.SendMail(MAILTYPE_FILEERROR, s);

					g_util.ErrorFilePrintf(sErrorLogPath, "ERROR: File %s not TIFF or PDF file", fi.sFileName);
					g_util.Logprintf("ERROR: File %s not TIFF or PDF file", fi.sFileName);

					::MoveFileEx(sFullInputPath, sErrorPath, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);

					g_util.SendMail(MAILTYPE_POLLINGERROR, s);

					m_pollDB.UpdateService(SERVICESTATUS_HASERROR, fi.sFileName, "File type error (possibly corrupt) ",  sErrorMessage);
					m_pollDB.InsertLogEntry(EVENTCODE_POLLERROR, pInput->m_inputname, fi.sFileName, + _T("Unknown file type"),0,0,0,_T(""), sErrorMessage);
					if (pInput->m_sendackfile)
						g_util.IssueMessage(m_pollDB, Job, STATUSNUMBER_POLLINGERROR, pInput->m_ackfilefolder, 1036, sErrorMessage);

					continue;
				}
			}
			

			if (bPDFOK && pInput->m_splitPDF) {
	
				CString sInfo;
				int nPageCount = 0;
				if (SplitPDF(sFullInputPath, fi.sFileName, pInput->m_inputpath, 1, sInfo, nPageCount, sErrorMessage) == TRUE) {
					if (nPageCount > 1) {
						// Break out of file loop and re-scan folder.....
						break;
					}
				}
			}

			//////////////////////////////////////
			// Format name with regular expression
			//////////////////////////////////////

			CString sTranslatedName = fi.sFileName;

			CString sNameBeforeTranslated = sTranslatedName;

			BOOL bExpressionMatch = FALSE;
			if (pInput->m_useregexp) {
				for (int ex = 0; ex < pInput->m_NumberOfRegExps; ex++) {
					REGEXPSTRUCT *pRegExp = &pInput->m_RegExpList[ex];

					if (g_util.TryMatchExpression(pRegExp->m_matchExpression, sNameBeforeTranslated, pRegExp->m_partialmatch)) {
						sTranslatedName = g_util.FormatExpression(pRegExp->m_matchExpression, pRegExp->m_formatExpression, sNameBeforeTranslated, pRegExp->m_partialmatch);
						if (sTranslatedName != _T("")) {
							g_util.Logprintf("INFO: Translated %s to %s using regexp.", sNameBeforeTranslated, sTranslatedName);
							g_util.ErrorFilePrintf(sErrorLogPath, "Translated %s to %s using using regexp %s / %s ", sNameBeforeTranslated, sTranslatedName, pRegExp->m_matchExpression, pRegExp->m_formatExpression);
							bExpressionMatch = TRUE;
							break;	// Success!

						}
						else {
							g_util.Logprintf("INFO: No regular expression matches for file name %s", sNameBeforeTranslated);
						}
					}
				}
			}

			if (pInput->m_rejectifnoregularexpressionmatch && pInput->m_useregexp && pInput->m_NumberOfRegExps > 0 && bExpressionMatch == FALSE) {
				g_util.Logprintf("ERROR: Cannot match expression(s) for file name '%s' - job rejected", fi.sFileName);
				g_util.ErrorFilePrintf(sErrorLogPath, "ERROR - Cannot match expression(s) for file name '%s' - job rejected", fi.sFileName);

				CString s;
				s = _T("Name parsing error - ") + sNameBeforeTranslated;
				g_util.SendMail(MAILTYPE_POLLINGERROR, s);
				sErrorPath.Format("%s%d\\%s", g_prefs.m_errorPath, pInput->m_ID, sNameBeforeTranslated);
				::MoveFileEx(sFullInputPath, sErrorPath, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);

				m_pollDB.UpdateService(SERVICESTATUS_HASERROR, fi.sFileName, "Filename parsing error - " + sNameBeforeTranslated, sErrorMessage);
				m_pollDB.InsertLogEntry(EVENTCODE_POLLERROR, pInput->m_inputname, fi.sFileName, _T("Input name parse error"), 0, 0, 0, _T(""), sErrorMessage);
				if (pInput->m_sendackfile)
					g_util.IssueMessage(m_pollDB, Job, STATUSNUMBER_POLLINGERROR, pInput->m_ackfilefolder, 1036, sErrorMessage);

				continue;

			}

			//////////////////////////////////////
			// Format name with external script
			//////////////////////////////////////

			CString sOriginalScriptedInputName = "";;
			if (pInput->m_callscript) {
				if (g_util.FileExist(pInput->m_scriptfile)) {
					CString sCommandName;
					sOriginalScriptedInputName = sFullInputPath;
					sCommandName.Format("\"%s\" \"%s\" \"%s\"", pInput->m_scriptfile, sFullInputPath, g_util.GetTempFolder() + _T("\\Scripted"));

					CString sMsg;
					sMsg.Format("Calling script file %s", sCommandName);

					g_util.Logprintf("INFO:  Calling external script  %s", pInput->m_scriptfile);
					g_util.DeleteFiles(g_util.GetTempFolder() + _T("\\Scripted"), "*.*");
					if (g_util.DoCreateProcess(sCommandName)) {

						int nTimeOut = 10;
						while (--nTimeOut > 0) {
							CString s = g_util.GetFirstFile(g_util.GetTempFolder() + _T("\\Scripted"), _T("*.*"));
							if (s != "") {
								::DeleteFile(sFullInputPath);
								sTranslatedName = g_util.GetFileName(s);
								sFullInputPath.Format("%s\\%s", g_util.GetTempFolder() + _T("\\Scripted"), sTranslatedName);
								dwInitialSize = g_util.GetFileSize(sFullInputPath);
								tInitialWriteTime = g_util.GetWriteTime(sFullInputPath);
								g_util.Logprintf("INFO:  Script successfully renamed file to %s", sTranslatedName);
								g_util.ErrorFilePrintf(sErrorLogPath, "Translated %s to %s using script %s ", sNameBeforeTranslated, sTranslatedName, pInput->m_scriptfile);

								break;
							}
							Sleep(1000);
						}
						if (nTimeOut <= 0)
							g_util.Logprintf("ERROR (INPUT): Script timeout on file %s", sFullInputPath);

					}
					else
						g_util.Logprintf("ERROR: Error in CreateProcess for external script  %s", pInput->m_scriptfile);
				}
				else {
					g_util.Logprintf("ERROR: Script file  %s not found", pInput->m_scriptfile);
				}
			}

			//////////////////////////////////////
			// Decompose name into job structure 
			//////////////////////////////////////

			TCHAR szSearchName[260];
			_tcscpy(szSearchName, sTranslatedName);

			Job.InitJobAttributes(pInput);

			Job.m_filename = sNameBeforeTranslated;
			Job.m_ccdatafilename = g_util.GetFileName(sNameBeforeTranslated, TRUE);

			Job.m_colorname = bPDFOK ? _T("PDF") : _T("");
			Job.m_comment = "";
			Job.m_hasweekpubdate = FALSE;
			Job.m_pageindex = 0;
			Job.m_weekreference = 0;
			BOOL isGray = FALSE;
			BOOL bIsAutoCreated = FALSE;
			BOOL isWeekNumber = FALSE;
			BOOL isWeekReference = FALSE;
			BOOL hasVersion = FALSE;
			int nVersionFromFileName = 0;

			if (pInput->m_mayaddpage) {
				pInput->m_guessedition = FALSE;
				pInput->m_guesssection = FALSE;
				pInput->m_guesslocation = FALSE;
				pInput->m_guessrun = TRUE;
			}

			BOOL validName = g_util.Decompose(sTranslatedName, pInput, &Job, isGray, isWeekNumber, hasVersion);
			nVersionFromFileName = Job.m_version;

			Job.m_hasweekpubdate = isWeekNumber;

			if (Job.m_weekreference > 0 && pInput->m_useweekreference)
				Job.m_hasweekpubdate = FALSE;
			else
				Job.m_weekreference = 0;

				Job.m_pressrunID = 1;
				Job.m_locationID = 1;
			

			if (validName == FALSE) {
				g_util.Logprintf("ERROR: Cannot parse file name '%s' ('%s') using mask '%s'", sTranslatedName, fi.sFileName, pInput->m_namingmask);
				g_util.Logprintf("ERROR (INPUT): PlanPageName= %s", Job.m_planname);
				g_util.Logprintf("ERROR (INPUT): Publication = %s", g_prefs.GetPublicationName(Job.m_publicationID));
				g_util.Logprintf("ERROR (INPUT): Section     = %s", g_prefs.GetSectionName(Job.m_sectionID));
				g_util.Logprintf("ERROR (INPUT): Edition     = %s", g_prefs.GetEditionName(Job.m_editionID));
				g_util.Logprintf("ERROR (INPUT): Pubdate     = d:%d m:%d y:%d", Job.m_pubdate.GetDay(), Job.m_pubdate.GetMonth(), Job.m_pubdate.GetYear());
				if (isWeekNumber) {
					g_util.Logprintf("ERROR (INPUT): Week start   = d:%d m:%d y:%d", Job.m_pubdateweekstart.GetDay(), Job.m_pubdateweekstart.GetMonth(), Job.m_pubdateweekstart.GetYear());
					g_util.Logprintf("ERROR (INPUT): Week end     = d:%d m:%d y:%d", Job.m_pubdateweekend.GetDay(), Job.m_pubdateweekend.GetMonth(), Job.m_pubdateweekend.GetYear());
				}

				g_util.Logprintf("ERROR (INPUT): Week reference   = %d", Job.m_weekreference);
				g_util.Logprintf("ERROR (INPUT): Pagename    = %s", Job.m_pagename);
				g_util.Logprintf("ERROR (INPUT): Pagination  = %d", Job.m_pagination);
				g_util.Logprintf("ERROR (INPUT): PageIndex   = %d", Job.m_pageindex);

				g_util.Logprintf("ERROR (INPUT): Markgroup   = %s", Job.m_markgroup);
				g_util.Logprintf("ERROR (INPUT): Color       = %s", Job.m_colorname);

				g_util.ErrorFilePrintf(sErrorLogPath, "ERROR parsing file name '%s' ('%s') using mask %s", sTranslatedName, fi.sFileName, pInput->m_namingmask);
				g_util.ErrorFilePrintf(sErrorLogPath, "PlanPageName= %s", Job.m_planname);
				g_util.ErrorFilePrintf(sErrorLogPath, "Publication = %s", (Job.m_planname != "" && Job.m_colorname != "") ? "PLANPAGENAME" : g_prefs.GetPublicationName(Job.m_publicationID));
				g_util.ErrorFilePrintf(sErrorLogPath, "Section     = %s", (Job.m_planname != "" && Job.m_colorname != "") ? "PLANPAGENAME" : g_prefs.GetSectionName(Job.m_sectionID));
				g_util.ErrorFilePrintf(sErrorLogPath, "Edition     = %s", (Job.m_planname != "" && Job.m_colorname != "") ? "PLANPAGENAME" : g_prefs.GetEditionName(Job.m_editionID));

				g_util.ErrorFilePrintf(sErrorLogPath, "Version     = %d", nVersionFromFileName);
				if (Job.m_pubdate.GetYear() <= 2000 && pInput->m_uselatestdate)
					g_util.ErrorFilePrintf(sErrorLogPath, "Pubdate     = latest date in production (no date in file name)");
				else
					g_util.ErrorFilePrintf(sErrorLogPath, "Pubdate     = d:%d m:%d y:%d", Job.m_pubdate.GetDay(), Job.m_pubdate.GetMonth(), Job.m_pubdate.GetYear());

				g_util.ErrorFilePrintf(sErrorLogPath, "Week ref.   = %d", Job.m_weekreference);
				g_util.ErrorFilePrintf(sErrorLogPath, "Pagename    = %s", Job.m_pagename);
				g_util.ErrorFilePrintf(sErrorLogPath, "Pagination  = %d", Job.m_pagination);
				g_util.ErrorFilePrintf(sErrorLogPath, "PageIndex   = %d", Job.m_pageindex);
				
				g_util.ErrorFilePrintf(sErrorLogPath, "Markgroup   = %s", Job.m_markgroup);
				g_util.ErrorFilePrintf(sErrorLogPath, "Color       = %s", Job.m_colorname);

				if (bColorPDFPage)
					g_util.ErrorFilePrintf(sErrorLogPath, "Colorspace other than DeviceGray detected in file");

				if (g_prefs.GetPublicationName(Job.m_publicationID) == "") {
					g_util.Logprintf("ERROR: No publication name found");
					g_util.ErrorFilePrintf(sErrorLogPath, "No publication name found");
				}

				if (g_prefs.GetSectionName(Job.m_sectionID) == "") {
					g_util.Logprintf("ERROR: No section name found - are you missing a default setting?");
					g_util.ErrorFilePrintf(sErrorLogPath, "No section name found - are you missing a default setting?");
				}

				if (g_prefs.GetEditionName(Job.m_editionID) == "") {
					g_util.Logprintf("ERROR: No edition name found - are you missing a default setting?");
					g_util.ErrorFilePrintf(sErrorLogPath, "No edition name found - are you missing a default setting?");
				}

				if (Job.m_pagename == "" && Job.m_pagination == 0) {
					g_util.Logprintf("ERROR: No pagename or pagination number found");
					g_util.ErrorFilePrintf(sErrorLogPath, "No pagename or pagination number found");
				}

				if (Job.m_colorname == "") {
					g_util.Logprintf("ERROR: No color name found");
					g_util.ErrorFilePrintf(sErrorLogPath, "No color name found");
				}

				CString s;

				s = _T("Error parsing filename - ") + sNameBeforeTranslated;
				g_util.SendMail(MAILTYPE_POLLINGERROR, s);
				sErrorPath.Format("%s%d\\%s", g_prefs.m_errorPath, pInput->m_ID, sNameBeforeTranslated);
				::MoveFileEx(sFullInputPath, sErrorPath, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
	
				m_pollDB.UpdateService(SERVICESTATUS_HASERROR, fi.sFileName, "Filename parsing error - " + sNameBeforeTranslated, sErrorMessage);
				m_pollDB.InsertLogEntry(EVENTCODE_POLLERROR, pInput->m_inputname, fi.sFileName, _T("Input name parse error"), 0, 0, 0, _T(""), sErrorMessage);

				continue;
			}

			int nEditions = 1;

			if (pInput->m_callrenamessp) {
				if (!m_pollDB.CallMapNamesCustomSP(pInput->m_ID, &Job, sErrorMessage)) {
					g_util.Logprintf("ERROR: CallMapNamesCustomSP() - %s" , sErrorMessage);
					bReConnect = TRUE;
					m_pollDB.UpdateService(SERVICESTATUS_HASERROR, fi.sFileName, "Database error", sErrorMessage);
					m_pollDB.InsertLogEntry(EVENTCODE_POLLERROR, pInput->m_inputname, fi.sFileName, _T("Database error - ") + sErrorMessage, 0, 0, 0, _T(""), sErrorMessage);
					continue;
				}
			}

			// See if some IDs must be deducted from plan 
			if (Job.m_sectionID == 0 && Job.m_guesssection) {
				;
			}
			if (Job.m_editionID == 0 && Job.m_guessedition) {
				;
			}
			if (Job.m_pressrunID == 0 && Job.m_guessrun) {
				;
			}
			if (Job.m_locationID == 0 && Job.m_guesslocation) {
				;
			}

			if (isGray) {
				Job.m_colorname = _T("K");
				g_util.ErrorFilePrintf(sErrorLogPath, "Gray color detected - translating to color K");
			}

			CString sColor = Job.m_colorname;
			// Known color name?

			

			if (g_prefs.m_logToFile) {
				g_util.Logprintf("INFO: Filename = %s (%s)", sTranslatedName, sNameBeforeTranslated);
				g_util.Logprintf("      PlanPageName= %s", Job.m_planname);
				g_util.Logprintf("      Publication = %s (id %d)", (Job.m_planname != "" && Job.m_colorname != "") ? "PLANPAGENAME" : g_prefs.GetPublicationName(Job.m_publicationID), Job.m_publicationID);
				g_util.Logprintf("      Section     = %s (id %d)", (Job.m_planname != "" && Job.m_colorname != "") ? "PLANPAGENAME" : g_prefs.GetSectionName(Job.m_sectionID), Job.m_sectionID);
				g_util.Logprintf("      Edition     = %s (id %d)", (Job.m_planname != "" && Job.m_colorname != "") ? "PLANPAGENAME" : g_prefs.GetEditionName(Job.m_editionID), Job.m_editionID);
				if (Job.m_pubdate.GetYear() <= 2000 && pInput->m_uselatestdate)
					g_util.Logprintf("      Pubdate     = Latest date (no date in filename");
				else
					g_util.Logprintf("      Pubdate     = d:%d m:%d y:%d", Job.m_pubdate.GetDay(), Job.m_pubdate.GetMonth(), Job.m_pubdate.GetYear());

				if (isWeekNumber) {
					g_util.Logprintf("      Week start   = d:%d m:%d y:%d", Job.m_pubdateweekstart.GetDay(), Job.m_pubdateweekstart.GetMonth(), Job.m_pubdateweekstart.GetYear());
					g_util.Logprintf("      Week end     = d:%d m:%d y:%d", Job.m_pubdateweekend.GetDay(), Job.m_pubdateweekend.GetMonth(), Job.m_pubdateweekend.GetYear());
				}

				if (Job.m_auxpubdate.GetYear() > 2000)
					g_util.Logprintf("  Aux Pubdate     = d:%d m:%d y:%d", Job.m_auxpubdate.GetDay(), Job.m_auxpubdate.GetMonth(), Job.m_auxpubdate.GetYear());

				g_util.Logprintf("      Week ref.   = %d", Job.m_weekreference);

				g_util.Logprintf("      Pagename    = %s", Job.m_pagename);

				g_util.Logprintf("      Pagination  = %d", Job.m_pagination);
				g_util.Logprintf("      PageIndex   = %d", Job.m_pageindex);
				g_util.Logprintf("      Comment     = %s", Job.m_comment);

				//util.Logprintf("      Location    = %s (id %d)", g_prefs.GetLocationName(Job.m_locationID), Job.m_locationID);

				CString sGroupName = Job.m_locationgroupIDList;
				
				g_util.Logprintf("      Group List  = %s", sGroupName);

				g_util.Logprintf("      Color       = %s", Job.m_colorname);
				//g_util.Logprintf("      Markgroup   = %s (name %s, include %d)", Job.m_markgroup, sMarkGroupFound, bMarkGroupInclude);
				g_util.Logprintf("      Guess Edition=%d,Guess Issue=%d,Guess Section=%d,Guess Location=%d", Job.m_guessedition, Job.m_guessrun, Job.m_guesssection, Job.m_guesslocation);
				g_util.Logprintf("      Pagetype    = %d", Job.m_pagetype);
				g_util.Logprintf("      Panoramapage= %s", Job.m_panomate);
				g_util.Logprintf("      MiscString1 =%s", Job.m_miscstring1);
				g_util.Logprintf("      MiscString2 =%s", Job.m_miscstring2);
				g_util.Logprintf("      MiscString3 =%s", Job.m_miscstring3);
				if (bColorPDFPage)
					g_util.Logprintf("      Colorspace other than DeviceGray detected in file");

			}

			g_util.ErrorFilePrintf(sErrorLogPath, "File name '%s' ('%s') parsed using mask %s", sTranslatedName, fi.sFileName, pInput->m_namingmask);
			g_util.ErrorFilePrintf(sErrorLogPath, "PlanPageName= %s", Job.m_planname);
			g_util.ErrorFilePrintf(sErrorLogPath, "Publication = %s", g_prefs.GetPublicationName(Job.m_publicationID));
			g_util.ErrorFilePrintf(sErrorLogPath, "Section     = %s", g_prefs.GetSectionName(Job.m_sectionID));
			g_util.ErrorFilePrintf(sErrorLogPath, "Edition     = %s", g_prefs.GetEditionName(Job.m_editionID));

			if (Job.m_pubdate.GetYear() <= 2000 && pInput->m_uselatestdate)
				g_util.ErrorFilePrintf(sErrorLogPath, "Pubdate     = latest date in production (no date in file name)");
			else
				g_util.ErrorFilePrintf(sErrorLogPath, "Pubdate     = d:%d m:%d y:%d", Job.m_pubdate.GetDay(), Job.m_pubdate.GetMonth(), Job.m_pubdate.GetYear());

			if (isWeekNumber) {
				g_util.ErrorFilePrintf(sErrorLogPath, "Week start   = d:%d m:%d y:%d", Job.m_pubdateweekstart.GetDay(), Job.m_pubdateweekstart.GetMonth(), Job.m_pubdateweekstart.GetYear());
				g_util.ErrorFilePrintf(sErrorLogPath, "Week end     = d:%d m:%d y:%d", Job.m_pubdateweekend.GetDay(), Job.m_pubdateweekend.GetMonth(), Job.m_pubdateweekend.GetYear());
			}

			if (Job.m_auxpubdate.GetYear() > 2000 && pInput->m_allowpastpubdate)
				g_util.ErrorFilePrintf(sErrorLogPath, "Aux Pubdate  = d:%d m:%d y:%d", Job.m_auxpubdate.GetDay(), Job.m_auxpubdate.GetMonth(), Job.m_auxpubdate.GetYear());


			g_util.ErrorFilePrintf(sErrorLogPath, "Week ref.   = %d", Job.m_weekreference);

			g_util.ErrorFilePrintf(sErrorLogPath, "Pagename    = %s", Job.m_pagename);
			g_util.ErrorFilePrintf(sErrorLogPath, "Pagination  = %d", Job.m_pagination);
			g_util.ErrorFilePrintf(sErrorLogPath, "Version  = %d", Job.m_version);
			g_util.ErrorFilePrintf(sErrorLogPath, "Markgroup   = %s", Job.m_markgroup);
			g_util.ErrorFilePrintf(sErrorLogPath, "Color       = %s", Job.m_colorname);
			g_util.ErrorFilePrintf(sErrorLogPath, "Panoramapage=%s", Job.m_panomate);
			g_util.ErrorFilePrintf(sErrorLogPath, "MiscString1 =%s", Job.m_miscstring1);
			g_util.ErrorFilePrintf(sErrorLogPath, "MiscString2 =%s", Job.m_miscstring2);
			g_util.ErrorFilePrintf(sErrorLogPath, "MiscString3 =%s", Job.m_miscstring3);
			if (bColorPDFPage)
				g_util.ErrorFilePrintf(sErrorLogPath, "Colorspace other than DeviceGray detected in file");

			sColor = _T("PDF");

			if (g_BreakSignal)
				break;

			// remove leading zeros
			while (Job.m_pagename[0] == _T('0'))
				Job.m_pagename = Job.m_pagename.Mid(1);



			//////////////////////////////////////
				// First database select - In progress right now ?
				//////////////////////////////////////

			TCHAR szMessage[MAX_PATH];
			CString sJobName = sNameBeforeTranslated;
			int dbAccessStat = -1;
			BOOL bPlateAdded = FALSE;

			int		nCurrentPlanLock = PLANLOCK_UNKNOWN;
			CString	sPlanClientName, sPlanClientTime;

			int bIsLocked = FALSE;
			BOOL bDoUnlock = FALSE;
			CString sLockClientName;
			CString sLockClientTime;

			int nCurrentLock = PLANLOCK_UNKNOWN;

			if (Job.m_publicationID > 0 && Job.m_pubdate.GetYear() > 2000 && g_prefs.m_publicationlock) {
				m_pollDB.PublicationPlanLock(Job.m_publicationID, Job.m_pubdate, Job.m_auxpubdate, PLANLOCK_LOCK, nCurrentLock, sLockClientName, sLockClientTime, sErrorMessage);
				if (nCurrentLock == PLANLOCK_NOLOCK) { // Could not lock!
					dbAccessStat = POLLDBSTATUS_PAGEINPROGRESS;
					g_util.Logprintf("INFO:  Publication is currently locked for planning by %s ", sLockClientName);
					bIsLocked = TRUE;
				}
				if (nCurrentLock == PLANLOCK_LOCK) {
					bDoUnlock = TRUE;
				}
			}

			if (bIsLocked == FALSE) {

				if (Job.m_publicationID == 0 ||
					Job.m_editionID == 0 && Job.m_guessedition == FALSE ||
					Job.m_sectionID == 0 && Job.m_guesssection == FALSE) {

					g_util.Logprintf("ERROR: One or more IDs in file name not defined in system. '%s' unknown to system", sNameBeforeTranslated);
					g_util.ErrorFilePrintf(sErrorLogPath, "ERROR: One or more IDs in file name not defined in system");

					if (Job.m_editionID == 0 && Job.m_guessedition == FALSE)
						sLastError = _T("Unknown edition name");
					if (Job.m_publicationID == 0)
						sLastError = _T("Unknown publication name");
					if (Job.m_sectionID == 0 && Job.m_guesssection == FALSE)
						sLastError = _T("Unknown section name");
				
					g_util.ErrorFilePrintf(sErrorLogPath, "ERROR: %s", sLastError);

					CString sOrgName = sNameBeforeTranslated;
					if (sTranslatedName != sOrgName)
						sOrgName += " (" + sTranslatedName + ")";
					CString s;
					s = sOrgName + _T(" - ") + sLastError;
					g_util.SendMail(MAILTYPE_POLLINGERROR, s);
						
					sErrorPath.Format("%s%d\\%s", g_prefs.m_errorPath, pInput->m_ID, sNameBeforeTranslated);
					MoveFileEx( sFullInputPath, sErrorPath, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);

					m_pollDB.UpdateService(SERVICESTATUS_HASERROR, sOrgName, sLastError, _T(""), sErrorMessage);
					m_pollDB.InsertLogEntry(EVENTCODE_POLLERROR, pInput->m_inputname, fi.sFileName, sLastError, 0, 0, 0, _T(""), sErrorMessage);
					if (pInput->m_sendackfile)
						g_util.IssueMessage(m_pollDB, Job, STATUSNUMBER_POLLINGERROR, pInput->m_ackfilefolder, 1036, sErrorMessage);

					continue;
				}

				if (Job.m_pubdatefound == FALSE && pInput->m_uselatestdate == FALSE && pInput->m_mayaddpage) {

					Job.m_pubdatefound = TRUE;
					CTime tm = CTime::GetCurrentTime();
					tm += CTimeSpan(pInput->m_pubdateplus, -1 * pInput->m_rollovertime.GetHour(), -1 * pInput->m_rollovertime.GetMinute(), -1 * pInput->m_rollovertime.GetSecond());
					Job.m_pubdate = tm;

				}

				if (Job.m_publicationID > 0) {
					PUBLICATIONSTRUCT *pPub = g_prefs.GetPublicationStruct(Job.m_publicationID);
					if (pPub != NULL)
						Job.m_approved = pPub->m_Approve;
				}

				dbAccessStat = m_pollDB.LookupInputJob(&Job, pInput, sNameBeforeTranslated, pInput->m_copyeditions, sErrorMessage);
				g_util.Logprintf("INFO:  Database reported code %d for file %s", dbAccessStat, sNameBeforeTranslated);
				g_util.ErrorFilePrintf(sErrorLogPath, "Database lookup reported code %d", dbAccessStat);

				if (dbAccessStat < 0) {
					// Report db error..
					bReConnect = TRUE;
					sLastError = _T("Database error trying to lookup job");
					g_util.Logprintf("ERROR: Database access error (LookupInputJob) - %s", sErrorMessage);


					sErrorPath.Format("%s%d\\%s", g_prefs.m_errorPath, pInput->m_ID, sNameBeforeTranslated);
					::MoveFileEx(sFullInputPath, sErrorPath, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);

					m_pollDB.UpdateService(SERVICESTATUS_HASERROR, fi.sFileName, sLastError + " - " + sNameBeforeTranslated, sErrorMessage);
					m_pollDB.InsertLogEntry(EVENTCODE_POLLERROR, pInput->m_inputname, fi.sFileName, +_T("Database error looking up job"), 0, 0, 0, _T(""), sErrorMessage);

					g_util.SendMail(MAILTYPE_DBERROR, sLastError);
					continue;
				}

				CTime tSavedPubDate = Job.m_pubdate;

				BOOL bAllowUnplanned1 = pInput->m_mayaddpage;
				if (pInput->m_mayaddpage) {
					m_pollDB.LoadNewPublicationName(Job.m_publicationID, sErrorMessage);
					PUBLICATIONSTRUCT *pPublication1 = g_prefs.GetPublicationStruct(Job.m_publicationID);
					if (pPublication1 != NULL /*&& g_prefs.m_respectallowunplannedforpublication*/)
						bAllowUnplanned1 = pPublication1->m_allowunplanned;

					if (pInput->m_ID == g_prefs.m_MayAddPagesInputID)
						bAllowUnplanned1 = TRUE;

				}

				if (dbAccessStat == POLLDBSTATUS_NEWPAGE && bAllowUnplanned1 && g_prefs.m_planlock) {
					// Aquire planlock and retry lookup..

					g_util.Logprintf("INFO:  Requesting plan lock..");
					int nWaitLoop = 0;
					nCurrentPlanLock = PLANLOCK_UNKNOWN;
					while (nCurrentPlanLock != PLANLOCK_LOCK && g_BreakSignal == FALSE) {
						if (m_pollDB.PlanLock(PLANLOCK_LOCK, nCurrentPlanLock, sPlanClientName, sPlanClientTime, sErrorMessage) == FALSE) {
							g_util.Logprintf("ERROR: %s", szMessage);
							break;
						}
						if (nCurrentPlanLock == PLANLOCK_NOLOCK) {
							//if (nWaitLoop%100==0)
							g_util.Logprintf("INFO:  Unable to accuire plan lock. Owner: %s at %s", sPlanClientName, sPlanClientTime);

							nWaitLoop++;
							Sleep(3000);
						}
						else if (nCurrentPlanLock == PLANLOCK_LOCK) {
							g_util.Logprintf("INFO:  Got plan lock - proceeding..");
							break;
						}
					}

					if (g_BreakSignal) {
						m_pollDB.PlanLock(PLANLOCK_UNLOCK, nCurrentPlanLock, sPlanClientName, sPlanClientTime, sErrorMessage);
						break;
					}

					// Re-run query
					Job.m_pubdate = tSavedPubDate;

					g_util.Logprintf("WARNING: Re-running query with plan lock..(%d)", nCurrentPlanLock);
					Sleep(100);

					dbAccessStat = m_pollDB.LookupInputJob(&Job,  pInput, sNameBeforeTranslated, pInput->m_copyeditions, sErrorMessage);

					g_util.Logprintf("INFO:  (Planlock'ed) - database reported code %d for file %s", dbAccessStat, sNameBeforeTranslated);
					g_util.ErrorFilePrintf(sErrorLogPath, "(Planlock'ed) - database lookup reported code %d", dbAccessStat);

					if (dbAccessStat < 0) {
						// Report db error..
						bReConnect = TRUE;
						sLastError = _T("Database error trying to lookup job (2)");
						g_util.Logprintf("ERROR: Database access error (LookupInputJob(2)) - %s", sErrorMessage);

						sErrorPath.Format("%s%d\\%s", g_prefs.m_errorPath, pInput->m_ID, sNameBeforeTranslated);
						::MoveFileEx(sFullInputPath, sErrorPath, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);

						m_pollDB.UpdateService(SERVICESTATUS_HASERROR, fi.sFileName, sLastError + " - " + sNameBeforeTranslated, sErrorMessage);
						m_pollDB.InsertLogEntry(EVENTCODE_POLLERROR, pInput->m_inputname, fi.sFileName, +_T("Database error looking up job"), 0, 0, 0, _T(""), sErrorMessage);

						g_util.SendMail(MAILTYPE_DBERROR, sLastError);
						continue;
					}


				}

				g_util.Logprintf("INFO:  Auto-plate generation system %d",  pInput->m_autogenerateplate ? "enabled" : "disabled");

				if (dbAccessStat == POLLDBSTATUS_NEWPAGE  && pInput->m_autogenerateplate &&
					Job.m_publicationID > 0 && Job.m_pubdate.GetYear() > 2000 && (Job.m_editionID > 0 || pInput->m_guessedition) && (Job.m_sectionID > 0 || pInput->m_guesssection) && (Job.m_pagename != "" || Job.m_pagination > 0) && Job.m_colorname != "") {

					g_util.Logprintf("INFO:  Trying to add plate set for unplanned sub-edition..");

					// Re-run query with originally found date
					Job.m_pubdate = tSavedPubDate;

					int nNewPlateRet = m_pollDB.CreateSubEditionPlate(&Job, pInput->m_autogenerateplatehold, pInput->m_autogenerateplateapproval, pInput->m_autogenerateplatefreedevice, sErrorMessage);
					if (nNewPlateRet == 0 && Job.m_auxpubdate != Job.m_pubdate && pInput->m_allowpastpubdate) {
						Job.m_pubdate = Job.m_auxpubdate;
						nNewPlateRet = m_pollDB.CreateSubEditionPlate(&Job, pInput->m_autogenerateplatehold, pInput->m_autogenerateplateapproval, pInput->m_autogenerateplatefreedevice, sErrorMessage);
						if (nNewPlateRet == 0)
							Job.m_pubdate = tSavedPubDate;
					}

					if (nNewPlateRet == 0)
						g_util.Logprintf("WARNING: Did not find press run with master edition plate to copy..");
					else {
						g_util.Logprintf("INFO: Created new plate set for sub-edition..! - retrying polling for this new plate set..");
						dbAccessStat = m_pollDB.LookupInputJob(&Job, pInput, sNameBeforeTranslated, pInput->m_copyeditions, sErrorMessage);
						bPlateAdded = TRUE;
						g_util.Logprintf("INFO:  Database reported code %d for file %s (after plate add)", dbAccessStat, sNameBeforeTranslated);
					}

				}
			}

			// We have hit a locked timed edition - re-try with next-in-line edition
			int nTimedEditionRetries = 20;
			if (dbAccessStat == POLLDBSTATUS_GOT_TIMED_EDITION)
			{
				do {
					dbAccessStat = m_pollDB.LookupInputJob(&Job,  pInput, sNameBeforeTranslated, pInput->m_copyeditions, sErrorMessage);
				} while (dbAccessStat == POLLDBSTATUS_GOT_TIMED_EDITION && --nTimedEditionRetries > 0);
			}

			BOOL bAllowUnplanned = pInput->m_mayaddpage;
			m_pollDB.LoadNewPublicationName(Job.m_publicationID, sErrorMessage);
			PUBLICATIONSTRUCT *pPublication = g_prefs.GetPublicationStruct(Job.m_publicationID);
			if (pPublication != NULL && pInput->m_mayaddpage /* &&g_prefs.m_respectallowunplannedforpublication*/) {
				bAllowUnplanned = pPublication->m_allowunplanned;

				if (pInput->m_ID == g_prefs.m_MayAddPagesInputID)
					bAllowUnplanned = TRUE;

			}

			if (dbAccessStat == POLLDBSTATUS_NEWPAGE && bAllowUnplanned) { // New page flag!
				g_util.ErrorFilePrintf(sErrorLogPath, "Database lookup - new page detected");

				Job.m_mastercopyseparationset = 0;			 // First site will be copyseparationset (in sp). Other sites will re-use masterset

				if (Job.m_pubdatefound == FALSE) {
					CTime tm = CTime::GetCurrentTime();
					tm += CTimeSpan(pInput->m_pubdateplus, -1 * pInput->m_rollovertime.GetHour(), -1 * pInput->m_rollovertime.GetMinute(), -1 * pInput->m_rollovertime.GetSecond());
					Job.m_pubdate = tm;
				}

				// Look if PublicationTemplate default exists...
				BOOL bHasProofDefault = FALSE;
				CString sMiscString3ForUnknownPage = _T("");
				int nLocationIDForUnknownPage = 1;
				int nPressIDForUnknownPage = 1;
				int nTemplateIDForUnbknownPage = 1;
				int nApproveForUnknownPage = 0;
				int nPriorityForUnknownPage = 50;
				int nCopiesForUnknownPage = 1;
				BOOL bHoldForUnknownPage = pInput->m_putonhold;
				CString sMarkGroupsForUnknownPage = _T("");

				if (pPublication != NULL) {

					Job.m_approved = pPublication->m_Approve; //	
					Job.m_prooftemplateID = pPublication->m_ProofID;

					if (pPublication->m_ProofID > 0)
						bHasProofDefault = TRUE;

					CTime tm = Job.m_pubdate;

					tm += CTimeSpan(-1 * pPublication->m_deadline.GetDay(), -1 * pPublication->m_deadline.GetHour(), -1 * pPublication->m_deadline.GetMinute(), 0);
					Job.m_deadline = tm;

					sMiscString3ForUnknownPage = pPublication->m_channelIDList;

				}

						
				Job.m_approved = pInput->m_approvalrequired;
				Job.m_copies = pInput->m_defaultcopies;
				Job.m_hold = pInput->m_putonhold;

				if (bHasProofDefault == FALSE)
					Job.m_prooftemplateID = pInput->m_defaultprooftemplateID;

				Job.m_priority = pInput->m_priority;
				Job.m_locationID = 1;// g_prefs.GetLocationID(g_prefs.m_LocationList[i].m_locationname);
				Job.m_templateID = 1; // g_prefs.GetTemplateID(pInput->m_locationtemplate[i]);
				Job.m_miscstring3 = (pPublication->m_autoassignchannelstounplanned || g_prefs.m_alwaysautoassignchannels) ? sMiscString3ForUnknownPage : _T("");

				g_util.Logprintf("INFO: New page mode: MiscString3=%s", sMiscString3ForUnknownPage);

				int ret = m_pollDB.InsertUnplannedPage(&Job, pInput->m_unplannedrequiresapply, FALSE, FALSE, sErrorMessage);

				g_util.Logprintf("INFO: InsertUnplannedPage(2) returned %d, masterset=%d", ret, Job.m_mastercopyseparationset);
				if (ret == 2) {
					dbAccessStat = POLLDBSTATUS_PRODUCTIONLOCKED;
				}
				if (ret == 0) {
					dbAccessStat = -1;
				}
				if (g_prefs.m_planlock) {
					g_util.Logprintf("INFO:  Releasing plan-lock");
					m_pollDB.PlanLock(PLANLOCK_UNLOCK, nCurrentPlanLock, sPlanClientName, sPlanClientTime, sErrorMessage);
				}
				dbAccessStat = POLLDBSTATUS_NEWPAGE;
				Job.m_version = 0;
						
				if (g_prefs.m_planlock == PLANLOCK_LOCK) {
					g_util.Logprintf("INFO:  Releasing plan-lock");
					m_pollDB.PlanLock(PLANLOCK_UNLOCK, nCurrentPlanLock, sPlanClientName, sPlanClientTime, sErrorMessage);
				}

			}

			if (dbAccessStat < 0) {
				// Report db error..
				bReConnect = TRUE;
				sLastError = _T("Database error trying to lookup job");
				g_util.Logprintf("ERROR: Database access error (LookupInputJob) - %s", sErrorMessage);
				
				sErrorPath.Format("%s%d\\%s", g_prefs.m_errorPath, pInput->m_ID, sNameBeforeTranslated);
				::MoveFileEx(sFullInputPath, sErrorPath, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);

				g_util.SendMail(MAILTYPE_DBERROR, sLastError);
				continue;
			}
			 

			if (dbAccessStat == POLLDBSTATUS_PAGEINPROGRESS) {
				// File in progress internally (status=15 or 25 or 35 or 45 or 55)
				::DeleteFile(sErrorLogPath);
				// Move back scripted file to original name in case of in-progress state..
				if (sOriginalScriptedInputName != "")
					::MoveFileEx(sFullInputPath, sOriginalScriptedInputName, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);

				if (bDoUnlock)
					m_pollDB.PublicationPlanLock(Job.m_publicationID, Job.m_pubdate, Job.m_auxpubdate, PLANLOCK_UNLOCK, nCurrentLock, sLockClientName, sLockClientTime, sErrorMessage);

				continue;
			}

			if (dbAccessStat == POLLDBSTATUS_COLORNOTINPRODUCTION || dbAccessStat == POLLDBSTATUS_PRODUCTIONLOCKED || dbAccessStat == POLLDBSTATUS_PRODUCTIONCOLORLOCKED) {
				// Color error
				if (dbAccessStat == POLLDBSTATUS_COLORNOTINPRODUCTION) {
					sLastError = _T("Color not in production");
					g_util.Logprintf("ERROR: Illegal color %s for parsed file name '%s'", Job.m_colorname, sNameBeforeTranslated);
					g_util.ErrorFilePrintf(sErrorLogPath, "ERROR: Polled color is not valid according to production plan");
				}
				else if (dbAccessStat == POLLDBSTATUS_PRODUCTIONLOCKED) {
					sLastError = _T("Production is locked - no more pages allowed");
					g_util.Logprintf("ERROR: Production restricts more pages '%s'", sNameBeforeTranslated);
					g_util.ErrorFilePrintf(sErrorLogPath, "ERROR: Production restricts more pages at this time");
				}
				else { // 12
					sLastError = _T("Production restricts more colors");
					g_util.Logprintf("ERROR:  Production restricts more colors (%s) for parsed file name '%s'", Job.m_colorname, sNameBeforeTranslated);
					g_util.ErrorFilePrintf(sErrorLogPath, "ERROR: Production restricts adding colors at this time");
				}


				CString s;
				CString sOrgName = sNameBeforeTranslated;
				if (sTranslatedName != sOrgName)
					sOrgName += " (" + sTranslatedName + ")";
				s = sOrgName + _T(" - ") + sLastError;
				g_util.SendMail(MAILTYPE_POLLINGERROR, s);

				sErrorPath.Format("%s%d\\%s", g_prefs.m_errorPath, pInput->m_ID, sNameBeforeTranslated);
				::MoveFileEx(sFullInputPath, sErrorPath, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
				m_pollDB.UpdateService(SERVICESTATUS_HASERROR, sOrgName, sLastError, sErrorMessage);
				m_pollDB.InsertLogEntry(EVENTCODE_POLLERROR, pInput->m_inputname, fi.sFileName, sLastError, 0, 0, 0, _T(""), sErrorMessage);
				if (pInput->m_sendackfile)
					g_util.IssueMessage(m_pollDB, Job, STATUSNUMBER_POLLINGERROR, pInput->m_ackfilefolder, 1036, sErrorMessage);
				continue;

			}

			if (dbAccessStat == POLLDBSTATUS_ILLEGALCOLORPAGE || dbAccessStat == POLLDBSTATUS_UNKNOWNPAGE || (dbAccessStat == POLLDBSTATUS_NEWPAGE && !bAllowUnplanned) || dbAccessStat == POLLDBSTATUS_GOT_TIMED_EDITION) {


				if (dbAccessStat == POLLDBSTATUS_ILLEGALCOLORPAGE) {
					sLastError = _T("Parsed file name has illegal colors for plan");
					g_util.Logprintf("ERROR: Parsed file name '%s' has illegal colors for plan", sTranslatedName);
					g_util.ErrorFilePrintf(sErrorLogPath, "ERROR: Parsed file name '%s'  has illegal colors for plan", sTranslatedName);
					sLastError = "Illegal page colors for plan";
				}

				if (dbAccessStat == POLLDBSTATUS_UNKNOWNPAGE) {
					sLastError = _T("Parsed file name unknown to system");
					g_util.Logprintf("ERROR: Parsed file name '%s' unknown to system", sTranslatedName);
					g_util.ErrorFilePrintf(sErrorLogPath, "ERROR: Parsed file name '%s' unknown to system", sTranslatedName);					
				}

				if (dbAccessStat == POLLDBSTATUS_NEWPAGE && !bAllowUnplanned) {
					sLastError = _T("Parsed file name not in production");
					g_util.Logprintf("ERROR: Parsed file name '%s' not in production", sTranslatedName);
					g_util.ErrorFilePrintf(sErrorLogPath, "ERROR: Parsed file name '%s' not in production", sTranslatedName);
				}

				CString s;
				CString sOrgName = sNameBeforeTranslated;
				if (sTranslatedName != sOrgName)
					sOrgName += " (" + sTranslatedName + ")";
				s = sOrgName + _T(" - ") + sLastError;
				g_util.SendMail(MAILTYPE_POLLINGERROR, s);

				sErrorPath.Format("%s%d\\%s", g_prefs.m_errorPath, pInput->m_ID, sNameBeforeTranslated);
				::MoveFileEx(sFullInputPath, sErrorPath, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);


				m_pollDB.UpdateService(SERVICESTATUS_HASERROR, fi.sFileName, "Filename parsing error - " + sNameBeforeTranslated, sErrorMessage);
				m_pollDB.InsertLogEntry(EVENTCODE_POLLERROR, pInput->m_inputname, fi.sFileName, sLastError, 0, 0, 0, _T(""), sErrorMessage);
				m_pollDB.PlanLock(PLANLOCK_UNLOCK, nCurrentPlanLock, sPlanClientName, sPlanClientTime, sErrorMessage);
				if (pInput->m_sendackfile)
					g_util.IssueMessage(m_pollDB, Job, STATUSNUMBER_POLLINGERROR, pInput->m_ackfilefolder, 1036, sErrorMessage);

				continue;
			}


			// Gray color conversion?

			if (bPDFOK && bColorPDFPage && Job.m_miscstring2 == "K" && g_prefs.m_testforillegalpdfcolors) {

				// RESET STATUS!
				int dbStatus = m_pollDB.UpdateStatusInput(pInput->m_ID, Job.m_mastercopyseparationset, pInput->m_inputtype, Job.m_previousstatus, "!",
					sNameBeforeTranslated, Job.m_comment, REAPPROVEUPDATES_NOCHANGE, sErrorMessage);
				sLastError = _T("PDF non-gray color space");

				g_util.Logprintf("ERROR: Parsed file  '%s' has illegal non-gray color space", sTranslatedName);
				g_util.ErrorFilePrintf(sErrorLogPath, "ERROR: Parsed file '%s'  has illegal non-gray color space", sTranslatedName);
				CString sOrgName = sNameBeforeTranslated;
				if (sTranslatedName != sOrgName)
					sOrgName += " (" + sTranslatedName + ")";
				CString s = sOrgName + " - Illegal color space";
				g_util.SendMail(MAILTYPE_POLLINGERROR, s);

				sErrorPath.Format("%s%d\\%s", g_prefs.m_errorPath, pInput->m_ID, sNameBeforeTranslated);
				::MoveFileEx(sFullInputPath, sErrorPath, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);

				m_pollDB.UpdateService(SERVICESTATUS_HASERROR, sOrgName, "PDF color space error", sErrorMessage);
				m_pollDB.InsertLogEntry(EVENTCODE_POLLERROR, pInput->m_inputname, fi.sFileName, sLastError, Job.m_mastercopyseparationset, 0, 0, _T(""), sErrorMessage);
				m_pollDB.PlanLock(PLANLOCK_UNLOCK, nCurrentPlanLock, sPlanClientName, sPlanClientTime, sErrorMessage);

				if (bDoUnlock)
					m_pollDB.PublicationPlanLock(Job.m_publicationID, Job.m_pubdate, Job.m_auxpubdate, PLANLOCK_UNLOCK, nCurrentLock, sLockClientName, sLockClientTime, sErrorMessage);
				continue;
			}		

			if (g_prefs.m_reloadpublicationdetails) {
				m_pollDB.LoadNewPublicationName(Job.m_publicationID, sErrorMessage);
			}

			// Log..

			g_util.Logprintf("INFO: Parse result - file name '%s' decoded to pubID=%d, secID=%d, edID=%d, date=%s, page=%s, color=%s",
				sNameBeforeTranslated, Job.m_publicationID, Job.m_sectionID, Job.m_editionID, Job.m_pubdate.Format("%d.%m.%Y"), Job.m_pagename, Job.m_colorname);

			CString sThisPubDate;
			sThisPubDate.Format("%d/%d/%d", Job.m_pubdate.GetDay(), Job.m_pubdate.GetMonth(), Job.m_pubdate.GetYear());
			CString sPollMsg = "Pub: " + g_prefs.GetPublicationName(Job.m_publicationID) + 
								" | Pg: " + Job.m_pagename + " " + Job.m_colorname +
								" | Date: " + sThisPubDate +
								" | Sec: " + g_prefs.GetSectionName(Job.m_sectionID) +
								" | Ed: " + g_prefs.GetEditionName(Job.m_editionID);

			if (dbAccessStat == 2 || dbAccessStat == 3 || dbAccessStat == 4) {
				sPollMsg += " Added to system";
				g_util.ErrorFilePrintf(sErrorLogPath, "File %s added to system", sTranslatedName);
			}

			BOOL bAddedEditionPage = FALSE;
			if (dbAccessStat == POLLDBSTATUS_SUBEDITIONPAGEADDED) {
				sPollMsg += " Edition page added to system";
				g_util.Logprintf("Edition page added to system");
				g_util.ErrorFilePrintf(sErrorLogPath, "Edition page added to system");
				bAddedEditionPage = TRUE;
			}

			if (dbAccessStat == POLLDBSTATUS_ISSUEPAGEADDED) {
				sPollMsg += " Issue added to system";
				g_util.ErrorFilePrintf(sErrorLogPath, "Issue page added to system");

			}
			if (dbAccessStat == POLLDBSTATUS_NEWPAGE) {
				sPollMsg += " Unplanned page added to system";
				g_util.ErrorFilePrintf(sErrorLogPath, "Unplanned page added to system");
			}

			// Name is A-OK. Move to storage directory

			CString sDestPath = g_prefs.FormCcdataFileNameHires(&Job);

			CString sThisFullInputPath = sFullInputPath;

			int nPageFormatID = g_prefs.GetPageFormatIDFromPublication(m_pollDB, Job.m_publicationID, g_prefs.m_reloadpageformatdetails);


			BOOL bFileMoveOK = TRUE;
			CString sErr = _T("");
			BOOL bDoesExistAlready = g_util.FileExist(sDestPath);

			// Caclulate CRC before moving to CCDATA
			int nCRC = 0;
			int nRetries = 10;
			while (nCRC == 0 && nRetries-- > 0) {
				nCRC = g_util.CRC32Poll(sThisFullInputPath);
				if (nCRC == 0)
					::Sleep(500);
			}
			g_util.Logprintf("INFO: CRC = %d ..", nCRC);
		
			// File is still not moved to CCFiles...

			if (bFileMoveOK) {

				if (g_util.Reconnect(_T("\\\\") + g_prefs.m_serverName + _T("\\") + g_prefs.m_serverShare + _T("\\CCFiles"),
					g_prefs.m_mainserverusecussrentuser ? "" : g_prefs.m_mainserverusername,
					g_prefs.m_mainserverusecussrentuser ? "" : g_prefs.m_mainserverpassword) == FALSE) {

					g_util.Logprintf("ERROR: Error connecting to main server share %s - %s", _T("\\\\") + g_prefs.m_serverName + _T("\\") + g_prefs.m_serverShare + _T("\\CCFiles"),
						_T("\\\\") + g_prefs.m_serverName + _T("\\") + g_prefs.m_serverShare + _T("\\CCFiles"), g_util.GetLastWin32Error());
					::Sleep(1000);

				}

				if (bHasCopiedFile == FALSE  && pInput->m_makecopy && pInput->m_copyfolder != _T("") ) {

					CString sCopyDestPath = pInput->m_copyfolder + _T("\\") + fi.sFileName; //util.GetFileName(pInput->m_useFTP == FALSE ? fi.sFileName : sFtpTempName);

					::CopyFile(sThisFullInputPath, sCopyDestPath, FALSE);
					g_util.Logprintf("INFO: Making copy of input file %s to %s", sThisFullInputPath, sCopyDestPath);
				}

				g_util.Logprintf("INFO:  Starting copy to CCFiles..");



				if (bFileMoveOK) {

					// Last check on file existence..
					if (g_util.FileExist(sThisFullInputPath) == FALSE) {
						sErr.Format("ERROR: File disappeared from input folder (%s)", sThisFullInputPath);
						g_util.Logprintf("ERROR: %s", sErr);
						g_util.ErrorFilePrintf(sErrorLogPath, "ERROR: File %s disappeared from input folder (masterset %d)", sThisFullInputPath, Job.m_mastercopyseparationset);
						bFileMoveOK = FALSE;
					}

					if (bFileMoveOK) {

						if (!::MoveFileEx(sThisFullInputPath, sDestPath, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
							g_util.Logprintf("ERROR: Attempt 1 of 2: Error Moving %s to storage %s (masterset %d) - %s", sThisFullInputPath, sDestPath, Job.m_mastercopyseparationset, g_util.GetLastWin32Error());
							g_util.ErrorFilePrintf(sErrorLogPath, "ERROR: Moving %s to storage %s - retrying.. (masterset %d)", sThisFullInputPath, sDestPath, Job.m_mastercopyseparationset);

							if (g_util.FileExist(sThisFullInputPath) == FALSE)
								g_util.Logprintf("ERROR: Input file %s is gone ?!??? - WHAT..", sThisFullInputPath);

							if (g_util.FileExist(sDestPath)) {
								TCHAR szOwner[512];
								strcpy(szOwner, "");
								g_util.GetOwner(sDestPath.GetBuffer(255), szOwner, 255);
								sDestPath.ReleaseBuffer();

								if (strlen(szOwner) > 0)
									g_util.Logprintf("ERROR: Existing file %s seems to be locked by %s", sDestPath, szOwner);
								CString sLogData = g_util.GetProcessOwner(sDestPath);
								if (sLogData != "")
									g_util.Logprintf("ERROR: Lock info for file %s : %s", sDestPath, sLogData);
							}

							if (g_util.FileExist(sDestPath)) {
								g_util.Logprintf("INFO: Attempting to force close file handle...");
								g_util.ForceHandlesClose(sDestPath);
							}

							// Error moving file to file storage - retry..

							::Sleep(1000);

							if (!::MoveFileEx(sThisFullInputPath, sDestPath, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
								BOOL bRepairFailed = TRUE;
								g_util.Logprintf("ERROR: Attempt 2 of 2: Error Moving %s to storage %s (masterset %d) - %s",sThisFullInputPath, sDestPath, Job.m_mastercopyseparationset, g_util.GetLastWin32Error());

								g_util.ErrorFilePrintf(sErrorLogPath, "ERROR: Moving %s to storage %s - failed! (masterset %d)", sThisFullInputPath, sDestPath, Job.m_mastercopyseparationset);
							
			
								sLastError = "Error moving file: " + sThisFullInputPath + " -> " + sDestPath;
								bFileMoveOK = FALSE;

								m_pollDB.UpdateService(SERVICESTATUS_HASERROR, fi.sFileName, sLastError,  sErrorMessage);
								m_pollDB.InsertLogEntry(EVENTCODE_POLLERROR, pInput->m_inputname, fi.sFileName, sLastError, Job.m_mastercopyseparationset, 0, 0, _T(""), sErrorMessage);
								m_pollDB.PlanLock(PLANLOCK_UNLOCK, nCurrentPlanLock, sPlanClientName, sPlanClientTime, sErrorMessage);

								if (pInput->m_sendackfile)
									g_util.IssueMessage(m_pollDB, Job, STATUSNUMBER_POLLINGERROR, pInput->m_ackfilefolder, 1036, sErrorMessage);
							}
						}
					}

					if (bFileMoveOK) {
						// Very last last check...
						if (g_util.FileExist(sDestPath))
							g_util.Logprintf("INFO:  File %s copied to storage location %s", sThisFullInputPath, sDestPath);
						else {
							g_util.Logprintf("ERROR: Final test of file in CCFiles failed! -  %s", sDestPath);
							bFileMoveOK = FALSE;
						}

						if (nCRC == 0) {
							nCRC = g_util.CRC32Poll(sThisFullInputPath);
							g_util.Logprintf("INFO: CRC = %d (after move to CCDATA)", nCRC);
						}
					}
					// Remove FILE_ATTRIBUTE_ARCHIVE		
					// ::SetFileAttributes(sDestPath, g_pProgressFormView->m_autoapprove ? (DWORD)FILE_ATTRIBUTE_ARCHIVE :(DWORD)0);
				}
			}

			if (g_prefs.m_calcCRCaftermove) {
				int nCRCAter = g_util.CRC32Poll(sDestPath);
				g_util.Logprintf("INFO: CRC after move to CCDATA = %u ..", nCRCAter);

				if (nCRC > 0 && nCRCAter > 0 && nCRCAter != nCRC) {
					g_util.Logprintf("ERROR: Recalculation of CRC failed comparison for file %s", sDestPath);
					bFileMoveOK = FALSE;
				}

				if (nCRC > 0 && nCRCAter > 0 && nCRCAter == nCRC)
					g_util.Logprintf("INFO: CRC check after move til CCDATA is OK");
			}

			////////////////////////////////////////
			// Report to database:
			//  Only report error if it is a file move error
			////////////////////////////////////////

			CTime t = CTime::GetCurrentTime();
			if (bFileMoveOK == FALSE) {
				CString sOrgName = sNameBeforeTranslated;
				if (sTranslatedName != sOrgName)
					sOrgName += " (" + sTranslatedName + ")";

				// Move file to garbage folder
				sErrorPath.Format("%s%d\\%s", g_prefs.m_errorPath, pInput->m_ID, sNameBeforeTranslated);
				::MoveFileEx(sFullInputPath, sErrorPath, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);

				// RESET STATUS!

				if (Job.m_previousstatus == 5 || Job.m_previousstatus == 6 || Job.m_previousstatus == 1)
					Job.m_previousstatus = 0;

				int dbStat = m_pollDB.UpdateStatusInput(pInput->m_ID, Job.m_mastercopyseparationset, pInput->m_inputtype, Job.m_previousstatus, "!",
															sNameBeforeTranslated, Job.m_comment,REAPPROVEUPDATES_NOCHANGE, sErrorMessage);
				if (dbStat < 0) {
					g_util.Logprintf("ERROR: m_pollDB.UpdateStatusInput() - %s ", sErrorMessage);

					bReConnect = TRUE;
					if (bDoUnlock)
						m_pollDB.PublicationPlanLock(Job.m_publicationID, Job.m_pubdate, Job.m_auxpubdate, PLANLOCK_UNLOCK, nCurrentLock, sLockClientName, sLockClientTime, sErrorMessage);
					break;
				}
				else if (dbStat == 0) {
					g_util.Logprintf("ERROR: Unable to reset status to %d for MasterSeparationSet %d (%s) - (SP returned 0) ", Job.m_previousstatus, Job.m_mastercopyseparationset, sNameBeforeTranslated);
				}
				else {
					g_util.Logprintf("INFO:  Successfully set status back to %d for MasterSeparationSet %d (%s)", Job.m_previousstatus, Job.m_mastercopyseparationset, sNameBeforeTranslated);
				}

				m_pollDB.DecreaseVersion(Job.m_mastercopyseparationset, sErrorMessage);

				sLastError = _T("File moved to error folder - ") + sNameBeforeTranslated;
				m_pollDB.UpdateService(SERVICESTATUS_HASERROR, fi.sFileName, sLastError, sErrorMessage);
				m_pollDB.InsertLogEntry(EVENTCODE_POLLERROR, pInput->m_inputname, fi.sFileName, sLastError, Job.m_mastercopyseparationset, 0, 0, _T(""), sErrorMessage);

				if (bDoUnlock)
					m_pollDB.PublicationPlanLock(Job.m_publicationID, Job.m_pubdate, Job.m_auxpubdate, PLANLOCK_UNLOCK, nCurrentLock, sLockClientName, sLockClientTime, sErrorMessage);
				continue;
			}

			////////////////////////////////////////////
			// At this stage the file is moved to CCfiles OK
			////////////////////////////////////////////


			::DeleteFile(sFullInputPath);
			CString sString = _T("");
			// We survived so far - report success
			if (nVersionFromFileName == 0)
				sString = bDoesExistAlready ? "Version update" : "Initial version";
			else
				sString = nVersionFromFileName == Job.m_version ? "Version update" : "Initial version";

			if (dbAccessStat == POLLDBSTATUS_SUBEDITIONPAGEADDED)
				sString += _T(" - Edition page added to system");

			if (dbAccessStat == POLLDBSTATUS_ISSUEPAGEADDED)
				sString += _T(" - Issue added to system");

			
			if (bFileMoveOK && g_prefs.m_callcustomSP) {
				int nCustomFlag = 0;

				if (m_pollDB.CallCustomSP(Job.m_mastercopyseparationset, Job.m_copyseparationset, nCustomFlag, &Job, sErrorMessage) == FALSE)
					g_util.Logprintf("ERROR: m_pollDB.CallCustomSP() - %s ", sErrorMessage);
			}

			if (bFileMoveOK && nVersionFromFileName > 0) {
				if (m_pollDB.UpdateVersion(Job.m_mastercopyseparationset, nVersionFromFileName, sErrorMessage) == FALSE) {
					g_util.Logprintf("ERROR: m_pollDB.UpdateVersion() - %s ", sErrorMessage);
					bReConnect = TRUE;
				}
			}

			if (m_pollDB.UpdateCRC(Job.m_mastercopyseparationset,  pInput->m_inputtype, nCRC, sErrorMessage) == FALSE) {
				g_util.Logprintf("ERROR: UpdateCRC() failed - %s", sErrorMessage);
				bReConnect = TRUE;
			}

			////////////////////////////////////
			// Update to POLLED
			////////////////////////////////////

			// Special case

			dbAccessStat = m_pollDB.UpdateStatusInput(pInput->m_ID, Job.m_mastercopyseparationset, 
													pInput->m_inputtype , STATUSNUMBER_TRANSMITTED, _T(""), sNameBeforeTranslated,  Job.m_comment, 
													pInput->m_reapproveaction, sErrorMessage);
			if (dbAccessStat > 0) {
				g_util.Logprintf("INFO: '%s' status updated for mastercopyseparationset %d", sNameBeforeTranslated, Job.m_mastercopyseparationset);

				if (pInput->m_sendackfile)
					g_util.IssueMessage(m_pollDB, Job, STATUSNUMBER_TRANSMITTED, pInput->m_ackfilefolder, pInput->m_ackflagvalue, sErrorMessage);
				if (g_prefs.m_updatemiscstring2 && Job.m_miscstring2 != "")
					// We don't care about errors here....
					m_pollDB.UpdateMiscString2(Job.m_mastercopyseparationset, Job.m_miscstring2, sErrorMessage);
				if (g_prefs.m_updatemiscstring3 && Job.m_miscstring3 != "")
					// We don't care about errors here....
					m_pollDB.UpdateMiscString3(Job.m_mastercopyseparationset, Job.m_miscstring3, sErrorMessage);

				if (g_prefs.m_updatereadytime) {
					// We don't care about errors here....
					m_pollDB.UpdateReadyTime(Job.m_mastercopyseparationset,  sErrorMessage);
				}

			}
			else if (dbAccessStat == 0) {
				g_util.Logprintf("ERROR: '%s' Unable to set status for mastercopyseparationset %d - SP returned 0", sNameBeforeTranslated, Job.m_mastercopyseparationset);
			}
			else {
				bReConnect = TRUE;
				g_util.Logprintf("ERROR: '%s' Unable to set status for mastercopyseparationset %d - database error %s", sNameBeforeTranslated, Job.m_mastercopyseparationset, szMessage);
				dbAccessStat = FALSE;
			}

			if (bOverrideInputTime > 0) {
				if (!m_pollDB.UpdateInputTime(Job.m_mastercopyseparationset, tOverrideInputTime, sErrorMessage)) {
					g_util.Logprintf("ERROR: UpdateInputTime() failed - %s", sErrorMessage);
					bReConnect = TRUE;
				}
			}

			// Delete error log file - we will not use it
			::DeleteFile(sErrorLogPath);

			////////////////////////////////////
			// Status is now updated to POLLED
			////////////////////////////////////

			if (pInput->m_overruleapproval) {
				if (m_pollDB.UpdateApproval(Job.m_mastercopyseparationset,
											pInput->m_overruledapproval_approvalrequired ? APPROVAL_NOTAPPROVED : APPROVAL_AUTO,
											pInput->m_overruledapproval_approvalrequired ? "InputCenter Approval Required" : "InputService AutoApprove", sErrorMessage) == FALSE) {
					g_util.Logprintf("ERROR: UpdateApproval() failed - %s", sErrorMessage);
					bReConnect = TRUE;
				}
				Job.m_approved = pInput->m_overruledapproval_approvalrequired ? APPROVAL_AUTO : APPROVAL_APPROVED;
			}

			if (bPlateAdded)
				sString = "Sub-edition plate added to plan";

			int nState = (Job.m_approved == APPROVAL_APPROVED || Job.m_approved == APPROVAL_AUTO) ? STATE_RELEASED : STATE_LOCKED;

	
			if (pInput->m_overrulerelease) {
				if (!m_pollDB.UpdateRelease(Job.m_mastercopyseparationset, pInput->m_overruledrelease_putonhold, sErrorMessage)) {
					g_util.Logprintf("ERROR: UpdateRelease() failed - %s", sErrorMessage);
					bReConnect = TRUE;
				}
				Job.m_hold = pInput->m_overruledrelease_putonhold;
			}

			m_pollDB.UpdateService(SERVICESTATUS_RUNNING, fi.sFileName, _T(""), sErrorMessage);
			m_pollDB.InsertLogEntry(EVENTCODE_POLLED, pInput->m_inputname, fi.sFileName, sString + _T(" (") + g_util.Int2String(Job.m_mastercopyseparationset) + _T(")"), Job.m_mastercopyseparationset, nVersionFromFileName > 0 ? nVersionFromFileName - 1 : Job.m_version, 0, _T(""), sErrorMessage);

			if (bDoUnlock)
				m_pollDB.PublicationPlanLock(Job.m_publicationID, Job.m_pubdate, Job.m_auxpubdate, PLANLOCK_UNLOCK, nCurrentLock, sLockClientName, sLockClientTime, sErrorMessage);

			if (bReConnect)
				break;
		}
		

	} while (g_BreakSignal == FALSE);

	m_pollDB.UpdateService(SERVICESTATUS_STOPPED, _T(""), _T(""), sErrorMessage);
	m_pollDB.ExitDB();

	g_util.Logprintf("INFO:  Polling stopped.");

	return;
}