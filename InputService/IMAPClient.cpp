#include "StdAfx.h"
#include "Defs.h"
#include "Utils.h"
#include "EmailClient.h"
#include "IMAPClient.h"
#include <string.h>
#include <direct.h>
#include <winnetwk.h>
#include <CkEmail.h>
#include <CkEmailBundle.h>
#include <CkMailMan.h>
#include <CkStringArray.h>
#include <CkImap.h>

CIMAPClient::CIMAPClient(void)
{
	m_IMAPconnectiontimeout = 60;
	m_IMAPuseSSL = FALSE;
	m_proxyHTTPHost = _T("");
	m_proxyHTTPPort = 0;
	m_IMAPMailbox = _T("Inbox");
}

CIMAPClient::~CIMAPClient(void)
{
}

void CIMAPClient::SetIMAPConnectoinTimeout(int timeout)
{
	m_IMAPconnectiontimeout = timeout;
}

void CIMAPClient::SetIMAPSSLmode(BOOL useSSL)
{
	m_IMAPuseSSL = useSSL;
}


void CIMAPClient::SetIMAPMailBox(CString sMailBox)
{
	if (sMailBox != _T(""))
		m_IMAPMailbox = sMailBox;
}


void CIMAPClient::HTTPProxySettings(CString sProxyHTTPHostname, int nProxyHTTPPort)
{
	m_proxyHTTPHost = sProxyHTTPHostname;
	m_proxyHTTPPort = nProxyHTTPPort;
}

BOOL CIMAPClient::TestIMAPConnection(CString sIMAPServer, int nIMAPPort, CString sIMAPUsername, CString sIMAPPassword, TCHAR *szErrorMessage)
{
	CkImap   mailman;
	CUtils util;
	CkString sMsg;

	//bool success = mailman.UnlockComponent("INFRAL.CB1092019_4czEsHZS615x");
	bool success = mailman.UnlockComponent("NFRLGC.CB1112020_vZdFLucd1RAZ");

	if (success != true) {
		mailman.LastErrorText(sMsg);
		util.Logprintf("IMAP ERROR: Error initializing IMAP component - %s",(LPCSTR)sMsg);
		_stprintf(szErrorMessage, "Error initializing IMAP component - %s", (LPCSTR)sIMAPServer);
		return FALSE;
	}

	if (m_IMAPconnectiontimeout > 0)
		mailman.put_ConnectTimeout(m_IMAPconnectiontimeout);

	if (m_proxyHTTPHost != _T("") && m_proxyHTTPPort > 0) {
		mailman.put_HttpProxyHostname(m_proxyHTTPHost);
		mailman.put_HttpProxyPort(m_proxyHTTPPort);
	}

	if (m_IMAPuseSSL)
		mailman.put_Ssl(true);

	if (nIMAPPort > 0)
		mailman.put_Port(nIMAPPort);

	success = mailman.Connect(sIMAPServer);
	if (success != true) {
		mailman.LastErrorText(sMsg);
		util.Logprintf("IMAP ERROR: Error connecting to server %s - %s", (LPCSTR)sIMAPServer, (LPCSTR)sMsg);
		_stprintf(szErrorMessage, "Error connecting to IMAP server %s", (LPCSTR)sIMAPServer);
		return FALSE;
	}

	success = mailman.Login(sIMAPUsername, sIMAPPassword);
	if (success != true) {
		mailman.LastErrorText(sMsg);
		util.Logprintf("IMAP ERROR: Error logging in to server %s - %s", (LPCSTR)sIMAPServer, (LPCSTR)sMsg);
		_stprintf(szErrorMessage, "Error logging in to IMAP server %s", (LPCSTR)sIMAPServer);
		return FALSE;
	}

	success = mailman.SelectMailbox(m_IMAPMailbox);
	if (success != true)
	{
		mailman.LastErrorText(sMsg);
		_stprintf(szErrorMessage, "IMAP ERROR: Error selecting mailbox %s in to server %s - %s", (LPCSTR)m_IMAPMailbox, (LPCSTR)sIMAPServer, (LPCSTR)sMsg);
		return false;
	}

	int numMessages = mailman.get_NumMessages();

	if (numMessages == -1) {
		mailman.LastErrorText(sMsg);
		util.Logprintf("IMAP ERROR: Error getting mail count - %s", (LPCSTR)sMsg);
		_stprintf(szErrorMessage, "Error getting mail count");
	}
	mailman.Disconnect();

	return numMessages != -1 ? TRUE : FALSE;
}


int CIMAPClient::GetMailAttachments(CString sIMAPServer, int nIMAPPort, CString sIMAPUsername, CString sIMAPPassword, CString sEmailFilter, BOOL bDeleteAfter,
	CString sDestFolder, EMAILFILEINFOSTRUCT aFileList[], int nMaxFiles, TCHAR *szErrorMessage)
{
	CUtils util;
	int nAttachments = 0;
	_tcscpy(szErrorMessage, _T(""));

	CkImap   mailman;
	CkString sMsg;

	CStringArray aEmailList;
	aEmailList.RemoveAll();
	if (sEmailFilter.Trim() != _T(""))
		util.StringSplitter(sEmailFilter.Trim(), ",", aEmailList);

	//bool success = mailman.UnlockComponent("INFRAL.CB1092019_4czEsHZS615x");
	bool success = mailman.UnlockComponent("NFRLGC.CB1112020_vZdFLucd1RAZ");
	if (success != true) {
		mailman.LastErrorText(sMsg);
		util.Logprintf("IMAP ERROR: Error initializing IMAP component - %s", (LPCSTR)sMsg);
		return -1;
	}

	if (m_IMAPconnectiontimeout > 0)
		mailman.put_ConnectTimeout(m_IMAPconnectiontimeout);

	if (m_proxyHTTPHost != _T("") && m_proxyHTTPPort > 0) {
		mailman.put_HttpProxyHostname(m_proxyHTTPHost);
		mailman.put_HttpProxyPort(m_proxyHTTPPort);
	}

	if (m_IMAPuseSSL)
		mailman.put_Ssl(true);

	if (nIMAPPort > 0 )
		mailman.put_Port(nIMAPPort);

	success = mailman.Connect(sIMAPServer);
	if (success != true) {
		mailman.LastErrorText(sMsg);
		util.Logprintf("IMAP ERROR: Error connecting to server %s - %s", (LPCSTR)sIMAPServer, (LPCSTR)sMsg);
		_stprintf(szErrorMessage, "Error connecting to IMAP server %s", (LPCSTR)sIMAPServer);
		return -1;
	}

	success = mailman.Login(sIMAPUsername, sIMAPPassword);
	if (success != true) {
		mailman.LastErrorText(sMsg);
		util.Logprintf("IMAP ERROR: Error logging in to server %s - %s", (LPCSTR)sIMAPServer, (LPCSTR)sMsg);
		_stprintf(szErrorMessage, "Error logging in to IMAP server %s", (LPCSTR)sIMAPServer);
		return -1;
	}

	success = mailman.SelectMailbox(m_IMAPMailbox);
	if (success != true)
	{
		mailman.LastErrorText(sMsg);
		_stprintf(szErrorMessage, "IMAP ERROR: Error selecting mailbox %s in to server %s - %s", (LPCSTR)m_IMAPMailbox, (LPCSTR)sIMAPServer, (LPCSTR)sMsg);
		mailman.Disconnect(); 
		return -1;
	}
	
	CkMessageSet *messageSet = mailman.Search("ALL", true);

	if (messageSet == NULL) {
		mailman.LastErrorText(sMsg);
		_stprintf(szErrorMessage, "IMAP ERROR: Error getting mail count - %s", (LPCSTR)sMsg);
		mailman.Disconnect();
		return -1;
	}

	//  Fetch the emails into a bundle object:

	CkEmailBundle *bundle = mailman.FetchBundle(*messageSet);
	if (bundle == NULL)
	{
		mailman.LastErrorText(sMsg);
		_stprintf(szErrorMessage, "IMAP ERROR: Error getting messages (FetchBundle) in mailbox %s in to server %s - %s", (LPCSTR)m_IMAPMailbox, (LPCSTR)sIMAPServer, (LPCSTR)sMsg);
		mailman.Disconnect();
		return -1;
	}

	CkString sUIDL;
	CkEmail *email = 0;

	int nEmails = bundle->get_MessageCount();

	for (int i = 0; i <= nEmails - 1; i++) {

		email = bundle->GetEmail(i);

		int n = email->get_NumAttachments();

		//  You may also save individual attachments:
		email->get_Uidl(sUIDL);

		CString sFrom = email->ck_from();

		BOOL bPassed = FALSE;
		if (aEmailList.GetCount() > 0) {
			for (int em = 0; em < aEmailList.GetCount(); em++) {
				CString ss = aEmailList[em].Trim();
				if (ss.CompareNoCase(sFrom) != -1) {
					bPassed = TRUE;
					break;
				}
			}
		}
		else
			bPassed = TRUE;

		if (bPassed) {
			for (int j = 0; j <= n - 1; j++) {

				email->put_OverwriteExisting(false);
				CString sFileName = email->getAttachmentFilename(j);
				if (email->SaveAttachedFile(j, sDestFolder) == false) {
					util.Logprintf("IMAP ERROR: Error saving attachment %s to folder %s - %s", (LPCSTR)sFileName, (LPCSTR)sDestFolder, (LPCSTR)mailman.lastErrorText());
					continue;
				}
				sFileName = email->getAttachmentFilename(j);
				if (util.FileExist(sDestFolder + _T("\\") + sFileName)) {
					CString sNewFilename = util.GetFileName(sFileName, TRUE) + _T("_") + GenerateTimeStamp() + _T(".") + util.GetExtension(sFileName);
					::MoveFileEx(sDestFolder + _T("\\") + sFileName, sDestFolder + _T("\\") + sNewFilename, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
					aFileList[nAttachments].sFileName = sNewFilename;
					aFileList[nAttachments].nFileSize = util.GetFileSize(sDestFolder + _T("\\") + sNewFilename);
					aFileList[nAttachments].tJobTime = CTime::GetCurrentTime();
					aFileList[nAttachments].tWriteTime = CTime::GetCurrentTime();
					aFileList[nAttachments].sUIDL = sUIDL.getString();
					nAttachments++;
				}
			}

			if (bDeleteAfter)
				success = mailman.SetMailFlag(*email, "Deleted", 1);
		}
		delete email;
	}
	delete bundle;
	delete messageSet;
	if (bDeleteAfter)
		success = mailman.ExpungeAndClose();

	mailman.Disconnect();

	return nAttachments;
}


BOOL CIMAPClient::DeleteIMAPEmail(CString sIMAPServer, int nIMAPPort, CString sIMAPUsername, CString sIMAPPassword, CString uidl, TCHAR *szErrorMessage)
{
	CUtils util;
	int nAttachments = 0;
	_tcscpy(szErrorMessage, _T(""));

	CkImap   mailman;
	CkString sMsg;

//	bool success = mailman.UnlockComponent("INFRAL.CB1092019_4czEsHZS615x");
	bool success = mailman.UnlockComponent("NFRLGC.CB1112020_vZdFLucd1RAZ");

	if (success != true) {
		mailman.LastErrorText(sMsg);
		util.Logprintf("IMAP ERROR: Error initializing IMAP compoment - %s", (LPCSTR)sMsg);
		return FALSE;
	}

	if (m_IMAPconnectiontimeout > 0)
		mailman.put_ConnectTimeout(m_IMAPconnectiontimeout);

	if (m_proxyHTTPHost != _T("") && m_proxyHTTPPort > 0) {
		mailman.put_HttpProxyHostname(m_proxyHTTPHost);
		mailman.put_HttpProxyPort(m_proxyHTTPPort);
	}

	if (m_IMAPuseSSL)
		mailman.put_Ssl(true);

	if (nIMAPPort > 0)
		mailman.put_Port(nIMAPPort);

	success = mailman.Connect(sIMAPServer);
	if (success != true) {
		mailman.LastErrorText(sMsg);
		util.Logprintf("IMAP ERROR: Error connecting to server %s - %s", (LPCTSTR)sIMAPServer, (LPCTSTR)sMsg);
		_stprintf(szErrorMessage, "Error connecting to IMAP server %s", (LPCTSTR)sIMAPServer);
		return FALSE;
	}

	success = mailman.Login(sIMAPUsername, sIMAPPassword);
	if (success != true) {
		mailman.LastErrorText(sMsg);
		util.Logprintf("IMAP ERROR: Error logging in to server %s - %s", (LPCTSTR)sIMAPServer, (LPCTSTR)sMsg);
		_stprintf(szErrorMessage, "Error logging in to IMAP server %s", (LPCTSTR)sIMAPServer);
		return FALSE;
	}

	success = mailman.SelectMailbox(m_IMAPMailbox);
	if (success != true)
	{
		mailman.LastErrorText(sMsg);
		_stprintf(szErrorMessage, "IMAP ERROR: Error selecting mailbox %s in to server %s - %s", (LPCTSTR)m_IMAPMailbox, (LPCTSTR)sIMAPServer, (LPCTSTR)sMsg);
		mailman.Disconnect();
		return FALSE;
	}

	CkMessageSet *messageSet = mailman.Search("ALL", true);

	if (messageSet == NULL) {
		mailman.LastErrorText(sMsg);
		_stprintf(szErrorMessage, "IMAP ERROR: Error getting mail count - %s", (LPCTSTR)sMsg);
		mailman.Disconnect();
		return FALSE;
	}

	//  Fetch the emails into a bundle object:

	CkEmailBundle *bundle = mailman.FetchBundle(*messageSet);
	if (bundle == NULL)
	{
		mailman.LastErrorText(sMsg);
		_stprintf(szErrorMessage, "IMAP ERROR: Error getting messages (FetchBundle) in mailbox %s in to server %s - %s", (LPCTSTR)m_IMAPMailbox, (LPCTSTR)sIMAPServer, (LPCTSTR)sMsg);
		mailman.Disconnect();
		return FALSE;
	}

	CkString sUIDL;
	CString sUIDLx;
	CkEmail *email = 0;

	int nEmails = bundle->get_MessageCount();

	for (int i = 0; i <= nEmails - 1; i++) {

		email = bundle->GetEmail(i);
		if (email == NULL)
			continue;
		email->get_Uidl(sUIDL);
		sUIDLx = sUIDL.getUnicode();
		if (sUIDLx == uidl)
		{
			success = mailman.SetMailFlag(*email, "Deleted", 1);
			break;
		}
	}

	success = mailman.ExpungeAndClose();
	if (success != true)
	{
		mailman.LastErrorText(sMsg);
		_stprintf(szErrorMessage, "IMAP ERROR: Error getting mail count - %s", (LPCSTR)sMsg);
		mailman.Disconnect();
		return FALSE;
	} 

	return TRUE;
}

CString CIMAPClient::GenerateTimeStamp()
{
	SYSTEMTIME ltime;
	CString sTimeStamp;

	::GetLocalTime(&ltime);
	sTimeStamp.Format(_T("%.4d%.2d%.2d%.2d%.2d%.2d%.3d"), (int)ltime.wYear, (int)ltime.wMonth, (int)ltime.wDay, (int)ltime.wHour, (int)ltime.wMinute, (int)ltime.wSecond, (int)ltime.wMilliseconds);

	return sTimeStamp;
}

