#pragma once
#include "stdafx.h"
#include "EmailClient.h"

class CIMAPClient
{
public:
	CIMAPClient(void);
	~CIMAPClient(void);

	int		m_IMAPconnectiontimeout;
	BOOL    m_IMAPuseSSL;

	CString m_proxyHTTPHost;
	int		m_proxyHTTPPort;

	CString m_IMAPMailbox;

	void	SetIMAPConnectoinTimeout(int timeout);
	void	SetIMAPSSLmode(BOOL useSSL);
	void	SetIMAPMailBox(CString sMailBox);

	void	HTTPProxySettings(CString sProxyHTTPHostname, int nProxyHTTPPort);


	BOOL	TestIMAPConnection(CString sIMAPServer, int nIMAPPort, CString sIMAPUsername, CString sIMAPPassword, TCHAR *szErrorMessage);
	int		GetMailAttachments(CString sIMAPServer, int nIMAPPort, CString sIMAPUsername, CString sIMAPPassword, CString sEmailFilter, BOOL bDeleteAfter,
		CString sDestFolder, EMAILFILEINFOSTRUCT aFileList[], int nMaxFiles, TCHAR *szErrorMessage);

	CString GenerateTimeStamp();
	BOOL DeleteIMAPEmail(CString sIMAPServer, int nIMAPPort, CString sIMAPUsername, CString sIMAPPassword, CString uidl, TCHAR *szErrorMessage);

};

