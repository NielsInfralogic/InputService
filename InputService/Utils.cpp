#include "StdAfx.h"
#include "Defs.h"
#include "Utils.h"
#include "Tiffio.h"
#include "Ping.h"
#include "Prefs.h"
#include "EmailClient.h"
#include "IMAPClient.h"
#include <string.h> 
#include <direct.h>
#include <winnetwk.h>
#include <CkCrypt2.h>


#include <boost/regex.hpp>
#include <boost/regex/v4/fileiter.hpp>

# ifdef BOOST_MSVC
#  pragma warning(disable: 4244 4267)
#endif

extern CPrefs g_prefs;

extern DATEOFWEEK pDateOfWeek;


CUtils::CUtils(void)
{
}

CUtils::~CUtils(void)
{
}

CString CUtils::Int2String(int n)
{
	CString s;
	s.Format("%d", n);
	return s;
}

CString CUtils::UInt2String(int un)
{
	CString s;
	s.Format("%u", un);
	return s;
}

CString CUtils::LimitErrorMessage(CString s)
{
	if (s.GetLength() >= MAX_ERRMSG)
		return s.Left(MAX_ERRMSG - 1);

	return s;
}

CString CUtils::LimitErrorMessage(TCHAR *szStr)
{
	CString s(szStr);
	return LimitErrorMessage(s);
}



int CUtils::CRC32(CString sFileName)
{
	return CRC32(sFileName, FALSE);
}

int CUtils::CRC32(CString sFileName, BOOL bFastMode)
{
	HANDLE	Hndl;
	BYTE	buf[8192];
	DWORD	nBytesRead;

	if ((Hndl = ::CreateFile(sFileName, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0)) == INVALID_HANDLE_VALUE)
		return 0;
	
	DWORD sum = 0;
	do {
		if (!::ReadFile(Hndl, buf, 8092, &nBytesRead, NULL)) {
			::CloseHandle(Hndl);
			return 0;
		}
		if (nBytesRead > 0) {
			for (int i=0; i<(int)nBytesRead; i++) {
				sum += buf[i];
				// just let it overflow..
			}
		}

		// one block only in fast mode...
		if (bFastMode)
			break;

	}
	while (nBytesRead > 0);

	::CloseHandle(Hndl);

	int nCRC = sum;
	return nCRC;
}


int CUtils::StringSplitter(CString sInput, CString sSeparators, CStringArray &sArr)
{
	sArr.RemoveAll();

	int nElements = 0;
	CString s = sInput;
	s.Trim();

	int v = s.FindOneOf(sSeparators);
	BOOL isLast = FALSE;
	do {
		nElements++;
		if (v != -1) 
			sArr.Add(s.Left(v));	
		else {
			sArr.Add(s);
			isLast = TRUE;
		}
		if (isLast == FALSE) {
			s = s.Mid(v+1);
			v = s.FindOneOf(sSeparators);
		}
	} while (isLast == FALSE);

	return nElements;
}

CString CUtils::LoadFile(CString sFilename)
{
	DWORD			BytesRead;
	TCHAR			readbuffer[4097];
	CString s = "";

	HANDLE			Hndl = CreateFile(sFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);

	if (Hndl == INVALID_HANDLE_VALUE)
		return "";

	if (ReadFile(Hndl, readbuffer, 4096, &BytesRead, NULL)) {
		CloseHandle(Hndl);
		readbuffer[BytesRead] = 0;
		s = readbuffer;
		return s;
	} 
	
	CloseHandle(Hndl);

	return "";
}

BOOL CUtils::LockCheck(CString sFileToLock)
{
	if (g_prefs.m_lockcheckmode == LOCKCHECKMODE_NONE)
		return TRUE;

	// Appempt to lock and read from  file
	OVERLAPPED		Overlapped;
	DWORD			BytesRead, dwSizeHigh;
	TCHAR			readbuffer[4097];
	HANDLE			Hndl;

	if (g_prefs.m_lockcheckmode == LOCKCHECKMODE_READ)
		Hndl = CreateFile(sFileToLock, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
	else
		Hndl = CreateFile(sFileToLock, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);

	if (Hndl == INVALID_HANDLE_VALUE)
		return FALSE;

	if (g_prefs.m_lockcheckmode == LOCKCHECKMODE_RANGELOCK) {
		DWORD dwSizeLow = ::GetFileSize (Hndl, &dwSizeHigh);
		if (dwSizeLow == -1) {
			CloseHandle(Hndl);
			return FALSE;
		}

		if (!LockFileEx(Hndl, LOCKFILE_FAIL_IMMEDIATELY | LOCKFILE_EXCLUSIVE_LOCK, 0, dwSizeLow, dwSizeHigh, &Overlapped)) {
			CloseHandle(Hndl);
			return FALSE;
		} 
		
		UnlockFileEx(Hndl, 0, dwSizeLow, dwSizeHigh, &Overlapped);
	}

	if (ReadFile(Hndl, readbuffer, 4096, &BytesRead, NULL)) {
		CloseHandle(Hndl);
		return TRUE;
	} 
	
	CloseHandle(Hndl);
	return FALSE;	
}


BOOL CUtils::StableTimeCheck(CString sFileToTest, int nStableTimeSec)
{
	HANDLE	Hndl;
	FILETIME WriteTime, LastWriteTime;
	DWORD dwFileSize,dwLastFileSize;

	if (nStableTimeSec == 0)
		return TRUE;

	Hndl = ::CreateFile(sFileToTest, GENERIC_READ , 0 /*  FILE_SHARE_READ*/, NULL, OPEN_EXISTING, 0, 0);
	if (Hndl == INVALID_HANDLE_VALUE)
		return FALSE;
	::GetFileTime( Hndl, NULL, NULL, &WriteTime);
	::GetFileSize(Hndl, &dwFileSize);
	
	::CloseHandle(Hndl);
	
	::Sleep(nStableTimeSec*1000);

	Hndl = CreateFile(sFileToTest, GENERIC_READ , 0 /*  FILE_SHARE_READ*/, NULL, OPEN_EXISTING, 0, 0);
	if (Hndl == INVALID_HANDLE_VALUE)
		return FALSE;
	GetFileTime( Hndl, NULL, NULL, &LastWriteTime);
	::GetFileSize(Hndl, &dwLastFileSize);
	
	CloseHandle(Hndl);
	
	return (WriteTime.dwLowDateTime == LastWriteTime.dwLowDateTime && WriteTime.dwHighDateTime == LastWriteTime.dwHighDateTime && dwFileSize == dwLastFileSize );
}

DWORD CUtils::GetFileSize(CString sFile)
{
	DWORD	dwFileSizeLow = 0, dwFileSizeHigh = 0;

	HANDLE	Hndl = ::CreateFile(sFile, GENERIC_READ ,0 /*  FILE_SHARE_READ*/, NULL, OPEN_EXISTING, 0, 0);
	if (Hndl == INVALID_HANDLE_VALUE)
		return 0;

	dwFileSizeLow = ::GetFileSize(Hndl, &dwFileSizeHigh);
	::CloseHandle(Hndl);

	return dwFileSizeLow;
}


CTime CUtils::GetWriteTime(CString sFile)
{
	FILETIME WriteTime, LocalWriteTime;
	SYSTEMTIME SysTime;
	CTime t(1975,1,1,0,0,0);
	BOOL ok = TRUE;

	HANDLE Hndl = ::CreateFile(sFile, GENERIC_READ , 0 /*  FILE_SHARE_READ*/, NULL, OPEN_EXISTING, 0, 0);
	if (Hndl == INVALID_HANDLE_VALUE)
		return t;

	ok = ::GetFileTime( Hndl, NULL, NULL, &WriteTime);

	CloseHandle(Hndl);
	if (ok) {
		::FileTimeToLocalFileTime( &WriteTime, &LocalWriteTime );
		::FileTimeToSystemTime(&LocalWriteTime, &SysTime);
		CTime t2((int)SysTime.wYear,(int)SysTime.wMonth,(int)SysTime.wDay,(int)SysTime.wHour,(int)SysTime.wMinute, (int)SysTime.wSecond); 
		t = t2;
	}

	return t;
}

BOOL CUtils::FileExist(CString sFile)
{
	HANDLE hFind;
	WIN32_FIND_DATA  ff32;
	BOOL ret = FALSE;

	hFind = ::FindFirstFile(sFile, &ff32);
	if (hFind != INVALID_HANDLE_VALUE) {
		if (!(ff32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			ret = TRUE;
	}

	FindClose(hFind);
	return ret;
}

BOOL CUtils::CheckFolder(CString sFolder)
{
	TCHAR	szCurrentDir[_MAX_PATH];
	BOOL	ret = TRUE;

	::GetCurrentDirectory(_MAX_PATH, szCurrentDir);

	if (_tchdir(sFolder.GetBuffer(255)) == -1) 
		ret = FALSE;

	sFolder.ReleaseBuffer();

	::SetCurrentDirectory(szCurrentDir);
	
	return ret;
}

BOOL CUtils::CheckFolderWithPing(CString sFolder)
{
	TCHAR	szCurrentDir[_MAX_PATH];
	BOOL	ret = TRUE;
	CString sServerName = "";

	if (g_prefs.m_bypasspingtest)
		return CheckFolder(sFolder);

	// Resolve mapped drive connection
	if (sFolder.Mid(1,1) == _T(":")) {
		TCHAR szRemoteName[MAX_PATH];
		DWORD lpnLength = MAX_PATH;

		if (::WNetGetConnection(sFolder.Mid(0,2), szRemoteName, &lpnLength) == NO_ERROR)
			sFolder = szRemoteName;
	}


	if (sFolder.Mid(0,2) == _T("\\\\") )
		sServerName = sFolder.Mid(2);

	int n = sServerName.Find("\\");
	if (n != -1)
		sServerName = sServerName.Left(n);

	 BOOL 	   bSuccess = FALSE;
	if (sServerName != "") {
		WSADATA wsa;
		if (WSAStartup(MAKEWORD(2, 0), &wsa) != 0)
			return FALSE;

		CPing p;
		CPingReply pr;
		bSuccess = p.PingUsingWinsock(sServerName, pr, 30, 1000, 32, 0, FALSE);
		WSACleanup();
	}

	if (sServerName != "" && bSuccess == FALSE)
		return FALSE;

	::GetCurrentDirectory(_MAX_PATH, szCurrentDir);

	if ((_tchdir(sFolder.GetBuffer(255))) == -1) 
		ret = FALSE;

	sFolder.ReleaseBuffer();

	::SetCurrentDirectory(szCurrentDir);
	
	return ret;
}



BOOL CUtils::Reconnect(CString sFolder, CString sUser, CString sPW)
{
	if (g_prefs.m_bypassreconnect)
		return TRUE;

	//if (CheckFolderWithPing(sFolder) == FALSE)
	//	return FALSE;
	NETRESOURCE nr; 
	nr.dwScope = NULL; 
	nr.dwDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
	nr.dwUsage = RESOURCEDISPLAYTYPE_GENERIC;  //RESOURCEUSAGE_CONNECTABLE 
	nr.dwType = RESOURCETYPE_DISK;
	nr.lpLocalName = NULL;
	nr.lpComment = NULL;
	nr.lpProvider = NULL;
	nr.lpRemoteName = sFolder.GetBuffer(260);
	DWORD dwResult = WNetAddConnection2(&nr, // NETRESOURCE from enumeration 
									sUser != "" ? sPW.GetBuffer(200) : NULL,                  // no password 
									sUser != "" ? (LPSTR) sUser.GetBuffer(200) : NULL,                  // logged-in user 
										CONNECT_UPDATE_PROFILE);       // update profile with connect information 
	sFolder.ReleaseBuffer();
	sUser.ReleaseBuffer();
	sPW.ReleaseBuffer();

	// Try alternative connection method...
	if (sPW == "")
		sPW = "\"\"";
	if (dwResult != NO_ERROR) {
		CString commandLine = "NET USE \\\\"+sFolder + " " + sPW + " /USER:" + sUser;
		return	DoCreateProcessEx(commandLine, 300);
	}

	return CheckFolderWithPing(sFolder);
}


BOOL CUtils::DoCreateProcessEx(CString sCmdLine, int nTimeout)
{
    STARTUPINFO			si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);

	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;

    // Start the child process. 
	TCHAR szCmdLine[MAX_PATH];
	strcpy(szCmdLine, sCmdLine);
	if( !::CreateProcess( NULL,	// No module name (use command line). 
						szCmdLine,				// Command line. 
						NULL,					// Process handle not inheritable. 
						NULL,					// Thread handle not inheritable. 
						FALSE,					// Set handle inheritance to FALSE. 
						0,						// No creation flags. 
						NULL,					// Use parent's environment block. 
						NULL,					// Use parent's starting directory. 
						&si,					// Pointer to STARTUPINFO structure.
						&pi ) )	{				// Pointer to PROCESS_INFORMATION structure.
		
			Logprintf("Create Process for external script file failed. (%s)", szCmdLine);
		
	   return FALSE;
    }

	::WaitForInputIdle(pi.hProcess, nTimeout>0 ? nTimeout*1000 : INFINITE);
	::WaitForSingleObject( pi.hProcess, nTimeout>0 ? nTimeout*1000 : INFINITE );
	::CloseHandle( pi.hProcess );
	::CloseHandle( pi.hThread );
	return TRUE;
}

CString CUtils::GetExtension(CString sFileName)
{
	int n = sFileName.ReverseFind('.');
	if (n == -1)
		return _T("");
	return sFileName.Mid(n+1);
}

CString CUtils::GetFileName(CString sFullName)
{
	int n = sFullName.ReverseFind('\\');
	if (n == -1)
		return sFullName;
	return sFullName.Mid(n+1);
}

CString CUtils::GetFilePath(CString sFullName)
{
	int n = sFullName.ReverseFind('\\');
	if (n == -1)
		return sFullName;
	return sFullName.Left(n);
}


CString CUtils::GetFileName(CString sFullName, BOOL bExcludeExtension)
{
	if (bExcludeExtension) {
		int m = sFullName.ReverseFind('.');
		if (m != -1)
			sFullName = sFullName.Left(m);
	}
	int n = sFullName.ReverseFind('\\');
	if (n == -1)
		return sFullName;
	return sFullName.Mid(n+1);
}


int CUtils::GetFileAgeHours(CString sFileName)
{
	if (FileExist(sFileName) == FALSE)
		return 100000;
	CTime t1 = GetWriteTime(sFileName);
	CTime t2 = CTime::GetCurrentTime();
	CTimeSpan ts = t2 - t1;
	if (ts.GetTotalHours() > 0)
		return ts.GetTotalHours();
	
	return -1;
}



int CUtils::GetFileAgeMinutes(CString sFileName)
{
	if (FileExist(sFileName) == FALSE)
		return -1;
	CTime t1 = GetWriteTime(sFileName);
	CTime t2 = CTime::GetCurrentTime();
	CTimeSpan ts = t2 - t1;
	if (ts.GetTotalMinutes() > 0)
		return ts.GetTotalMinutes();
	
	return -1;
}


CString  CUtils::GetFirstFile(CString sSpoolFolder, CString sSearchMask)
{
	WIN32_FIND_DATA	fdata;
	HANDLE			fHandle;
	TCHAR			szNameSearch[MAX_PATH];		

	CString sFoundFile = "";
	if (CheckFolderWithPing(sSpoolFolder) == FALSE)
		return "";
	
	if (sSpoolFolder.Right(1) != "\\")
		sSpoolFolder += _T("\\");

	_stprintf(szNameSearch, "%s%s",(LPCTSTR)sSpoolFolder, (LPCTSTR)sSearchMask);
	fHandle = ::FindFirstFile(szNameSearch, &fdata);
	if (fHandle == INVALID_HANDLE_VALUE) {
		// All empty
		return "";
	}

	do {
		if (!(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )) {
			sFoundFile.Format( "%s\\%s", (LPCTSTR)sSpoolFolder, fdata.cFileName);	
			break;
		} 
		if (!::FindNextFile(fHandle, &fdata)) 
			break;
	} while (TRUE);

	::FindClose(fHandle);
	
	return sFoundFile;
}

int CUtils::GetFolderList(CString sSpoolFolder, CStringArray &sList)
{
	WIN32_FIND_DATA	fdata;
	HANDLE			fHandle;
	TCHAR			NameSearch[MAX_PATH];		
	int				nFolders=0;
	
	sList.RemoveAll();
	
	if (CheckFolderWithPing(sSpoolFolder) == FALSE)
		return -1;

	if (sSpoolFolder.Right(1) != "\\")
		sSpoolFolder += _T("\\");

	_stprintf(NameSearch, "%s\\*.*",(LPCTSTR)sSpoolFolder);
	fHandle = ::FindFirstFile(NameSearch, &fdata);
	if (fHandle == INVALID_HANDLE_VALUE) {
		// All empty
		return 0;
	}

	// Got something
	do {
			
		CString sFolder = fdata.cFileName;

		if ((fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) && sFolder != _T(".") && sFolder != _T("..")) {  
			
			sList.Add(sFolder);
			nFolders++;
		}

		if (!::FindNextFile(fHandle, &fdata)) 
			break;

	} while (TRUE);

	::FindClose(fHandle);

	return nFolders;
}

int CUtils::ScanDir(CString sSpoolFolder, CString sSearchMask, FILEINFOSTRUCT aFileList[], int nMaxFiles, CString sIgnoreMask,
																					BOOL bIgnoreHidden, BOOL bKillZeroByteFiles)
{
	return ScanDir(sSpoolFolder, sSearchMask, aFileList, nMaxFiles, sIgnoreMask,
									bIgnoreHidden, bKillZeroByteFiles, FALSE, 0, NULL);
}

int CUtils::ScanDir(CString sSpoolFolder, CString sSearchMask, FILEINFOSTRUCT aFileList [], int nMaxFiles, CString sIgnoreMask, 
					BOOL bIgnoreHidden, BOOL bKillZeroByteFiles, BOOL bUseRegex, int nRegex, REGEXPSTRUCT pRegex [])
{
	WIN32_FIND_DATA	fdata;
	HANDLE			fHandle;
	TCHAR			NameSearch[MAX_PATH], FoundFile[MAX_PATH];		
	int				nFiles=0;
	FILETIME		WriteTime, CreateTime, LocalWriteTime;
	SYSTEMTIME		SysTime;
	
	if (CheckFolderWithPing(sSpoolFolder) == FALSE)
		return -1;
	
	if (sSpoolFolder.Right(1) != "\\")
		sSpoolFolder += _T("\\");

	_stprintf(NameSearch, "%s%s",(LPCTSTR)sSpoolFolder, (LPCTSTR)sSearchMask);
	fHandle = ::FindFirstFile(NameSearch, &fdata);
	if (fHandle == INVALID_HANDLE_VALUE) {
		// All empty
		return 0;
	}

	// Got something
	do {

		if (!(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )) {


			if ( bIgnoreHidden == FALSE || 
				(bIgnoreHidden && (fdata.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == 0) ||
				(bIgnoreHidden && (fdata.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) == 0)) {

				BOOL bFilterOK = TRUE;

				if (sIgnoreMask != "" && sIgnoreMask != "*" && sIgnoreMask != "*.*") {
					CString s1 = GetExtension(fdata.cFileName);
					CString s2 = GetExtension(sIgnoreMask);
					if (s1.CompareNoCase(s2) == 0)
						bFilterOK  = FALSE;
				}

				CString sFile = fdata.cFileName;
				if ( sFile.CompareNoCase(".DS_Store") == 0 || sFile.CompareNoCase("Thumbs.db") == 0)
					bFilterOK  = FALSE;

				if (bFilterOK && bUseRegex) {
					bFilterOK = FALSE;
					
					
					for (int reg=0; reg <nRegex; reg++) {
						bFilterOK = TryMatchExpression(pRegex[reg].m_matchExpression, sFile, pRegex[reg].m_partialmatch);
						if (bFilterOK)
							break;
					}
					if (bFilterOK == FALSE) {
						sFile.MakeUpper();
						for (int reg=0; reg <nRegex; reg++) {
							bFilterOK = TryMatchExpression(pRegex[reg].m_matchExpression, sFile, pRegex[reg].m_partialmatch);
							if (bFilterOK)
								break;
						}
					}
					if (bFilterOK == FALSE) {
						sFile.MakeLower();
						for (int reg=0; reg <nRegex; reg++) {
							bFilterOK = TryMatchExpression(pRegex[reg].m_matchExpression, sFile, pRegex[reg].m_partialmatch);
							if (bFilterOK)
								break;
						}
					}


				}

				if (bFilterOK) {

					_stprintf(FoundFile, "%s\\%s", (LPCTSTR)sSpoolFolder, fdata.cFileName);	
					HANDLE Hndl = INVALID_HANDLE_VALUE;
					if (g_prefs.m_lockcheckmode == LOCKCHECKMODE_READ)
						Hndl = ::CreateFile(FoundFile, GENERIC_READ , FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
					else if (g_prefs.m_lockcheckmode != LOCKCHECKMODE_NONE)
						Hndl = ::CreateFile(FoundFile, GENERIC_READ , FILE_SHARE_READ |FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
					if (Hndl != INVALID_HANDLE_VALUE || g_prefs.m_lockcheckmode == LOCKCHECKMODE_NONE) {
			
						int ok = TRUE;
						if (g_prefs.m_lockcheckmode != LOCKCHECKMODE_NONE) {
							ok = ::GetFileTime( Hndl, &CreateTime, NULL, &WriteTime);
							::CloseHandle(Hndl);
						} else {
							CreateTime.dwHighDateTime = fdata.ftCreationTime.dwHighDateTime;
							CreateTime.dwLowDateTime = fdata.ftCreationTime.dwLowDateTime;
							WriteTime.dwHighDateTime = fdata.ftLastWriteTime.dwHighDateTime;
							WriteTime.dwLowDateTime = fdata.ftLastWriteTime.dwLowDateTime;
						}

						if (bKillZeroByteFiles) {
							if (fdata.nFileSizeLow == 0 && fdata.nFileSizeHigh == 0 && GetFileAgeMinutes(FoundFile) > 5) {
								::DeleteFile(FoundFile);
								ok  = FALSE;
							}
						}

						if (ok && fdata.nFileSizeLow == 0 && fdata.nFileSizeHigh == 0)
							ok  = FALSE;

						if (ok) {
							aFileList[nFiles].sFolder = sSpoolFolder;
							aFileList[nFiles].sFileName = fdata.cFileName;
							aFileList[nFiles].nFileSize = fdata.nFileSizeLow;

							// Store create time for FIFO sorting

							FileTimeToLocalFileTime( g_prefs.m_bSortOnCreateTime ? &CreateTime :  &WriteTime, &LocalWriteTime );
							FileTimeToSystemTime(&LocalWriteTime, &SysTime);
							CTime t((int)SysTime.wYear,(int)SysTime.wMonth,(int)SysTime.wDay,(int)SysTime.wHour,(int)SysTime.wMinute, (int)SysTime.wSecond); 
							aFileList[nFiles].tJobTime = t;	
		

							// Store last write time for stable time checker

							FileTimeToLocalFileTime( &WriteTime, &LocalWriteTime );
							FileTimeToSystemTime(&LocalWriteTime, &SysTime);
							CTime t2((int)SysTime.wYear,(int)SysTime.wMonth,(int)SysTime.wDay,(int)SysTime.wHour,(int)SysTime.wMinute, (int)SysTime.wSecond); 
							aFileList[nFiles++].tWriteTime = t2;
						}
					}	
				}
			} 
		}

		if (!::FindNextFile(fHandle, &fdata)) 
			break;

	} while (nFiles < nMaxFiles);

	::FindClose(fHandle);

	// Sort found files on create-time
	CTime		tTmpTimeValueCTime;
	CString 	sTmpFileName;
	DWORD		nTmpFileSize;
	for (int i=0;i<nFiles-1;i++) {
		for (int j=i+1;j<nFiles;j++) {
			if (aFileList[i].tJobTime > aFileList[j].tJobTime) {

				// Swap elements
				sTmpFileName = aFileList[i].sFileName; 
				aFileList[i].sFileName = aFileList[j].sFileName;
				aFileList[j].sFileName = sTmpFileName;				

				sTmpFileName = aFileList[i].sFolder;
				aFileList[i].sFolder = aFileList[j].sFolder;
				aFileList[j].sFolder = sTmpFileName;

				tTmpTimeValueCTime = aFileList[i].tJobTime;
				aFileList[i].tJobTime = aFileList[j].tJobTime;
				aFileList[j].tJobTime = tTmpTimeValueCTime;

				nTmpFileSize = aFileList[i].nFileSize;
				aFileList[i].nFileSize = aFileList[j].nFileSize;
				aFileList[j].nFileSize = nTmpFileSize;

				tTmpTimeValueCTime = aFileList[i].tWriteTime;
				aFileList[i].tWriteTime = aFileList[j].tWriteTime;
				aFileList[j].tWriteTime = tTmpTimeValueCTime;
			}
		}
	}

	return nFiles;
}

CTime CUtils::GetLogFileTime(CString sLogFileName)
{
	HANDLE	Hndl;
	char	infbuf[257];
	DWORD	nBytesRead;

	CTime tm(1975, 1, 1, 0, 0, 0);

	if ((Hndl = ::CreateFile(sLogFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0)) == INVALID_HANDLE_VALUE)
		return tm;

	if (!::ReadFile(Hndl, infbuf, 256, &nBytesRead, NULL)) {
		::CloseHandle(Hndl);
		return tm;
	}

	::CloseHandle(Hndl);

	infbuf[nBytesRead == 256 ? 255 : nBytesRead] = 0;

	// [212312313] 2009-01-02 19:28:12

	CString s = infbuf;
	int n = s.Find("] ");
	if (n != -1)
		s = s.Mid(n + 2).Trim();

	//	Logprintf("INFO:  InputTime from error log file %s", s);

	if (atoi(s) > 2000)
		return String2Time(s);

	return tm;
}


CTime CUtils::String2Time(CString s)
{
	// 2010-01-22 23:24:25

	CTime tm(1975, 1, 1, 0, 0, 0);

	try {

		if (s.GetLength() < 19)
			return tm;

		int y = atoi(s.Mid(0, 4));
		int m = atoi(s.Mid(5, 2));
		int d = atoi(s.Mid(8, 2));
		int h = atoi(s.Mid(11, 2));
		int mi = atoi(s.Mid(14, 2));
		int sec = atoi(s.Mid(17, 2));


		CTime tm2(y, m, d, h, mi, sec);
		tm = tm2;
	}
	catch (CException *ex)
	{
	}
	catch (...)
	{
	}

	return tm;
}

void CUtils::ErrorFilePrintf(CString sFileName, char *msg, ...)
{
	TCHAR	szLogLine[16000], szFinalLine[16000];
	va_list	ap;
	DWORD	n, nBytesWritten;
	SYSTEMTIME	ltime;

	HANDLE hFile = ::CreateFile(sFileName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;

	// Seek to end of file
	::SetFilePointer(hFile, 0, NULL, FILE_END);

	va_start(ap, msg);
	n = vsprintf(szLogLine, msg, ap);
	va_end(ap);
	szLogLine[n] = '\0';

	::GetLocalTime(&ltime);
	_stprintf(szFinalLine, "[%.2d-%.2d %.2d:%.2d:%.2d.%.3d]  %s\r\n", (int)ltime.wDay, (int)ltime.wMonth, (int)ltime.wHour, (int)ltime.wMinute, (int)ltime.wSecond, (int)ltime.wMilliseconds, szLogLine);

	::WriteFile(hFile, szFinalLine, (DWORD)_tcsclen(szFinalLine), &nBytesWritten, NULL);

	::CloseHandle(hFile);
}




BOOL CUtils::GetCCDATAFreeSpaceMB(DWORD &i32FreeMB)
{
	ULARGE_INTEGER	i64FreeBytesToCaller;
	ULARGE_INTEGER	i64TotalBytes;
	ULARGE_INTEGER	i64FreeBytes;

	BOOL fResult = GetDiskFreeSpaceEx(g_prefs.m_hiresPDFPath,
		(PULARGE_INTEGER)&i64FreeBytesToCaller,
		(PULARGE_INTEGER)&i64TotalBytes,
		(PULARGE_INTEGER)&i64FreeBytes);

	if (fResult == FALSE)
		return FALSE;

	ULONGLONG i64FreeMB = i64FreeBytes.QuadPart / (1024 * 1024);
	i32FreeMB = (DWORD)i64FreeMB;
	return TRUE;
}

CTime CUtils::Systime2CTime(SYSTEMTIME SysTime)
{
	CTime t = CTime(1975, 1, 1, 0, 0, 0);
	try {
		t = CTime((int)SysTime.wYear, (int)SysTime.wMonth, (int)SysTime.wDay, (int)SysTime.wHour, (int)SysTime.wMinute, (int)SysTime.wSecond);
	}
	catch (CException *ex)
	{
		;
	}

	return t;
}

CString CUtils::GenerateTimeStamp()
{
	CTime t = CTime::GetCurrentTime();

	CString s;
	s.Format("%.4d%.2d%.2d%.2d%.2d%.2d", t.GetYear(), t.GetMonth(), t.GetDay(), t.GetHour(), t.GetMinute(), t.GetSecond());

	return s;

}


int CUtils::TokenizeName(CString sName, CString sSeparators, CString sv[], CString sp[])
{
	int nCount = 0;
	CString s = sName;
	while (s.GetLength() > 0) {

		int en = s.FindOneOf(sSeparators);
		if (en == -1) {
			sp[nCount] = "";
			sv[nCount++] = s;	
			break;
		} 
		sp[nCount] = s[en];
		sv[nCount++] = s.Left(en);

		s = s.Mid(en+1);
	}

	return nCount;
}	

BOOL CUtils::TryMatchExpression(CString sMatchExpression, CString sInputString, BOOL bPartialMatch)
{
	boost::regex	e;
	BOOL			ret;
	
	TCHAR szMatch[1024], szInput[1024];
	strcpy(szMatch, sMatchExpression);
	strcpy(szInput, sInputString);

	try {
		e.assign(szMatch);
		ret = boost::regex_match(szInput, e, bPartialMatch ? boost::match_partial|boost::match_default : boost::match_default);
	}
	catch(std::exception& e)
    {
         return FALSE;
    }
	
	return ret;
}

CString CUtils::FormatExpression(CString sMatchExpression, CString sFormatExpression, CString sInputString, BOOL bPartialMatch)
{
	boost::regex	e;
//	unsigned int				match_flag = bPartialMatch ? boost::match_partial : boost::match_default;
// 	unsigned int				flags = match_flag | boost::format_perl;
	CString			retSt = sInputString;
	
	TCHAR szMatch[1024], szInput[1024], szFormat[1024];
	strcpy(szMatch, sMatchExpression);
	strcpy(szInput, sInputString);
	strcpy(szFormat, sFormatExpression);
	
	std::string in = szInput;

	try {
		e.assign(szMatch);
	}
	catch(std::exception& e)
    {
         return retSt;
    }

	try {
		BOOL ret = boost::regex_match(in, e, bPartialMatch ? boost::match_partial|boost::match_default : boost::match_default);
		
		if (ret == TRUE) {
			std::string r = boost::regex_merge(in, e, szFormat, boost::format_perl | (bPartialMatch ? boost::match_partial|boost::match_default : boost::match_default));
			retSt = r.data();
		}
	}
	catch(std::exception& e)
    {
         return retSt;
    }
	return retSt;
}

CString CUtils::GetRawMaskEx(CString sOrgMask)
{
	int n = 0; //, nIndex = -1;
	CString sNamingMask = sOrgMask.MakeLower();
	CString sResult = _T("");

	while (n < sNamingMask.GetLength()) {

		if (sNamingMask[n] != _TCHAR('%')) {
			sResult += sNamingMask.Mid(n,1);
			n++;
			continue;
		}
		
		if (n + 1 < sNamingMask.GetLength())
			n++;

		// See if we have repeater instruction (digits)
		int reps = _tstoi(sNamingMask.Mid(n));
		if (reps > 0) {
			// Skip over digits
			while (n < sNamingMask.GetLength()) {
				if (isdigit((int)(sNamingMask[n])))
					n++;
				else
					break;
			}
		} else {
			reps = 1;
		}
		
		if (n >= sNamingMask.GetLength())
			break; // no more string..

		// Store id character	
		CString id = sNamingMask.Mid(n,1);
		for (int i=0; i<reps; i++)
			sResult += id.MakeUpper();

		// Skip over id
		n++;
		
		//	See if if have special instructions in brackets
		if (n  >= sNamingMask.GetLength()) 
			break;
		
	}
	return sResult;
}

BOOL CUtils::IsolateReference(CString sNameFormat, CString sFileName, CString &sColor, CString &sPageNumber) 
{
	int n=0;
	sPageNumber = _T("");
	sColor = _T("");
	if (sFileName == "")
		return FALSE;

	sNameFormat.MakeUpper();
	if (sNameFormat.Find("%P") == -1 && sNameFormat.Find("%C") == -1)
		return FALSE;

	CString sRawMask = GetRawMaskEx(sNameFormat);

	BOOL bIsFullyFixed = TRUE;
	if (sRawMask.FindOneOf("-_.") != -1) 
		bIsFullyFixed = FALSE;

	if (bIsFullyFixed) {
		// Standard fixed size naming scheme
		while (n < sRawMask.GetLength()) {
			switch (sRawMask[n]) {
			case 'P':
				sPageNumber += sFileName[n];
				break;
			case 'C':
				sColor += sFileName[n];
				break;
			}
			n++;
		}
	} else {
		CString sIsolatedName;
		
		int m = 0, mm = 0;
		while (n < sRawMask.GetLength() && m < sFileName.GetLength()) {

			// Isolate next id (until next delimiter)
			mm = sFileName.Mid(m).FindOneOf("-_.");	// Get next delimiter in job name						
			if (mm != -1)
				sIsolatedName = sFileName.Mid(m,mm-m+m);
			else
				sIsolatedName = sFileName.Mid(m);
				
			switch (sRawMask[n]) {
				case 'P':
					sPageNumber = sIsolatedName;
					break;
				case 'C':
					sColor = sIsolatedName;
					break;
				default:
					// Don't care character
					break;
			}
			m += mm+1;
			n++;
			if (sRawMask.Mid(n).FindOneOf("-_.") != -1) 
				n++;
		}		
	}

	
	return TRUE;
}



CString CUtils::ReadTiffImageDescription(CString sFullInputPath)
{
	/*TIFF *tif;
	char *szTag;
	CString s = "";

	if ((tif = TIFFOpen(sFullInputPath, "r")) == NULL) 
		return "";
	TIFFGetFieldDefaulted(tif, TIFFTAG_IMAGEDESCRIPTION, &szTag);

	if (szTag != NULL)
		s = szTag;
	TIFFClose(tif);

	return s;*/
	return "";
}


BOOL CUtils::DoCreateProcess(CString sCmdLine)
{
    STARTUPINFO			si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);

	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;

//    HANDLE  hStdOutput; 
//    HANDLE  hStdError; 

    // Start the child process. 
	TCHAR szCmdLine[MAX_PATH];
	strcpy(szCmdLine, sCmdLine);
	if( !::CreateProcess( NULL,	// No module name (use command line). 
						szCmdLine,				// Command line. 
						NULL,					// Process handle not inheritable. 
						NULL,					// Thread handle not inheritable. 
						FALSE,					// Set handle inheritance to FALSE. 
						0,						// No creation flags. 
						NULL,					// Use parent's environment block. 
						NULL,					// Use parent's starting directory. 
						&si,					// Pointer to STARTUPINFO structure.
						&pi ) )	{				// Pointer to PROCESS_INFORMATION structure.
			Logprintf("ERROE: Create Process for external script file failed. (%s)", szCmdLine);
	   return FALSE;
    }
	
	::WaitForInputIdle(pi.hProcess, g_prefs.m_scripttimeout>0 ? g_prefs.m_scripttimeout*1000 : INFINITE);
    // Wait until child process exits.
	::WaitForSingleObject( pi.hProcess, g_prefs.m_scripttimeout>0 ? g_prefs.m_scripttimeout*1000 : INFINITE );

    // Close process and thread handles. 
	::CloseHandle( pi.hProcess );
	::CloseHandle( pi.hThread );
	return TRUE;
}

int CUtils::DeleteFiles(CString sSpoolFolder, CString sSearchMask)
{
	WIN32_FIND_DATA	fdata;
	HANDLE			fHandle;
	TCHAR			NameSearch[MAX_PATH], FoundFile[MAX_PATH];		
	int				nFiles=0;
	CStringArray	arr;

	if (CheckFolderWithPing(sSpoolFolder) == FALSE)
		return -1;
	
	if (sSpoolFolder.Right(1) != "\\")
		sSpoolFolder += _T("\\");

	_stprintf(NameSearch, "%s%s",(LPCTSTR)sSpoolFolder, (LPCTSTR)sSearchMask);
	fHandle = ::FindFirstFile(NameSearch, &fdata);
	if (fHandle == INVALID_HANDLE_VALUE) {
		// All empty
		return 0;
	}

	// Got something
	do {
		if (!(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )) {
			_stprintf(FoundFile, "%s\\%s", (LPCTSTR)sSpoolFolder, fdata.cFileName);	
			HANDLE Hndl = CreateFile(FoundFile, GENERIC_READ , FILE_SHARE_READ |FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
			if (Hndl != INVALID_HANDLE_VALUE) {
				CloseHandle(Hndl);
				arr.Add(fdata.cFileName);
				nFiles++;
			}			
		} 
		
		if (!::FindNextFile(fHandle, &fdata)) 
			break;

	} while (TRUE);

	::FindClose(fHandle);

	for (int i=0; i<arr.GetCount(); i++)
		::DeleteFile(sSpoolFolder + arr.ElementAt(i));

	return nFiles;
}

int CUtils::DeleteFiles(CString sSpoolFolder, CString sSearchMask, int sMaxAgeHours)
{
	WIN32_FIND_DATA	fdata;
	HANDLE			fHandle;
	TCHAR			NameSearch[MAX_PATH], FoundFile[MAX_PATH];		
	int				nFiles=0;
	CStringArray	arr;
	FILETIME ftm ;
	FILETIME ltm;
	SYSTEMTIME  stm;

	CTime tNow = CTime::GetCurrentTime();

	if (CheckFolder(sSpoolFolder) == FALSE)
		return -1;
	
	if (sSpoolFolder.Right(1) != "\\")
		sSpoolFolder += _T("\\");

	_stprintf(NameSearch, "%s%s",(LPCTSTR)sSpoolFolder, (LPCTSTR)sSearchMask);
	fHandle = ::FindFirstFile(NameSearch, &fdata);
	if (fHandle == INVALID_HANDLE_VALUE) {
		// All empty
		return 0;
	}

	
	do {
		if (!(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )) {
			_stprintf(FoundFile, "%s\\%s", (LPCTSTR)sSpoolFolder, fdata.cFileName);	
			HANDLE Hndl = CreateFile(FoundFile, GENERIC_READ , FILE_SHARE_READ |FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
			if (Hndl != INVALID_HANDLE_VALUE) {
				CloseHandle(Hndl);
				ftm = fdata.ftLastWriteTime;
				::FileTimeToLocalFileTime( &ftm, &ltm );
				::FileTimeToSystemTime(&ltm, &stm);

				CTime tFileAge((int)stm.wYear,(int)stm.wMonth,(int)stm.wDay,(int)stm.wHour,(int)stm.wMinute, (int)stm.wSecond); 
				CTimeSpan ts = tNow - tFileAge;
				if (ts.GetTotalHours() >=sMaxAgeHours) {
					arr.Add(fdata.cFileName);
					nFiles++;
				}
			}			
		} 
		
		if (!::FindNextFile(fHandle, &fdata)) 
			break;

	} while (TRUE);

	::FindClose(fHandle);

	for (int i=0; i<arr.GetCount(); i++)
		DeleteFile(sSpoolFolder + arr.ElementAt(i));

	return nFiles;
}

BOOL CUtils::MoveFileWithRetry(CString sSourceFile, CString sDestinationFile, CString &sErrorMessage)
{
	if (::MoveFileEx(sSourceFile, sDestinationFile, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == FALSE) {
		::Sleep(1000);
		if (::MoveFileEx(sSourceFile, sDestinationFile, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == FALSE) {
			sErrorMessage = GetLastWin32Error();
			return FALSE;
		}
	}

	return TRUE;
}

BOOL CUtils::CopyFileWithRetry(CString sSourceFile, CString sDestinationFile, CString &sErrorMessage)
{
	int bCopySuccess = FALSE;
	int nRetries = g_prefs.m_transmitretries;
	if (nRetries <= 1)
		nRetries = 2;
	while (--nRetries >= 0 && bCopySuccess == FALSE) {
		bCopySuccess = ::CopyFile(sSourceFile, sDestinationFile, 0);
		if (bCopySuccess)
			break;
		::Sleep(1000);
		Reconnect(GetFilePath(sDestinationFile), "", "");
	}
	if (bCopySuccess == FALSE)
		sErrorMessage = GetLastWin32Error();
	return bCopySuccess;
}


int CUtils::MoveFiles(CString sSpoolFolder, CString sSearchMask, CString sOutputFolder, BOOL bIgnoreZeroLength)
{
	WIN32_FIND_DATA	fdata;
	HANDLE			fHandle;
	TCHAR			NameSearch[MAX_PATH], FoundFile[MAX_PATH];		
	int				nFiles=0;
	CStringArray	arr;
//	FILETIME ftm ;
//	FILETIME ltm;
//	SYSTEMTIME  stm;

	CTime tNow = CTime::GetCurrentTime();

	if (CheckFolder(sSpoolFolder) == FALSE)
		return -1;
	if (CheckFolder(sOutputFolder) == FALSE)
		return -1;
	
	if (sSpoolFolder.Right(1) != "\\")
		sSpoolFolder += _T("\\");

	if (sOutputFolder.Right(1) != "\\")
		sOutputFolder += _T("\\");


	_stprintf(NameSearch, "%s%s",(LPCTSTR)sSpoolFolder, (LPCTSTR)sSearchMask);
	fHandle = ::FindFirstFile(NameSearch, &fdata);
	if (fHandle == INVALID_HANDLE_VALUE) {
		// All empty
		return 0;
	}

	
	do {
		if (!(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )) {
			_stprintf(FoundFile, "%s\\%s", (LPCTSTR)sSpoolFolder, fdata.cFileName);	

			HANDLE Hndl = CreateFile(FoundFile, GENERIC_READ , FILE_SHARE_READ |FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
			if (Hndl != INVALID_HANDLE_VALUE) {
				CloseHandle(Hndl);
				if (bIgnoreZeroLength == FALSE || (bIgnoreZeroLength && fdata.nFileSizeLow>0)) {
					arr.Add(fdata.cFileName);
					nFiles++;
				}
			}			
		} 
		
		if (!::FindNextFile(fHandle, &fdata)) 
			break;

	} while (TRUE);

	::FindClose(fHandle);

	for (int i=0; i<arr.GetCount(); i++)
		::MoveFileEx(sSpoolFolder + arr.ElementAt(i), sOutputFolder + arr.ElementAt(i), MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH );

	return nFiles;
}


int CUtils::ScanDirCount(CString sSpoolFolder, CString sSearchMask)
{
	WIN32_FIND_DATA	fdata;
	HANDLE			fHandle;
	TCHAR			NameSearch[MAX_PATH];
	int				nFiles=0;
	
	if (CheckFolderWithPing(sSpoolFolder) == FALSE)
		return -1;
	
	if (sSpoolFolder.Right(1) != "\\")
		sSpoolFolder += _T("\\");

	_stprintf(NameSearch, "%s%s",(LPCTSTR)sSpoolFolder, (LPCTSTR)sSearchMask);
	fHandle = ::FindFirstFile(NameSearch, &fdata);
	if (fHandle == INVALID_HANDLE_VALUE) {
		return 0;
	}

	// Got something
	do {
		if (!(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )) {
			CString sFileName(fdata.cFileName);
			if (sFileName != "." && sFileName != ".." && (fdata.nFileSizeLow > 0 || fdata.nFileSizeHigh > 0))
				nFiles++; 
		}
			
		if (!::FindNextFile(fHandle, &fdata)) 
			break;
	} while (TRUE);
	
	::FindClose(fHandle);

	return nFiles;
}

void CUtils::TruncateLogFile(CString sFile, DWORD dwMaxSize)
{
	DWORD dwCurrentSize = GetFileSize(sFile);

	if (dwCurrentSize == 0)
		return;

	if (dwCurrentSize > dwMaxSize) {
		::CopyFile(sFile,sFile + "2",FALSE);
		::DeleteFile(sFile);

	}
}

BOOL CUtils::SetFileToCurrentTime(CString sFile)
{
    FILETIME ft;
    SYSTEMTIME st;
    BOOL ret ;

	HANDLE hFile = ::CreateFile(sFile, FILE_WRITE_ATTRIBUTES , 0, NULL, OPEN_EXISTING, 0, 0);

	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	::GetSystemTime(&st);              // gets current time
	::SystemTimeToFileTime(&st, &ft);  // converts to file time format
	
	ret = ::SetFileTime(hFile, &ft, (LPFILETIME) NULL, &ft);

	::CloseHandle(hFile);

    return ret;
}

void CUtils::Logprintf(const TCHAR *msg, ...)
{
	TCHAR	szLogLine[64000], szFinalLine[64000];
	va_list	ap;
	DWORD	n, nBytesWritten;
	SYSTEMTIME	ltime;

	if (g_prefs.m_logToFile== FALSE)
		return;


	TruncateLogFile(g_prefs.m_logFileFolder + _T("\\InputService.log"), g_prefs.m_maxlogfilesize);

	HANDLE hFile = ::CreateFile(g_prefs.m_logFileFolder + _T("\\InputService.log"), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;

	// Seek to end of file
	::SetFilePointer(hFile, 0, NULL, FILE_END);

	va_start(ap, msg);
	n = vsprintf(szLogLine, msg, ap);
	va_end(ap);
	szLogLine[n] = '\0';

	::GetLocalTime(&ltime);
	_stprintf(szFinalLine, "[%.2d-%.2d-%.4d %.2d:%.2d:%.2d.%.3d] %s\r\n", (int)ltime.wDay, (int)ltime.wMonth, (int)ltime.wYear, (int)ltime.wHour, (int)ltime.wMinute, (int)ltime.wSecond, (int)ltime.wMilliseconds, szLogLine);

	::WriteFile(hFile, szFinalLine, (DWORD)_tcsclen(szFinalLine), &nBytesWritten, NULL);

	::CloseHandle(hFile);
}




BOOL CUtils::TryMatchExpressionEx(CString sMatchExpression, CString sInputString, BOOL bPartialMatch, CString &sErrorMessage)
{
	sErrorMessage = _T("");

	boost::regex	e;
//	unsigned int	match_flag = bPartialMatch ? boost::match_partial : boost::match_default;
	BOOL			ret;
	
	TCHAR szMatch[1024], szInput[1024];
	strcpy(szMatch, sMatchExpression);
	strcpy(szInput, sInputString);

	try {
		e.assign(szMatch);
	}
	catch(std::exception& e)
    {
		 sErrorMessage =  _T("ERROR in regular expression ");
		 sErrorMessage += e.what();
		 Logprintf("ERROR in regular expression %s",e.what());
         return FALSE;
    }
	try {
		ret = boost::regex_match(szInput, e, bPartialMatch ? boost::match_partial|boost::match_default : boost::match_default);
	}
	catch(std::exception& e)
    {

		 sErrorMessage = _T("ERROR in regular expression ");
		 sErrorMessage += e.what();
		 Logprintf("ERROR in regular expression %s",e.what());
         return FALSE;
    }
	
	return ret;
}

CString CUtils::FormatExpressionEx(CString sMatchExpression, CString sFormatExpression, CString sInputString, BOOL bPartialMatch,  CString &sErrorMessage, BOOL *bMatched)
{
	sErrorMessage = _T("");
	*bMatched = TRUE;
	boost::regex	e;
//	unsigned int				match_flag = bPartialMatch ? boost::match_partial : boost::match_default;
//	unsigned int				flags = match_flag | boost::format_perl;
	CString			retSt = sInputString;
	
	TCHAR szMatch[1024], szInput[1024], szFormat[1024];
	strcpy(szMatch, sMatchExpression);
	strcpy(szInput, sInputString);
	strcpy(szFormat, sFormatExpression);
	
	std::string in = szInput;

	try {
		e.assign(szMatch);
	}
	catch(std::exception& e)
    {
		sErrorMessage.Format("ERROR in regular expression %s", e.what());
		 *bMatched = FALSE;
         return retSt;
    }

	try {
		BOOL ret = boost::regex_match(in, e, bPartialMatch ? boost::match_partial|boost::match_default : boost::match_default);
		*bMatched = ret;
		if (ret == TRUE) {
			std::string r = boost::regex_merge(in, e, szFormat, boost::format_perl | (bPartialMatch ? boost::match_partial|boost::match_default : boost::match_default));
			retSt = r.data();
		}
	}
	catch(std::exception& e)
    {
		sErrorMessage.Format("ERROR in regular expression %s", e.what());
		  *bMatched = FALSE;
         return retSt;
    }

	return retSt;
}

int CUtils::FreeDiskSpaceMB(CString sPath)
{
	ULARGE_INTEGER	i64FreeBytesToCaller;
	ULARGE_INTEGER	i64TotalBytes;
	ULARGE_INTEGER	i64FreeBytes;
		
	BOOL fResult = GetDiskFreeSpaceEx (sPath,
					(PULARGE_INTEGER)&i64FreeBytesToCaller,
					(PULARGE_INTEGER)&i64TotalBytes,
					(PULARGE_INTEGER)&i64FreeBytes);

	if (fResult) {
//		double ft = i64TotalBytes.HighPart; 
//		ft *= MAXDWORD;
//		ft += i64TotalBytes.LowPart;

		double ff = i64FreeBytes.HighPart;
		ff *= MAXDWORD;
		ff += i64FreeBytes.LowPart;
			
		double fMBfree = ff/(1024*1024);
//		TCHAR sz[30];
//		sprintf(sz, "%d MB free",(DWORD)fMBfree);
//		double ratio = 100 - 100.0 * ff/ft;

		return (int)fMBfree;
	}
	
	return -1;

}

CTime g_lastMailFolderError(1975,1,1,0,0,0);
CTime g_lastMailFileError(1975,1,1,0,0,0);
CTime g_lastMailDatabaseError(1975, 1, 1, 0, 0, 0);
CTime g_lastMailPollError(1975, 1, 1, 0, 0, 0);

BOOL CUtils::SendMail(int nMailType, CString sMessage)
{
	if (g_prefs.m_emailsmtpserver == "")
		return TRUE;

	CString sMailBody = _T("");
	CString sErrorType = _T("");
	CTimeSpan ts;
	CTime tNow = CTime::GetCurrentTime();
	
	if (nMailType == MAILTYPE_FILEERROR) {
		sErrorType = _T("File type validation error (corrupt file?)");
		ts = tNow - g_lastMailFileError;
	}
	else if (nMailType == MAILTYPE_DBERROR) {
		sErrorType = _T("Database processing error");
		ts = tNow - g_lastMailDatabaseError;
	}
	else if (nMailType == MAILTYPE_POLLINGERROR) {
		sErrorType = _T("File polling error");
		ts = tNow - g_lastMailPollError;
	} else {
		sErrorType = _T("Folder access error");
		ts = tNow - g_lastMailFolderError;
	}

	sMailBody = sErrorType  + _T(":\r\n") + sMessage; 

	if (g_prefs.m_emailpreventflooding && ts.GetTotalMinutes() < g_prefs.m_emailpreventfloodingdelay)
		return TRUE;

	CEmailClient email;

	BOOL ret = SendMail(g_prefs.m_emailsubject + _T(" - ") + sErrorType, sMailBody);

	if (nMailType == MAILTYPE_FILEERROR) 
		g_lastMailFileError = CTime::GetCurrentTime();
	else if (nMailType == MAILTYPE_DBERROR)
		g_lastMailDatabaseError = CTime::GetCurrentTime();
	else if (nMailType == MAILTYPE_POLLINGERROR)
		g_lastMailPollError = CTime::GetCurrentTime();
	else
		g_lastMailFolderError = CTime::GetCurrentTime();

	return ret;

}

BOOL CUtils::SendMail(CString sMailSubject, CString sMailBody)
{
	return 	SendMail(sMailSubject, sMailBody, _T(""));
}


BOOL CUtils::SendMail(CString sMailSubject, CString sMailBody, CString sAttachMent)
{
	CEmailClient email;

	email.m_SMTPuseSSL = g_prefs.m_mailusessl == 1;
	email.m_SMTPstartTLS = g_prefs.m_mailusessl == 2;
	return email.SendMailAttachment(g_prefs.m_emailsmtpserver, g_prefs.m_mailsmtpport, g_prefs.m_mailsmtpserverusername, g_prefs.m_mailsmtpserverpassword,
		g_prefs.m_emailfrom,
		g_prefs.m_emailto, g_prefs.m_emailcc, sMailSubject, sMailBody, false, sAttachMent);
}



int CUtils::GetMailAttachments(BOOL bIMAP, CString sMailServer, int nPort, CString sUsername, CString sPassword, BOOL bSSL,
	CString sEmailFilter, BOOL bDeleteAfter, 
								CString sDestFolder, FILEINFOSTRUCT aFileList [], int nMaxFiles, CString sInBox,
								CString &sErrorMessage)
{
	CEmailClient pop3_in;
	CIMAPClient imap_in;
	TCHAR szErrorMessage[MAX_ERRMSG];
	strcpy(szErrorMessage, "");
	int nRet = 0;
	EMAILFILEINFOSTRUCT aEmailFileList[MAXEMAILFILES];

	int nAttachments = 0;
	sErrorMessage = _T("");

	int nMaxAttachments = nMaxFiles >= MAXEMAILFILES ? MAXEMAILFILES : nMaxFiles;

	if (bIMAP) {
		imap_in.SetIMAPSSLmode(bSSL);
		imap_in.SetIMAPConnectoinTimeout(g_prefs.m_emailtimeout);
		imap_in.SetIMAPMailBox(sInBox != _T("") ? sInBox : _T("Inbox"));
		nRet = imap_in.GetMailAttachments(sMailServer, nPort, sUsername, sPassword, sEmailFilter, bDeleteAfter, sDestFolder,
			aEmailFileList, nMaxAttachments, szErrorMessage);
		
	}
	else {
		pop3_in.SetPOP3SSLmode(bSSL);
		pop3_in.SetPOP3ConnectoinTimeout(g_prefs.m_emailtimeout);
		nRet = pop3_in.GetMailAttachments(sMailServer, nPort, sUsername, sPassword, sEmailFilter, bDeleteAfter, sDestFolder,
			aEmailFileList, nMaxAttachments, szErrorMessage);		
	}

	if (nRet == -1) {
		sErrorMessage = szErrorMessage;
		return -1;
	}

	for (int i = 0; i < nRet; i++) {
		aFileList[i].sFileName = aEmailFileList[i].sFileName;
		aFileList[i].nFileSize = aEmailFileList[i].nFileSize;
		aFileList[i].sFolder = _T("");
		aFileList[i].tJobTime = aEmailFileList[i].tJobTime;
		aFileList[i].tWriteTime = aEmailFileList[i].tWriteTime;		
	}

	return nRet;

}




CString CUtils::GetLastWin32Error()
{
    TCHAR szBuf[4096]; 
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
					NULL,
					dw,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR) &lpMsgBuf,
					0, NULL );

    wsprintf(szBuf, "%s (errorcode %d)",lpMsgBuf, dw); 
 
	CString s(szBuf);
    LocalFree(lpMsgBuf);
 
	return s;
}

CString CUtils::CurrentTime2String()
{
	CString s;
	CTime tDate(CTime::GetCurrentTime());
	s.Format("%.2d-%.2d-%.4d %.2d:%.2d:%.2d", tDate.GetDay(), tDate.GetMonth(), tDate.GetYear(), tDate.GetHour(), tDate.GetMinute(), tDate.GetSecond());

	return s;
}

CTime CUtils::GetNextDay(CTime dt)
{
	CTimeSpan ts = CTimeSpan(1, 0, 0, 0);
	dt += ts;
	return dt;
}

CTime CUtils::ParseDate(CString sDate, CString sDateFormat)
{
	int y = 0;
	int m = 0;
	int d = 0;
	if (sDateFormat == "YYMMDD" && sDate.GetLength() == 6)
	{
		y = 2000 + atoi(sDate.Mid(0, 2));
		m = atoi(sDate.Mid(2, 2));
		d = atoi(sDate.Mid(4, 2));
	}
	else if (sDateFormat == "YYYYMMDD" && sDate.GetLength() == 8)
	{
		y = atoi(sDate.Mid(0, 4));
		m = atoi(sDate.Mid(4, 2));
		d = atoi(sDate.Mid(5, 2));
	}
	else if (sDateFormat == "DDMMYY" && sDate.GetLength() == 6)
	{
		d = atoi(sDate.Mid(0, 2));
		m = atoi(sDate.Mid(2, 2));
		y = 2000 + atoi(sDate.Mid(4, 2));
	}
	else if (sDateFormat == "DDMMYYYY" && sDate.GetLength() == 8)
	{
		d = atoi(sDate.Mid(0, 2));
		m = atoi(sDate.Mid(2, 2));
		y = atoi(sDate.Mid(4, 4));
	}
	if (y > 0 && m > 0 && d > 0)
		return CTime(y, m, d, 0, 0, 0);

	return CTime(1975, 1, 1, 0, 0, 0);
}


CString CUtils::Date2String(CTime tDate, CString sDateFormat, BOOL bAllowSeparators, int nWeekNumber)
{
	COleDateTime dtt(tDate.GetYear(), tDate.GetMonth(), tDate.GetDay(), tDate.GetHour(), tDate.GetMinute(), tDate.GetSecond());

	CTimeSpan ts1day = CTimeSpan(1, 0, 0, 0);
	CString sDate = "";
	CString s;

	CTime tDate1 = tDate + ts1day;

	if (sDateFormat == "DD1") {
		s.Format("%.2d", tDate1.GetDay());
		return s;
	}

	if (sDateFormat == "DD1MM") {
		s.Format("%.2d%.2d", tDate1.GetDay(), tDate1.GetMonth());
		return s;
	}
	if (sDateFormat == "DD1MMYY") {
		s.Format("%.2d%.2d%.2d", tDate1.GetDay(), tDate1.GetMonth(), tDate1.GetYear() - 2000);
		return s;
	}
	if (sDateFormat == "DD1MMYYYY") {
		s.Format("%.2d%.2d%.4d", tDate1.GetDay(), tDate1.GetMonth(), tDate1.GetYear());
		return s;
	}
	if (sDateFormat == "DDMMYY") {
		s.Format("%.2d%.2d%.2d", tDate.GetDay(), tDate.GetMonth(), tDate.GetYear() - 2000);
		return s;
	}
	if (sDateFormat == "DDMMYYYY") {
		s.Format("%.2d%.2d%.4d", tDate.GetDay(), tDate.GetMonth(), tDate.GetYear());
		return s;
	}
	if (sDateFormat == "YYMMDD") {
		s.Format("%.2d%.2d%.2d", tDate.GetYear() - 2000, tDate.GetMonth(), tDate.GetDay());
		return s;
	}
	if (sDateFormat == "YYYYMMDD") {
		s.Format("%.4d%.2d%.2d", tDate.GetYear(), tDate.GetMonth(), tDate.GetDay());
		return s;
	}


	int h = 0;
	while (h < sDateFormat.GetLength()) {
		int hbefore = h;

		if (sDateFormat.Find("J") == h && sDateFormat.Find("JJJ") == -1) {


			s.Format("%d", dtt.GetDayOfYear());
			sDate += s;
			h += s.GetLength();
		}

		if (sDateFormat.Find("JJJ") == h && sDateFormat.Find("JJJJ") == -1) {
			s.Format("%.3d", dtt.GetDayOfYear());
			sDate += s;
			h += s.GetLength();
		}
		if (sDateFormat.Find("JJJJ") == h) {
			s.Format("%.4d", dtt.GetDayOfYear());
			sDate += s;
			h += s.GetLength();
		}
		if (sDateFormat.Find("HH") == h) {
			h += 2;
			s.Format("%.2d", tDate.GetHour());
			sDate += s;
		}
		if (sDateFormat.Find("hh") == h) {
			h += 2;
			s.Format("%.2d", tDate.GetHour());
			sDate += s;
		}
		if (sDateFormat.Find("mm") == h) {
			h += 2;
			s.Format("%.2d", tDate.GetMinute());
			sDate += s;
		}
		if (sDateFormat.Find("ss") == h) {
			h += 2;
			s.Format("%.2d", tDate.GetSecond());
			sDate += s;
		}
		if (sDateFormat.Find("SS") == h) {
			h += 2;
			s.Format("%.2d", tDate.GetSecond());
			sDate += s;
		}

		if (sDateFormat.Find("DD") == h) {
			h += 2;
			s.Format("%.2d", tDate.GetDay());
			sDate += s;
		}
		if (sDateFormat.Find("MM") == h) {
			h += 2;
			s.Format("%.2d", tDate.GetMonth());
			sDate += s;
		}
		if (sDateFormat.Find("WW") == h) {
			h += 2;
			s.Format("%.2d", nWeekNumber);
			if (nWeekNumber > 0)
				sDate += s;
		}

		if (sDateFormat.Find("YY") == h && sDateFormat.Find("YYYY") == -1) {
			h += 2;
			s.Format("%.2d", tDate.GetYear() - 2000);
			sDate += s;
		}
		if (sDateFormat.Find("YYYY") == h) {
			h += 4;
			s.Format("%.4d", tDate.GetYear());
			sDate += s;
		}
		if (h == hbefore) {
			if (bAllowSeparators) {
				sDate += sDateFormat[h];
			}
			h++;
		}
	}
	return sDate;
}

CString CUtils::Date2StringEx(CTime tDate, CString sDateFormat, BOOL bAllowSeparators, int nWeekNumber)
{
	sDateFormat.MakeUpper();

	CString sDate = sDateFormat;
	CString sDay = "";
	CString sDayShort = "";
	CString sMonth = "";
	CString sMonthShort = "";
	CString sYearShort = "";
	CString sYearLong = "";
	CString sWeekNumber = "";
	CString sDayNumber = "";
	CString sDayOfYear = "";

	sDay.Format("%.2d", tDate.GetDay());
	sDayShort.Format("%d", tDate.GetDay());
	sMonth.Format("%.2d", tDate.GetMonth());
	sMonthShort.Format("%d", tDate.GetMonth());
	sYearLong.Format("%.4d", tDate.GetYear());
	sYearShort.Format("%.2d", tDate.GetYear() - 2000);
	sWeekNumber.Format("%.2d", nWeekNumber);
	int nDay = tDate.GetDayOfWeek() - 1;
	if (nDay == 0)
		nDay = 7;
	sDayNumber.Format("%d", nDay);

	COleDateTime dtt(tDate.GetYear(), tDate.GetMonth(), tDate.GetDay(), tDate.GetHour(), tDate.GetMinute(), tDate.GetSecond());
	sDayOfYear.Format("%d", dtt.GetDayOfYear());

	sDate.Replace("YYYY", sYearLong);
	sDate.Replace("YY", sYearShort);
	sDate.Replace("Y", sYearLong);
	sDate.Replace("MM", sMonth);
	sDate.Replace("M", sMonthShort);
	sDate.Replace("DD", sDay);
	sDate.Replace("D", sDayShort);
	sDate.Replace("WW", sWeekNumber);
	sDate.Replace("W", sWeekNumber);
	sDate.Replace("N", GetLocalWeekDayName(tDate, TRUE));
	sDate.Replace("Q", sDayNumber);
	sDate.Replace("J", sDayOfYear);


	if (bAllowSeparators == FALSE) {
		sDate.Replace(":", "");
		sDate.Replace("/", "");
		sDate.Replace("\\", "");
		sDate.Replace("|", "");
		sDate.Replace("<", "");
		sDate.Replace(">", "");
		sDate.Replace("*", "");
		sDate.Replace("?", "");
	}

	

	return sDate;
}

CString CUtils::GetLocalWeekDayName(CTime tDate, BOOL bUseIniWeekNames)
{
	CStringArray aS;
	TCHAR strWeekday[256];

	UINT DayOfWeek[] = {
		LOCALE_SDAYNAME7,   // Sunday
		LOCALE_SDAYNAME1,
		LOCALE_SDAYNAME2,
		LOCALE_SDAYNAME3,
		LOCALE_SDAYNAME4,
		LOCALE_SDAYNAME5,
		LOCALE_SDAYNAME6   // Saturday
	};

	StringSplitter(g_prefs.m_WeekDaysList, ",;", aS);

	if (bUseIniWeekNames && aS.GetCount() != 7)
		bUseIniWeekNames = FALSE;

	if (bUseIniWeekNames)
		return aS[tDate.GetDayOfWeek() - 1];
	else
		::GetLocaleInfo(LOCALE_USER_DEFAULT,   // Get string for day of the week from system
			DayOfWeek[tDate.GetDayOfWeek() - 1],   // Get day of week from CTime
			strWeekday, sizeof(strWeekday));

	CString s(strWeekday);
	return s;
}

CString CUtils::Time2String(CTime tm)
{
	CString s;
	s.Format("%.4d-%.2d-%.2d %.2d:%.2d:%.2d", tm.GetYear(), tm.GetMonth(), tm.GetDay(), tm.GetHour(), tm.GetMinute(), tm.GetSecond());

	return s;
}

CString CUtils::Time2String(CTime tDate, CString sDateFormat)
{
	////////////////////////////
	// Form output date format
	////////////////////////////

	CString sDate = "";
	CString s;
	COleDateTime dtt(tDate.GetYear(), tDate.GetMonth(), tDate.GetDay(), tDate.GetHour(), tDate.GetMinute(), tDate.GetSecond());

	int h = 0;
	while (h < sDateFormat.GetLength()) {
		int hbefore = h;

		if (sDateFormat.Find("J") == h && sDateFormat.Find("JJJ") == -1) {
			int nDayNumber = dtt.GetDayOfYear();

			s.Format("%d", dtt.GetDayOfYear());
			sDate += s;
			h += s.GetLength();
		}

		if (sDateFormat.Find("JJJ") == h && sDateFormat.Find("JJJJ") == -1) {
			s.Format("%.3d", dtt.GetDayOfYear());
			sDate += s;
			h += s.GetLength();
		}
		if (sDateFormat.Find("JJJJ") == h) {
			s.Format("%.4d", dtt.GetDayOfYear());
			sDate += s;
			h += s.GetLength();
		}

		if (sDateFormat.Find("DD") == h) {
			h += 2;
			s.Format("%.2d", tDate.GetDay());
			sDate += s;
		}
		if (sDateFormat.Find("MM") == h) {
			h += 2;
			s.Format("%.2d", tDate.GetMonth());
			sDate += s;
		}
		if (sDateFormat.Find("YY") == h && sDateFormat.Find("YYYY") == -1) {
			h += 2;
			s.Format("%.2d", tDate.GetYear() - 2000);
			sDate += s;
		}
		if (sDateFormat.Find("YYYY") == h) {
			h += 4;
			s.Format("%.4d", tDate.GetYear());
			sDate += s;
		}

		if (sDateFormat.Find("hh") == h) {
			h += 2;
			s.Format("%.2d", tDate.GetHour());
			sDate += s;
		}

		if (sDateFormat.Find("mm") == h) {
			h += 2;
			s.Format("%.2d", tDate.GetMinute());
			sDate += s;
		}

		if (sDateFormat.Find("ss") == h) {
			h += 2;
			s.Format("%.2d", tDate.GetSecond());
			sDate += s;
		}

		if (h == hbefore) {
			sDate += sDateFormat[h];
			h++;
		}
	}

	return sDate;
}

BOOL CUtils::SaveFile(CString sFileName, CString sContent)
{
	DWORD	nBytesWritten;

	HANDLE hFile = ::CreateFile(sFileName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	BOOL ok = ::WriteFile(hFile, sContent.GetBuffer(255), sContent.GetLength(), &nBytesWritten, NULL);
	sContent.ReleaseBuffer();
	::CloseHandle(hFile);
	if (ok == FALSE) {
		::DeleteFile(sFileName);
		return FALSE;
	}
	return TRUE;
}

void CUtils::AlivePoll()
{
	TCHAR	szFinalLine[100];
	DWORD	nBytesWritten;
	SYSTEMTIME	ltime;
	HANDLE hFile;

	hFile = ::CreateFile(g_prefs.m_alivelogFilePath + _T("\\FileDistributorExport.log"), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;

	::GetLocalTime(&ltime);
	_stprintf(szFinalLine, "%.2d-%.2d %.2d:%.2d:%.2d.%.3d\r\n", (int)ltime.wDay, (int)ltime.wMonth, (int)ltime.wHour, (int)ltime.wMinute, (int)ltime.wSecond, (int)ltime.wMilliseconds);
	::WriteFile(hFile, szFinalLine, (DWORD)_tcsclen(szFinalLine), &nBytesWritten, NULL);
	::CloseHandle(hFile);
}


void CUtils::AliveTX()
{
	TCHAR	szFinalLine[100];
	DWORD	nBytesWritten;
	SYSTEMTIME	ltime;
	HANDLE hFile;

	hFile = ::CreateFile(g_prefs.m_alivelogFilePath + _T("\\FileDistributorTX.log"), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;

	::GetLocalTime(&ltime);
	_stprintf(szFinalLine, "%.2d-%.2d %.2d:%.2d:%.2d.%.3d\r\n", (int)ltime.wDay, (int)ltime.wMonth, (int)ltime.wHour, (int)ltime.wMinute, (int)ltime.wSecond, (int)ltime.wMilliseconds);
	::WriteFile(hFile, szFinalLine, (DWORD)_tcsclen(szFinalLine), &nBytesWritten, NULL);
	::CloseHandle(hFile);
}

CString CUtils::EncodeBlowFish(CString sInput)
{
	CkCrypt2 crypt;

	if (crypt.UnlockComponent("INFRAL.CB1092019_4czEsHZS615x") == false)
		return sInput;

	//  Attention: use "blowfish2" for the algorithm name:
	crypt.put_CryptAlgorithm("blowfish2");

	//  CipherMode may be "ecb", "cbc", or "cfb"
	crypt.put_CipherMode("cbc");

	//  KeyLength (in bits) may be a number between 32 and 448.
	//  128-bits is usually sufficient.  The KeyLength must be a
	//  multiple of 8.
	crypt.put_KeyLength(128);

	//  The padding scheme determines the contents of the bytes
	//  that are added to pad the result to a multiple of the
	//  encryption algorithm's block size.  Blowfish has a block
	//  size of 8 bytes, so encrypted output is always
	//  a multiple of 8.
	crypt.put_PaddingScheme(0);

	//  EncodingMode specifies the encoding of the output for
	//  encryption, and the input for decryption.
	//  It may be "hex", "url", "base64", or "quoted-printable".
	crypt.put_EncodingMode("hex");

	//  An initialization vector is required if using CBC or CFB modes.
	//  ECB mode does not use an IV.
	//  The length of the IV is equal to the algorithm's block size.
	//  It is NOT equal to the length of the key.
	const char *ivHex = "0001020304050607";
	crypt.SetEncodedIV(ivHex, "hex");

	//  The secret key must equal the size of the key.  For
	//  256-bit encryption, the binary secret key is 32 bytes.
	//  For 128-bit encryption, the binary secret key is 16 bytes.
	const char *keyHex = "000102030405060708090A0B0C0D0E0F";
	crypt.SetEncodedKey(keyHex, "hex");

	const char *encStr = crypt.encryptStringENC(sInput);
	CString sOutput(encStr);

	return sOutput;
}


CString CUtils::DecodeBlowFish(CString sInput)
{
	CkCrypt2 crypt;

	if (crypt.UnlockComponent("INFRAL.CB1092019_4czEsHZS615x") == false)
		return sInput;

	//  Attention: use "blowfish2" for the algorithm name:
	crypt.put_CryptAlgorithm("blowfish2");

	//  CipherMode may be "ecb", "cbc", or "cfb"
	crypt.put_CipherMode("cbc");

	//  KeyLength (in bits) may be a number between 32 and 448.
	//  128-bits is usually sufficient.  The KeyLength must be a
	//  multiple of 8.
	crypt.put_KeyLength(128);

	//  The padding scheme determines the contents of the bytes
	//  that are added to pad the result to a multiple of the
	//  encryption algorithm's block size.  Blowfish has a block
	//  size of 8 bytes, so encrypted output is always
	//  a multiple of 8.
	crypt.put_PaddingScheme(0);

	//  EncodingMode specifies the encoding of the output for
	//  encryption, and the input for decryption.
	//  It may be "hex", "url", "base64", or "quoted-printable".
	crypt.put_EncodingMode("hex");

	//  An initialization vector is required if using CBC or CFB modes.
	//  ECB mode does not use an IV.
	//  The length of the IV is equal to the algorithm's block size.
	//  It is NOT equal to the length of the key.
	const char *ivHex = "0001020304050607";
	crypt.SetEncodedIV(ivHex, "hex");

	//  The secret key must equal the size of the key.  For
	//  256-bit encryption, the binary secret key is 32 bytes.
	//  For 128-bit encryption, the binary secret key is 16 bytes.
	const char *keyHex = "000102030405060708090A0B0C0D0E0F";
	crypt.SetEncodedKey(keyHex, "hex");

	const char *decStr = crypt.decryptStringENC(sInput);
	CString sOutput(decStr);

	return sOutput;
}


CString CUtils::GetTempFolder()
{
	TCHAR lpTempPathBuffer[MAX_PATH];

	if (GetTempPath(MAX_PATH, lpTempPathBuffer) == 0)
		return  _T("c:\\temp");
	CString s(lpTempPathBuffer);

	return s;
}

BOOL CUtils::Load(CString  csFileName, CString &csDoc)
{

	CFile file;
	if (!file.Open(csFileName, CFile::modeRead))
		return FALSE;
	int nLength = (int)file.GetLength();

#if defined(_UNICODE)
	// Allocate Buffer for UTF-8 file data
	unsigned char* pBuffer = new unsigned char[nLength + 1];
	nLength = file.Read(pBuffer, nLength);
	pBuffer[nLength] = '\0';

	// Convert file from UTF-8 to Windows UNICODE (AKA UCS-2)
	int nWideLength = MultiByteToWideChar(CP_UTF8, 0, (const char*)pBuffer, nLength, NULL, 0);
	nLength = MultiByteToWideChar(CP_UTF8, 0, (const char*)pBuffer, nLength,
		csDoc.GetBuffer(nWideLength), nWideLength);
	ASSERT(nLength == nWideLength);
	delete[] pBuffer;
#else
	nLength = file.Read(csDoc.GetBuffer(nLength), nLength);
#endif
	csDoc.ReleaseBuffer(nLength);
	file.Close();

	return TRUE;
}

BOOL CUtils::Save(CString  csFileName, CString csDoc)
{
	int nLength = csDoc.GetLength();
	CFile file;
	if (!file.Open(csFileName, CFile::modeWrite | CFile::modeCreate))
		return FALSE;
#if defined( _UNICODE )
	int nUTF8Len = WideCharToMultiByte(CP_UTF8, 0, csDoc, nLength, NULL, 0, NULL, NULL);
	char* pBuffer = new char[nUTF8Len + 1];
	nLength = WideCharToMultiByte(CP_UTF8, 0, csDoc, nLength, pBuffer, nUTF8Len + 1, NULL, NULL);
	file.Write(pBuffer, nLength);
	delete pBuffer;
#else
	file.Write((LPCTSTR)csDoc, nLength);
#endif
	file.Close();
	return TRUE;
}




DWORD CUtils::GenerateGUID(int nSpooler, int nTick)
{
	CTime tNow = CTime::GetCurrentTime();
	return nSpooler + 100 * tNow.GetSecond() + 10000 * tNow.GetMinute() + 1000000 * tNow.GetHour() + 100000000 * nTick;
}




CString CUtils::FormNameChannel(CJobAttributes &job, CHANNELGROUPSTRUCT &channelGroup, int nDaysToAdvance)
{
	CString sFinalName = FormName(channelGroup.m_transmitnameconv, channelGroup.m_transmitdateformat, "", job, channelGroup.m_transmitnameuseabbr, nDaysToAdvance);

	if (channelGroup.m_transmitnameoptions & TXNAMEOPTION_REMOVESPACES)
		sFinalName.Replace(" ", "");
	if (channelGroup.m_transmitnameoptions & TXNAMEOPTION_ALLSMALLCPAS)
		sFinalName.MakeLower();

	sFinalName.Replace(":", "");
	sFinalName.Replace("/", "");
	sFinalName.Replace("?", "");
	sFinalName.Replace("\\", "");
	sFinalName.Replace("<", "");
	sFinalName.Replace(">", "");
	sFinalName.Replace("|", "");
	sFinalName.Replace("*", "");


	return sFinalName;
}



CString CUtils::FormName(CString sOrgMask, CString sDateFormat, CString sTimeFormat, 
	                                                           CJobAttributes &job, int bAbbr, int nDaysToAdvance)
{
	BOOL	ret = FALSE;

	CString sCompleteName = sOrgMask;
	CString s;

	if (sOrgMask == "")
		return "";


	sCompleteName.Replace("%M1", job.m_miscstring1);
	sCompleteName.Replace("%M2", job.m_miscstring2);
	sCompleteName.Replace("%M3", job.m_miscstring3); // Used for channelidlist from WebCenter 

	s = g_prefs.GetPublicationName(job.m_publicationID);
	sCompleteName.Replace("%P", bAbbr ? g_prefs.LookupAbbreviationFromName(_T("Publication"), s) : s);
	sCompleteName.Replace("%p", s);

	CString sd, sv;
	for (int i = 1; i < 10; i++) {
		sd.Format("%%%dP", i);
		sv = bAbbr ? g_prefs.LookupAbbreviationFromName(_T("Publication"), s) : s;
		sv += "0000000000";
		sCompleteName.Replace(sd, sv.Left(i));
	}

	CTime tPubD = job.m_pubdate;

	if (nDaysToAdvance != 0) {
		CTimeSpan ts(nDaysToAdvance, 0, 0, 0);
		tPubD = tPubD + ts;
	}
	CString sPubDate = Date2StringEx(tPubD, sDateFormat, TRUE, job.m_weekreference);
	sCompleteName.Replace("%D", sPubDate);

	//if (g_prefs.m_profversion == FALSE) {
	s = g_prefs.GetEditionName(job.m_editionID);
	sCompleteName.Replace("%e", s);
	sCompleteName.Replace("%E", bAbbr ? g_prefs.LookupAbbreviationFromName(_T("Edition"), s) : s);

	for (int i = 1; i < 10; i++) {
		sd.Format("%%%dE", i);
		sv = bAbbr ? g_prefs.LookupNameFromAbbreviation(_T("Edition"), s) : s;
		sv += "0000000000";
		sCompleteName.Replace(sd, sv.Left(i));
	}

	for (int i = 1; i < 10; i++) {
		sd.Format("%%-%dE", i);
		sv = bAbbr ? g_prefs.LookupNameFromAbbreviation(_T("Edition"), s) : s;
		sv = "0000000000" + sv;
		sCompleteName.Replace(sd, sv.Right(i));
	}
	//	}

	s = g_prefs.GetSectionName(job.m_sectionID);
	sCompleteName.Replace("%S", bAbbr ? g_prefs.LookupAbbreviationFromName(_T("Section"), s) : s);
	sCompleteName.Replace("%s", s);

	for (int i = 1; i < 10; i++) {
		sd.Format("%%%dS", i);
		sv = bAbbr ? g_prefs.LookupAbbreviationFromName(_T("Section"), s) : s;
		sv += "0000000000";
		sCompleteName.Replace(sd, sv.Left(i));
	}

	CString sPageNumberString = job.m_pagename;
	CString sSuffix = job.m_pagename.Right(1);
	if (isdigit(sSuffix[0]))
		sSuffix = "";

	int nPageNameAsNumber = atoi(sPageNumberString);
	if (nPageNameAsNumber > 0) {
		if (sCompleteName.Find("%N") != -1 || sCompleteName.Find("%1N") != -1) {

			sCompleteName.Replace("%N", sPageNumberString + sSuffix);
		}
		if (sCompleteName.Find("%2N") != -1) {
			sPageNumberString.Format("%.2d", nPageNameAsNumber);
			sCompleteName.Replace("%2N", sPageNumberString + sSuffix);
		}
		if (sCompleteName.Find("%3N") != -1) {
			sPageNumberString.Format("%.3d", nPageNameAsNumber);
			sCompleteName.Replace("%3N", sPageNumberString + sSuffix);
		}
		if (sCompleteName.Find("%4N") != -1) {
			sPageNumberString.Format("%.4d", nPageNameAsNumber);
			sCompleteName.Replace("%4N", sPageNumberString + sSuffix);
		}
	}

	if (sCompleteName.Find("%M") != -1 || sCompleteName.Find("%1M") != -1) {
		sPageNumberString.Format("%d", job.m_pagination);
		sCompleteName.Replace("%M", sPageNumberString);
	}
	if (sCompleteName.Find("%2M") != -1) {
		sPageNumberString.Format("%.2d", job.m_pagination);
		sCompleteName.Replace("%2M", sPageNumberString);
	}
	if (sCompleteName.Find("%3M") != -1) {
		sPageNumberString.Format("%.3d", job.m_pagination);
		sCompleteName.Replace("%3M", sPageNumberString);
	}
	if (sCompleteName.Find("%4M") != -1) {
		sPageNumberString.Format("%.4d", job.m_pagination);
		sCompleteName.Replace("%4M", sPageNumberString);
	}

	if (sCompleteName.Find("%G") != -1 || sCompleteName.Find("%1G") != -1) {
		sPageNumberString.Format("%d", job.m_pageindex);
		sCompleteName.Replace("%G", sPageNumberString);
	}
	if (sCompleteName.Find("%2G") != -1) {
		sPageNumberString.Format("%.2d", job.m_pageindex);
		sCompleteName.Replace("%2G", sPageNumberString);
	}
	if (sCompleteName.Find("%3G") != -1) {
		sPageNumberString.Format("%.3d", job.m_pageindex);
		sCompleteName.Replace("%3G", sPageNumberString);
	}
	if (sCompleteName.Find("%4G") != -1) {
		sPageNumberString.Format("%.4d", job.m_pageindex);
		sCompleteName.Replace("%4G", sPageNumberString);
	}



	sCompleteName.Replace("%C", bAbbr ? g_prefs.LookupAbbreviationFromName(_T("Color"), job.m_colorname) : job.m_colorname);
	sCompleteName.Replace("%c", job.m_colorname);

	/* = g_prefs.GetLocationName(pJob->m_locationID);
	sCompleteName.Replace("%L", bAbbr ? g_prefs.LookupAbbreviationFromName(_T("Location"), s, bAbbr) : s);
	sCompleteName.Replace("%l", s);
	for (int i = 1; i < 10; i++) {
		sd.Format("%%%dL", i);
		CString sv = bAbbr ? g_prefs.LookupAbbreviationFromName(_T("Location"), s, bAbbr) : s;
		sv += "0000000000";
		sCompleteName.Replace(sd, sv.Left(i));
	}*/

	sCompleteName.Replace("%|", job.m_pressruncomment);
	sCompleteName.Replace("%.", job.m_pressruninkcomment);
	sCompleteName.Replace("%$", job.m_pressrunordernumber);


	sCompleteName.Replace("%K", job.m_comment);


	sCompleteName.Replace("%F", job.m_filename);
	sCompleteName.Replace("%f", GetFileName(job.m_filename, TRUE));

	sCompleteName.Replace("%^", job.m_customername);
	sCompleteName.Replace("%¨", job.m_customeralias);

	//sCompleteName.Replace("%Q", pJob->m_planname);
	//sCompleteName.Replace("%Q", g_prefs.GetPublisherName(job.m_publisherID));

	sCompleteName.Replace("%¤", g_prefs.GetExtendedAlias(job.m_publicationID));

	if (sCompleteName.Find("%*") != -1)
		sCompleteName.Replace("%*", GetLocalWeekDayName(job.m_pubdate, TRUE));

	s.Format("%d", job.m_version);
	sCompleteName.Replace("%V", s);

	s.Format("%d", job.m_pressrunID);
	sCompleteName.Replace("%R", s);
	for (int i = 1; i < 10; i++) {
		sd.Format("%%%dR", i);
		sv.Format("0000000000%d", job.m_pressrunID);
		sCompleteName.Replace(sd, sv.Right(i));
	}

	//	sCompleteName.Replace("%|", pJob->m_pressruncomment);		 
	//	sCompleteName.Replace("%.", pJob->m_pressruninkcomment);		
	//	sCompleteName.Replace("%$", pJob->m_pressrunordernumber);				

	CTime tNow = CTime::GetCurrentTime();
	s.Format("%.4d%-2d%-2dT%.2d:%.2d:%.2d.0", tNow.GetYear(), tNow.GetMonth(), tNow.GetDay(), tNow.GetHour(), tNow.GetMinute(), tNow.GetSecond());
	sCompleteName.Replace("%:", s);

	PUBLICATIONSTRUCT *pPub = g_prefs.GetPublicationStruct(job.m_publicationID);
	CString sPubComment = _T("");
	if (pPub != NULL)
		sPubComment = job.m_annumtext;
	sCompleteName.Replace("%,", sPubComment);


	s = Time2String(tNow, sTimeFormat);
	sCompleteName.Replace("%J", s);

	return sCompleteName;
}


int CUtils::GetWeekNumberFromDate(CTime tDate)
{

	//	::GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_IFIRSTWEEKOFYEAR, lpLCData , cchData)

	int nNumDaysOfYear = 0, nYY = 0, nC = 0, nG = 0, nJan1Weekday = 0;
	int nH = 0, nWeekday = 0, nYearNumber = 0, nWeekNumber = 0, nI = 0, nJ = 0;
	bool bIsLeapYear = false;
	bool bIsPrevYear = false;
	int nMonthArray[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

	int nYear = tDate.GetYear();
	nYearNumber = nYear;
	int nMonth = tDate.GetMonth();
	int nDay = tDate.GetDay();

	bIsLeapYear = (nYear % 4 == 0) && ((nYear % 100 != 0) || (nYear % 400 == 0));
	// Kolla om det föregående året är skottår
	bIsPrevYear = ((nYear - 1) % 4 == 0) && (((nYear - 1) % 100 != 0) || ((nYear - 1) % 400 == 0));

	// Kolla hur många dagar på året som har gått
	nNumDaysOfYear = nDay + nMonthArray[nMonth - 1];

	// Om det är skottår och månaden > 2 så öka antalet dagar med 1
	if (bIsLeapYear && nMonth > 2)
		nNumDaysOfYear++;

	// Ta reda på vilken veckodag 1 Januari var.
	nYY = (nYear - 1) % 100;
	nC = (nYear - 1) - nYY;
	nG = nYY + nYY / 4;
	nJan1Weekday = 1 + (((((nC / 100) % 4) * 5) + nG) % 7);

	// Ta reda på veckodagen
	nH = nNumDaysOfYear + (nJan1Weekday - 1);
	nWeekday = 1 + ((nH - 1) % 7);

	// Kolla om Y-M-D infaller i nYear-1, Veckonummer 52 eller 53
	if (nNumDaysOfYear <= (8 - nJan1Weekday) && nJan1Weekday > 4) {
		nYearNumber = nYear - 1;

		if (nJan1Weekday == 5 || (nJan1Weekday == 6 && bIsPrevYear))
			nWeekNumber = 53;
		else
			nWeekNumber = 52;
	}

	// Kolla om Y-M-D infaller i nYear+1, Veckonummer 1
	if (nYearNumber == nYear) {
		nI = bIsLeapYear ? 366 : 365;

		if ((nI - nNumDaysOfYear) < (4 - nWeekday)) {
			nYearNumber = nYear + 1;
			nWeekNumber = 1;
		}
	}

	// Kolla om Y-M-D infaller i nYearNumber, Veckonummer 1 till 53
	if (nYearNumber == nYear) {
		nJ = nNumDaysOfYear + (7 - nWeekday) + (nJan1Weekday - 1);
		nWeekNumber = nJ / 7;

		if (nJan1Weekday > 4)
			nWeekNumber--;
	}

	return nWeekNumber;
}


CTime CUtils::GetDateFromWeekNumber(int nYear, int nWeekNumber, int nDayOfWeek)
{
	// Return the day that begins a week in this country (in denmark this is always a monday (=0)).
	// Using local settings found in control panel.
/*	TCHAR szLocalInfo[2];
	BOOL bResult = ::GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_IFIRSTDAYOFWEEK, szLocalInfo, 2);
	int nDayFirstOfWeek = atoi(szLocalInfo);

	// Danish			CTime
	// 0: Monday		1:Sunday
	// 1: Tuesday		2:Monday
	// .                .
	// 6: Sunday		7:Saturday

	SetCyclic(nDayFirstOfWeek, 2, 1, 7); // This is done to match the day-numbering in the CTime class.



	CTime fstday(now.GetYear(),1,1,0,0,0);

	int daystoadd = (nWeekNumber-1)*7 + nDayOfWeek - fstday.GetDayOfWeek());
	fstday += CTimeSpan(daystoadd,0,0,0);
*/
	CTime now = CTime::GetCurrentTime();

	int nDay = 0;
	int nMonth = 0;

	(*pDateOfWeek)(nYear, nWeekNumber, nDayOfWeek, &nDay, &nMonth);

	CTime dt;
	if (nMonth > 0 && nMonth < 13 && nDay > 0 && nDay < 32) {
		try {
			CTime dt2(nYear, nMonth, nDay, 0, 0, 0);
			dt = dt2;
		}
		catch (CException *ex) {
			Logprintf("ERROR: Bad week-number to date convertion (week:%d, dayofweek=%d, year=%d returned day=%d, month=%d)", nWeekNumber, nDayOfWeek, nYear, nDay, nMonth);
		}
		Logprintf("INFO:  Week number conversion returned %.2d/%.2d/%.4d", dt.GetDay(), dt.GetMonth(), dt.GetYear());
	}
	else
		Logprintf("ERROR: Bad week-number to date convertion (week:%d, dayofweek=%d, year=%d returned day=%d, month=%d)", nWeekNumber, nDayOfWeek, nYear, nDay, nMonth);

	return dt;
}

CString CUtils::GetRawMaskEx(CString sOrgMask, CString &sDateFormat)
{
	int n = 0, nIndex = -1;
	CString sNamingMask = sOrgMask.MakeLower();
	CString sResult = _T("");
	sDateFormat = _T("");

	while (n < sNamingMask.GetLength()) {

		if (sNamingMask[n] != _TCHAR('%')) {
			sResult += sNamingMask.Mid(n, 1);
			n++;
			continue;
		}

		if (n + 1 < sNamingMask.GetLength())
			n++;

		// See if we have repeater instruction (digits)
		int reps = atoi(sNamingMask.Mid(n));
		if (reps > 0) {
			// Skip over digits
			while (n < sNamingMask.GetLength()) {
				if (isdigit((int)(sNamingMask[n])))
					n++;
				else
					break;
			}
		}
		else {
			reps = 1;
		}

		if (n >= sNamingMask.GetLength())
			break; // no more string..

		// Store id character	
		CString id = sNamingMask.Mid(n, 1);
		for (int i = 0; i < reps; i++)
			sResult += id.MakeUpper();

		// Skip over id
		n++;

		//	See if if have special instructions in brackets
		if (n >= sNamingMask.GetLength())
			break;

		if (sNamingMask[n] == _TCHAR('[')) {

			int m = sNamingMask.Find(_TCHAR(']'), n);
			if (m == -1)
				continue;	// End bracket not found

			sDateFormat = sNamingMask.Mid(n + 1, m - n - 1);
			if (id.MakeUpper() == _T("D"))
				for (int i = 0; i < sDateFormat.GetLength() - 1; i++)
					sResult += _T("D");
			n = m + 1;
		}
	}
	return sResult;
}


int CUtils::PageName2PageNumber(CString sPageName)
{
	int n;
	if (sPageName == _T(""))
		return 0;
	sPageName.MakeUpper();
	
	TCHAR *sz = sPageName.GetBuffer(255);
	if (!isdigit(*sz))
		n = _tstoi(sz + 1) + 10000 * (*sz - _T('A') + 1);
	else
		n = _tstoi(sz);

	return n;
}


BOOL CUtils::Decompose(CString sJobName, INPUTPROCESSSTRUCT *pInput, CJobAttributes *pJob,
															BOOL &isGray, BOOL &bHasWeekNumber, BOOL &bHasVersion)
{
	BOOL	ret = FALSE;
	CString sDateFormat = "";
	bHasVersion = FALSE;
	bHasWeekNumber = FALSE;

	pJob->m_panomate = _T("");

	CString sRawMask = GetRawMaskEx(pInput->m_namingmask, sDateFormat);
	sDateFormat.MakeUpper();
	//Logprintf("INFO: Mask used %s", sRawMask);

	CTime	tDate = pJob->m_pubdate;
	CTime	tDateWeekStart = pJob->m_pubdate;
	CTime	tDateWeekEnd = pJob->m_pubdate;
	int		nWeekReference = 0;
	CString sPublication = _T("");
	CString sEdition = _T("");
	CString sSection = _T("");

	CString sPageName = _T("");
	CString sColor = pJob->m_colorname;
	CString sVersion = _T("");

	CString sDate = _T("");
	CString sComment = _T("");

	CString sCopies = _T("");

	CString sOffset = _T("");
	CString sPlanName = _T("");
	CString sPagination = _T("");
	CString sPageIndex = _T("");
	CString sMarkGroupSymbol = _T("");
	CString sPanoPage = _T("");

	CString sMiscString2 = _T("");
	CString sMiscString3 = _T("");
	CString sMiscString1 = _T("");


	// Fixed file name??
	BOOL bIsFullyFixed = TRUE;
	BOOL bMustHaveDate = FALSE;
	BOOL bHasJobName = sRawMask.FindOneOf("J") != -1;

	if (sRawMask.FindOneOf(pInput->m_separators) != -1) {
		bIsFullyFixed = FALSE;
		sRawMask.Replace("DDDDDDDD", "D");
		sRawMask.Replace("DDDDDDD", "D");
		sRawMask.Replace("DDDDDD", "D");
		sRawMask.Replace("DDDDD", "D");
		sRawMask.Replace("DDDD", "D");
		sRawMask.Replace("DDD", "D");
		sRawMask.Replace("DD", "D");
	}

	int n = 0, m = 0, mm = 0;
	if (bIsFullyFixed) {
		// Standard fixed size naming scheme
		while (n < sRawMask.GetLength()) {
			switch (sRawMask[n]) {
			case 'P':
				sPublication += sJobName[n];
				break;
			case 'S':
				sSection += sJobName[n];
				break;
			case 'E':
				sEdition += sJobName[n];
				break;
			case 'D':
				sDate += sJobName[n];
				bMustHaveDate = TRUE;
				break;
			case 'N':
				sPageName += sJobName[n];
				break;
			case 'U':
				sPagination += sJobName[n];
				break;
			case 'A':
				sPageIndex += sJobName[n];
				break;
			case 'Q':
				sPlanName += sJobName[n];
				break;
			case 'C':
				sColor += sJobName[n];
				break;
			case 'L':
				//sLocation += sJobName[n];
				break;
			case 'G':
				//sLocationGroup += sJobName[n];
				break;
			case '#':
				sCopies += sJobName[n];
				break;
			case 'K':
			case 'X':
			case '!':
				sComment += sJobName[n];
				break;
			case 'R':
			
			case 'T':
				//sTemplate += sJobName[n];
				break;
			case 'M':
				sMarkGroupSymbol += sJobName[n];
				break;
			case 'Z':
				sOffset += sJobName[n];
				break;
			case 'Y':
				sPanoPage += sJobName[n];
				break;
			case 'W':
				//sPress += sJobName[n];
				break;
			case 'V':
				sVersion += sJobName[n];
				break;
			case '<':
				sMiscString1 += sJobName[n];
				break;
			case '>':
				sMiscString2 += sJobName[n];
				break;
			case '(':
				sMiscString3 += sJobName[n];
				break;

			default:

				// Don't care character
				break;
			}
			n++;
		}
	}
	else {
		// Separated naming scheme
		int nIdx = 0, nIdx2 = 0;
		int m = 0, mm = 0;
		CString sIsolatedName;
		BOOL bWasSeparator = FALSE;
		while (n < sRawMask.GetLength() && m < sJobName.GetLength()) {

			// Isolate next id (until next delimiter)
			mm = sJobName.Mid(m).FindOneOf(pInput->m_separators);	// Get next delimiter in job name		
			if (mm == 0) {
				// Double separator detected
				m += 1;
				if (sRawMask.Mid(n).FindOneOf(pInput->m_separators) != -1)
					n++;
				continue;
			}
			if (mm != -1)
				//sIsolatedName = sJobName.Mid(m,mm-m+m);
				sIsolatedName = sJobName.Mid(m, mm);
			else
				sIsolatedName = sJobName.Mid(m);

			switch (sRawMask[n]) {
			case 'P':
				sPublication = sIsolatedName;
				break;
			case 'S':
				sSection = sIsolatedName;
				break;
			case 'E':
				sEdition = sIsolatedName;
				break;
			case 'D':
				sDate = sIsolatedName;
				bMustHaveDate = TRUE;
				break;
			case 'N':
				sPageName = sIsolatedName;
				break;
			case 'U':
				sPagination = sIsolatedName;
				break;
			case 'A':
				sPageIndex = sIsolatedName;
				break;
			case 'C':
				sColor = sIsolatedName;
				break;
			case 'Q':
				sPlanName = sIsolatedName;
				break;
			case 'L':
				//sLocation = sIsolatedName;
				break;
			case 'G':
				//sLocationGroup = sIsolatedName;
				break;
			case '#':
				sCopies = sIsolatedName;
				break;
			case 'K':
			case 'X':
			case '!':
				sComment = sIsolatedName;
				break;
			case 'R':
			case 'I':
				//sRun = sIsolatedName;
				break;
			case 'T':
				//sTemplate = sIsolatedName;
				break;
			case 'M':
				sMarkGroupSymbol = sIsolatedName;
				break;
			case 'Z':
				sOffset = sIsolatedName;
				break;

			case 'W':
				//sPress = sIsolatedName;
				break;
			case 'V':
				sVersion = sIsolatedName;
				break;

			case 'Y':
				sPanoPage = sIsolatedName;
				break;
			case '<':
				sMiscString1 = sIsolatedName;
				break;
			case '>':
				sMiscString2 = sIsolatedName;
				break;
			case '(':
				sMiscString3 = sIsolatedName;
				break;

			default:
				// Don't care character
				break;
			}
			m += mm + 1;
			n++;
			if (sRawMask.Mid(n).FindOneOf(pInput->m_separators) != -1)
				n++;
		}
	}


	if (_tstoi(sPlanName) > 0)
		sPanoPage == _T("");

	int nPanoMate = _tstoi(sPanoPage);
	sPanoPage.Format("%d", nPanoMate);


	// Set Defaults prior to validity test (will store values back later)
	if (sPublication == "")
		sPublication = g_prefs.GetPublicationName(pJob->m_publicationID);
	if (sEdition == "")
		sEdition = g_prefs.GetEditionName(pJob->m_editionID);
	if (sSection == "")
		sSection = g_prefs.GetSectionName(pJob->m_sectionID);

	if (sCopies == "") {
		CString tmp;
		tmp.Format("%d", pJob->m_copies);
		sCopies = tmp;
	}

	if (atoi(sVersion) > 0) {
		bHasVersion = TRUE;
		pJob->m_version = atoi(sVersion);
	}

	// Get date according to format specifier
//	if (sDate.GetLength() < sDateFormat.GetLength()) 
//		sDate == "";
	CTime tAuxDate(1975, 1, 1, 0, 0, 0);
	BOOL  bHasValidAuxDate = FALSE;

	if (sDate != "" && sDateFormat != "" && bMustHaveDate) {
		n = 0;
		CString sDay = "", sMonth = "", sYear = "";

		CString sWeek = "";
		CTime today = CTime::GetCurrentTime();

		sDateFormat.MakeUpper();
		while (n < sDateFormat.GetLength() && n < sDate.GetLength()) {
			switch (sDateFormat[n]) {
			case 'D':
				sDay += sDate[n];
				break;
			case 'M':
				sMonth += sDate[n];
				break;
			case 'Y':
				sYear += sDate[n];
				break;
			case 'W':
				sWeek += sDate[n];
				break;
			default:
				break;
			}
			n++;
		}

		// Handle phony year 99 or 9999
		if ((sDateFormat == "DDMMYY" || sDateFormat == "YYMMDD" || sDateFormat == "DDMMYYYY" || sDateFormat == "YYYYMMDD") && (sYear == "99" || sYear == "9999") && sMonth != "99") {
			sYear = "";
			sDateFormat.Replace("YY", "");
		}

		// Handle phony year 99 or 9999
		if ((sDateFormat == "DDMMYY" || sDateFormat == "YYMMDD" || sDateFormat == "DDMMYYYY" || sDateFormat == "YYYYMMDD") && (sYear == "XX" || sYear == "XXXX") && sMonth != "XX") {
			sYear = "";
			sDateFormat.Replace("YY", "");
		}

		// Handle phony month 99
		if ((sDateFormat == "DDMM" || sDateFormat == "MMDD" || sDateFormat == "DDMMYY" || sDateFormat == "YYMMDD" || sDateFormat == "DDMMYYYY" || sDateFormat == "YYYYMMDD") && (sMonth == "99" || sMonth == "XX")) {
			sMonth = "";
			sYear = "";
			sDateFormat = "DD";
		}

		CString sAuxDay = sDay, sAuxMonth = sMonth, sAuxYear = sYear;


		int nThisWeekNumber = GetWeekNumberFromDate(today);

		// Handle week number
		bHasWeekNumber = FALSE;
		if (atoi(sDate) > 0 && (((sDateFormat == "MMDD" || sDateFormat == "DDMM") && sDate[0] == '0' && sDate[1] == '0') || sWeek != "")) {

			int weeknumber = atoi(sDate);

			pJob->m_weekreference = weeknumber; // Overrules date range if wekk reference mode selected 

			if (weeknumber < 1 || weeknumber > 52)
				weeknumber = 1;

			BOOL bThisYear = weeknumber >= nThisWeekNumber;

			if (g_prefs.m_dayofweek > 0)
				tDate = GetDateFromWeekNumber(bThisYear ? today.GetYear() : today.GetYear() + 1, weeknumber, g_prefs.m_dayofweek);
			else {
				tDateWeekStart = GetDateFromWeekNumber(bThisYear ? today.GetYear() : today.GetYear() + 1, weeknumber, 1);
				tDateWeekEnd = GetDateFromWeekNumber(bThisYear ? today.GetYear() : today.GetYear() + 1, weeknumber, 7);
				tDate = tDateWeekStart;
				bHasWeekNumber = TRUE;
			}

			sDay.Format("%.2d", tDate.GetDay());
			sMonth.Format("%.2d", tDate.GetMonth());
			sYear.Format("%.4d", tDate.GetYear());

			CTime tAux = GetDateFromWeekNumber(bThisYear ? today.GetYear() - 1 : today.GetYear(), weeknumber, g_prefs.m_dayofweek > 0 ? g_prefs.m_dayofweek : 1);
			sAuxDay.Format("%.2d", tAux.GetDay());
			sAuxMonth.Format("%.2d", tAux.GetMonth());
			sAuxYear.Format("%.4d", tAux.GetYear());

		}
		else {

			// Day not in date format - default to day 1

			if (sDay == "" || atoi(sDay) == 0) {
				sDay = "01";
				sAuxDay = "01";
			}
			// Month not in date format - default to current month (or roll over to next month if last day in month)

			if (sMonth == "" || atoi(sMonth) == 0) {

				// Day is after current day

				if (atoi(sDay) >= today.GetDay()) {

					// Day after current date-day (future) - use current month/year

					sMonth.Format("%.2d", today.GetMonth());
					sYear.Format("%.4d", today.GetYear());

					// Aux date - set to previous month

					if (today.GetMonth() - 1 >= 1) {
						sAuxMonth.Format("%.2d", today.GetMonth() - 1);
						sAuxYear = sYear;
					}
					else {
						sAuxMonth = "01";
						sAuxYear.Format("%.4d", today.GetYear() - 1);
					}

				}
				else {

					// Day before current day - use next month

					if (today.GetMonth() < 12) {

						// Same year as now

						sMonth.Format("%.2d", today.GetMonth() + 1);
						sYear.Format("%.4d", today.GetYear());

						sAuxMonth.Format("%.2d", today.GetMonth());
						sAuxYear = sYear;

					}
					else {

						// Next year
						sMonth = "01";
						sYear.Format("%.4d", today.GetYear() + 1);

						sAuxMonth.Format("%.2d", today.GetMonth());
						sAuxYear.Format("%.4d", today.GetYear());
					}
				}
			}
		}

		// Year not in date format - default to current year (or roll over if 31/12+1)

		if (sYear == "" || atoi(sYear) == 0) {
			try {
				CTime tNewdate = CTime(today.GetYear(), atoi(sMonth), atoi(sDay), 23, 59, 59);
				if (tNewdate >= today) {
					sYear.Format("%.4d", today.GetYear());
					sAuxYear.Format("%.4d", today.GetYear() - 1);
				}
				else {
					sYear.Format("%.4d", today.GetYear() + 1);
					sAuxYear.Format("%.4d", today.GetYear());
				}
			}
			catch (CException *ex) {
				Logprintf("ERROR: Illegal date format %s-%s-%s in file %s", today.GetYear(), sMonth, sDay, sJobName);
				return FALSE;
			}

		}

		if (atoi(sYear) < 100)
			sYear = "20" + sYear;
		if (atoi(sAuxYear) < 100)
			sAuxYear = "20" + sAuxYear;

		sDate.Format("%s/%s/%s", sMonth, sDay, sYear);
		//	Logprintf("Date found %s",sDate);
			//tDate.Format("%m/%d/%y", sMonth, sDay, sYear);

		try {
			CTime  tm(atoi(sYear), atoi(sMonth), atoi(sDay), 0, 0, 0);
			tDate = tm;

		}
		catch (CException *ex) {
			Logprintf("ERROR: Illegal date format %s-%s-%s in file %s", sYear, sMonth, sDay, sJobName);
			return FALSE;
		}

		try {

			tAuxDate = CTime(atoi(sAuxYear), atoi(sAuxMonth), atoi(sAuxDay), 0, 0, 0);
			bHasValidAuxDate = TRUE;
		}
		catch (CException *ex) {
			Logprintf("ERROR: Illegal auxillary date format %s-%s-%s in file %s", sAuxYear, sAuxMonth, sAuxDay, sJobName);
			return FALSE;
		}

		pJob->m_pubdatefound = TRUE;
	}
	else {

		pJob->m_pubdatefound = bMustHaveDate && (sDate != "");
		sDate = tDate.Format("%m/%d/%Y");
	}




	// Truncate leading zeros
	int bLeadingZero = 0;
	for (int i = 0; i < sPageName.GetLength(); i++) {
		if (sPageName[i] == '0')
			bLeadingZero++;
		else
			break;
	}
	if (bLeadingZero > 0)
		sPageName = sPageName.Mid(bLeadingZero);





	// Offset pagename/pagination/pageindex if offset in filename

	if (sOffset != _T("")) {
		if (atoi(sOffset) > 0 && atoi(sPageName) > 0) {
			int nPageName = atoi(sPageName);
			int nOffset = atoi(sOffset);
			CString sRes;
			sRes.Format("%d", nPageName + nOffset - 1);
			sPageName = sRes;
			if (atoi(sPagination) > 0)
				sPagination = sRes;
			if (atoi(sPageIndex) > 0)
				sPageIndex = sRes;
			Logprintf("INFO: Offsetting pagenumber to ", sRes);
		}
	}

	int nPagination = atoi(sPagination);
	int nPageIndex = atoi(sPageIndex);

	if (sEdition == "" && g_prefs.m_EditionList.GetCount() == 1)
		sEdition = g_prefs.m_EditionList[0].m_name;

	if (sSection == "" && g_prefs.m_SectionList.GetCount() == 1)
		sSection = g_prefs.m_SectionList[0].m_name;




	pJob->m_polledpublicationname = sPublication;
	pJob->m_polledsectionname = sSection;
	pJob->m_pollededitionname = sEdition;
	pJob->m_polledissuename = _T("");

	


	sPublication = g_prefs.LookupNameFromAbbreviation(_T("Publication"), sPublication);

	sSection = g_prefs.LookupNameFromAbbreviation(_T("Section"), sSection);
	CString sCommonEdition = sEdition;
	sEdition = g_prefs.LookupNameFromAbbreviation(_T("Edition"), sEdition);



	isGray = sColor.CompareNoCase("Gray") == 0 || sColor.CompareNoCase("Grey") == 0 || sColor.CompareNoCase("G") == 0 || sColor.CompareNoCase(pInput->m_graycolorname) == 0;
	//Logprintf("Gray detection %d", (int)*isGray);

	if (sColor.CompareNoCase("CYAN") == 0)
		sColor = _T("C");
	if (sColor.CompareNoCase("MAGENTA") == 0)
		sColor = _T("M");
	if (sColor.CompareNoCase("YELLOW") == 0)
		sColor = _T("Y");
	if (sColor.CompareNoCase("BLACK") == 0)
		sColor = _T("K");

	if (sColor == _T("c"))
		sColor = _T("C");
	if (sColor == _T("m"))
		sColor = _T("M");
	if (sColor == _T("y"))
		sColor = _T("Y");
	if (sColor == _T("k"))
		sColor = _T("K");

	sColor = g_prefs.LookupNameFromAbbreviation(_T("Color"), sColor);

	// Return what was found
	pJob->m_planname = sPlanName;
	pJob->m_colorname = sColor;
	pJob->m_pagename = sPageName;
	pJob->m_pubdate = tDate;
	pJob->m_auxpubdate = bHasValidAuxDate ? tAuxDate : tDate;

	pJob->m_pubdateweekstart = tDateWeekStart;
	pJob->m_pubdateweekend = tDateWeekEnd;

	pJob->m_editionID = g_prefs.GetEditionID(sEdition);

	pJob->m_commoneditionID = 0;
	pJob->m_publicationID = g_prefs.GetPublicationID(sPublication);
	pJob->m_sectionID = g_prefs.GetSectionID(sSection);

	pJob->m_copies = atoi(sCopies);
	pJob->m_pagination = PageName2PageNumber(sPageName);
	pJob->m_markgroup = sMarkGroupSymbol;
	pJob->m_comment = sComment;
	pJob->m_pagination = nPagination;
	pJob->m_pageindex = nPageIndex;
	pJob->m_miscstring1 = sMiscString1;
	pJob->m_miscstring2 = sMiscString2;
	pJob->m_miscstring3 = sMiscString3;

	if (atoi(sPanoPage) > 0 && atoi(pJob->m_pagename) > 0)
		pJob->m_panomate = sPanoPage;

	if (sPlanName != "" && sColor != "")
		return TRUE;

	if (bMustHaveDate && sDate == "")
		return FALSE;

	// All ids MUST be resolved now... OR planname+color is specified

	BOOL bNameOK = TRUE;
	if (sDate == "" || sColor == "" || (sPageName == "" && nPagination == 0 && nPageIndex == 0)) {
		bNameOK = FALSE;

		if (sDate == "")
			Logprintf("ERROR: Parse error - Date empty");
		if (sColor == "")
			Logprintf("ERROR: Parse error - Color empty");

		if (sPageName == "" && nPagination == 0 && nPageIndex == 0)
			Logprintf("ERROR: Parse error - PageName or Pagination or PageIndex empty");
	}


	if ((sPublication == "" && !pInput->m_mayaddpage) || (sPublication == "" && pInput->m_mayaddpage && !pInput->m_registerunknownpublications)) {
		bNameOK = FALSE;
		if (sPublication == "" && !pInput->m_mayaddpage)
			Logprintf("ERROR: Parse error - Publication name empty - not allowed to add page");
		else
			Logprintf("ERROR: Parse error - Publication name empty - not allowed to register Publication name");
	}

	if (sSection == "" && !pInput->m_mayaddpage && !pJob->m_guesssection) {
		bNameOK = FALSE;
		Logprintf("ERROR: Parse error - Section name empty - not allowed to add page");
	}

	if (sSection == "" && pInput->m_mayaddpage && !pInput->m_registerunknownsections) {
		Logprintf("ERROR: Parse error - Section name empty - not allowed to register Section name");
		bNameOK = FALSE;
	}

	if (sEdition == "" && !pInput->m_mayaddpage && !pJob->m_guessedition) {
		bNameOK = FALSE;
		Logprintf("ERROR: Parse error - Edition name empty -  not allowed to add page");
	}

	if (sEdition == "" && pInput->m_mayaddpage && !pInput->m_registerunknownedition) {
		bNameOK = FALSE;
		Logprintf("ERROR: Parse error - Edition name empty - not allowed to register Section name");
	}



	return bNameOK;
}


BYTE g_CRCbufPoll[8192];

int CUtils::CRC32Poll(CString sFileName)
{
	HANDLE	Hndl;
	//BYTE	buf[8192];
	DWORD	nBytesRead;

	if ((Hndl = ::CreateFile(sFileName, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0)) == INVALID_HANDLE_VALUE)
		return 0;

	DWORD sum = 0;
	do {
		if (!::ReadFile(Hndl, g_CRCbufPoll, 8092, &nBytesRead, NULL)) {
			::CloseHandle(Hndl);
			return 0;
		}
		if (nBytesRead > 0) {
			for (int i = 0; i < nBytesRead; i++) {
				sum += g_CRCbufPoll[i];
				// just let it overflow..
			}
		}
	} while (nBytesRead > 0);

	::CloseHandle(Hndl);

	int nCRC = sum;
	return nCRC;
}


// Function name   : GetOwner
// Description     : Determines the 'Owner' of a given file or folder.
// Return type     : UINT is S_OK if successful; A Win32 'ERROR_' value otherwise.
// Argument        : LPCWSTR szFileOrFolderPathName is the fully qualified path of the file or folder to examine.
// Argument        : LPWSTR pUserNameBuffer is a pointer to a buffer used to contain the resulting 'Owner' string.
// Argument        : int nSizeInBytes is the size of the buffer.
UINT CUtils::GetOwner(LPCSTR szFileOrFolderPathName, LPSTR pUserNameBuffer, int nSizeInBytes)
{
	// 1) Validate the path:
	// 1.1) Length should not be 0.
	// 1.2) Path must point to an existing file or folder.
	if (!lstrlen(szFileOrFolderPathName) || !PathFileExists(szFileOrFolderPathName))
		return ERROR_INVALID_PARAMETER;

	// 2) Validate the buffer:
	// 2.1) Size must not be 0.
	// 2.2) Pointer must not be NULL.
	if (nSizeInBytes <= 0 || pUserNameBuffer == NULL)
		return ERROR_INVALID_PARAMETER;

	// 3) Convert the path to UNC if it is not already UNC so that we can extract a machine name from it:
	// 3.1) Use a big buffer... some OS's can have a path that is 32768 chars in length.
	TCHAR szUNCPathName[32767] = { 0 };
	// 3.2) If path is not UNC...
	if (!PathIsUNC(szFileOrFolderPathName)) {
		// 3.3) Mask the big buffer into a UNIVERSAL_NAME_INFO.
		DWORD dwUniSize = 32767 * sizeof(TCHAR);
		UNIVERSAL_NAME_INFO* pUNI = (UNIVERSAL_NAME_INFO*)szUNCPathName;
		// 3.4) Attempt to get the UNC version of the path into the big buffer.
		if (!WNetGetUniversalName(szFileOrFolderPathName, UNIVERSAL_NAME_INFO_LEVEL, pUNI, &dwUniSize)) {
			// 3.5) If successful, copy the UNC version into the buffer.
			lstrcpy(szUNCPathName, pUNI->lpUniversalName); // You must extract from this offset as the buffer has UNI overhead.
		}
		else {
			// 3.6) If not successful, copy the original path into the buffer.
			lstrcpy(szUNCPathName, szFileOrFolderPathName);
		}
	}
	else {
		// 3.7) Path is already UNC, copy the original path into the buffer.
		lstrcpy(szUNCPathName, szFileOrFolderPathName);
	}

	// 4) If path is UNC (will not be the case for local physical drive paths) we want to extract the machine name:
	// 4.1) Use a buffer bug enough to hold a machine name per Win32.
	TCHAR szMachineName[MAX_COMPUTERNAME_LENGTH + 1] = { 0 };
	// 4.2) If path is UNC...
	if (PathIsUNC(szUNCPathName)) {
		// 4.3) Use PathFindNextComponent() to skip past the double backslashes.
		LPSTR lpMachineName = PathFindNextComponent(szUNCPathName);
		// 4.4) Walk the the rest of the path to find the end of the machine name.
		int nPos = 0;
		LPSTR lpNextSlash = lpMachineName;
		while ((lpNextSlash[0] != L'\\') && (lpNextSlash[0] != L'\0')) {
			nPos++;
			lpNextSlash++;
		}
		// 4.5) Copyt the machine name into the buffer.
		lstrcpyn(szMachineName, lpMachineName, nPos + 1);
	}

	// 5) Derive the 'Owner' by getting the owner's Security ID from a Security Descriptor associated with the file or folder indicated in the path.
	// 5.1) Get a security descriptor for the file or folder that contains the Owner Security Information.
	// 5.1.1) Use GetFileSecurity() with some null params to get the required buffer size.
	// 5.1.2) We don't really care about the return value.
	// 5.1.3) The error code must be ERROR_INSUFFICIENT_BUFFER for use to continue.
	unsigned long   uSizeNeeded = 0;
	GetFileSecurity(szUNCPathName, OWNER_SECURITY_INFORMATION, 0, 0, &uSizeNeeded);
	UINT uRet = GetLastError();
	if (uRet == ERROR_INSUFFICIENT_BUFFER && uSizeNeeded) {
		uRet = S_OK; // Clear the ERROR_INSUFFICIENT_BUFFER

		// 5.2) Allocate the buffer for the Security Descriptor, check for out of memory
		LPBYTE lpSecurityBuffer = (LPBYTE)malloc(uSizeNeeded * sizeof(BYTE));
		if (!lpSecurityBuffer) {
			return ERROR_NOT_ENOUGH_MEMORY;
		}

		// 5.2) Get the Security Descriptor that contains the Owner Security Information into the buffer, check for errors
		if (!GetFileSecurity(szUNCPathName, OWNER_SECURITY_INFORMATION, lpSecurityBuffer, uSizeNeeded, &uSizeNeeded)) {
			free(lpSecurityBuffer);
			return GetLastError();
		}

		// 5.3) Get the the owner's Security ID (SID) from the Security Descriptor, check for errors
		PSID pSID = NULL;
		BOOL bOwnerDefaulted = FALSE;
		if (!GetSecurityDescriptorOwner(lpSecurityBuffer, &pSID, &bOwnerDefaulted)) {
			free(lpSecurityBuffer);
			return GetLastError();
		}

		// 5.4) Get the size of the buffers needed for the owner information (domain and name)
		// 5.4.1) Use LookupAccountSid() with buffer sizes set to zero to get the required buffer sizes.
		// 5.4.2) We don't really care about the return value.
		// 5.4.3) The error code must be ERROR_INSUFFICIENT_BUFFER for use to continue.
		LPSTR			pName = NULL;
		LPSTR			pDomain = NULL;
		unsigned long   uNameLen = 0;
		unsigned long   uDomainLen = 0;
		SID_NAME_USE    sidNameUse = SidTypeUser;
		LookupAccountSid(szMachineName, pSID, pName, &uNameLen, pDomain, &uDomainLen, &sidNameUse);
		uRet = GetLastError();
		if ((uRet == ERROR_INSUFFICIENT_BUFFER) && uNameLen && uDomainLen) {
			uRet = S_OK; // Clear the ERROR_INSUFFICIENT_BUFFER

			// 5.5) Allocate the required buffers, check for out of memory
			pName = (LPSTR)malloc(uNameLen * sizeof(TCHAR));
			pDomain = (LPSTR)malloc(uDomainLen * sizeof(TCHAR));
			if (!pName || !pDomain) {
				free(lpSecurityBuffer);
				return ERROR_NOT_ENOUGH_MEMORY;
			}

			// 5.6) Get domain and username
			if (!LookupAccountSid(szMachineName, pSID, pName, &uNameLen, pDomain, &uDomainLen, &sidNameUse)) {
				free(pName);
				free(pDomain);
				free(lpSecurityBuffer);
				return GetLastError();
			}

			// 5.7) Build the owner string from the domain and username if there is enough room in the buffer.
			if (nSizeInBytes > ((uNameLen + uDomainLen + 2) * sizeof(TCHAR))) {
				lstrcpy(pUserNameBuffer, pDomain);
				//	lstrcat(pUserNameBuffer, L"\\");
				lstrcat(pUserNameBuffer, _T("\\"));
				lstrcat(pUserNameBuffer, pName);
			}
			else {
				uRet = ERROR_INSUFFICIENT_BUFFER;
			}

			// 5.8) Release memory
			free(pName);
			free(pDomain);
		}
		// 5.9) Release memory
		free(lpSecurityBuffer);
	}
	return uRet;
}


CString CUtils::GetProcessOwner(CString sFileOrFolderPathName)
{
	CString sBuf = "";
	CString sCmd;
	DWORD dwBytesWritten;
	CString sTxtFile = GetTempFolder() + _T("\\processinfo.txt");

	sCmd.Format("\"%s\\Handle.exe\" -a \"%s\" > \"%s\"", g_prefs.m_apppath, sFileOrFolderPathName, sTxtFile);

	HANDLE hFile = ::CreateFile(GetTempFolder() + _T("\\gethandle.bat"),
		GENERIC_WRITE, FILE_SHARE_READ,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
		return "";

	::WriteFile(hFile, sCmd, sCmd.GetLength(), &dwBytesWritten, NULL);
	::CloseHandle(hFile);

	DoCreateProcessEx(GetTempFolder() + _T("\\gethandle.bat"), 120);

	for (int i = 0; i < 500; i++) {
		::Sleep(100);
		if (FileExist(sTxtFile) && GetFileSize(sTxtFile) > 0)
			break;
	}

	Load(sTxtFile, sBuf);

	return sBuf;
}

CString CUtils::GetFilePID(CString sFileOrFolderPathName)
{
	CString sBuf = "";
	CString sCmd;
	DWORD dwBytesWritten;

	CString sTxtFile = GetTempFolder() + _T("\\processinfo.txt");
	::DeleteFile(sTxtFile);
	sCmd.Format("\"%s\\Handle.exe\" \"%s\" > \"%s\"" , g_prefs.m_apppath, sFileOrFolderPathName, sTxtFile);

	HANDLE hFile = CreateFile(GetTempFolder() + _T("\\gethandle.bat"),
		GENERIC_WRITE, FILE_SHARE_READ,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
		return "";

	WriteFile(hFile, sCmd, sCmd.GetLength(), &dwBytesWritten, NULL);
	CloseHandle(hFile);


	DoCreateProcessEx(GetTempFolder() + _T("\\gethandle.bat"), 120);

	for (int i = 0; i < 500; i++) {
		Sleep(100);
		if (FileExist(sTxtFile) && GetFileSize(sTxtFile) > 0)
			break;
	}

	Load(sTxtFile, sBuf);
	return sBuf;
}

BOOL CUtils::ForceHandlesClose(CString sFilename)
{
	DWORD dwBytesWritten;

	if (FileExist(sFilename) == FALSE)
		return TRUE;

	CString sFileNameLocalDrive;

	/*if (g_prefs.m_ccdatalocaldriveletter != "" && sFilename[0] == '\\' && sFilename[1] == '\\') {
		sFilename = sFilename.Mid(2);
		int n = sFilename.Find("\\");
		if (n != -1)
			sFilename = sFilename.Mid(n);
		sFilename = g_prefs.m_ccdatalocaldriveletter + sFilename;
	}*/
	

	// Attempt 1 - directly at child process (may fail)
	CString sCmd;
	sCmd.Format("openfiles.exe /disconnect /a * /OP \"%s\"", sFilename);
	DoCreateProcessEx(sCmd, 60);

	// Attempt 2 - via batch file
	::DeleteFile("c:\\temp\\ForceClose.bat");
	HANDLE hFile = ::CreateFile(_T("c:\\temp\\ForceClose.bat"),
		GENERIC_WRITE, FILE_SHARE_READ,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile != INVALID_HANDLE_VALUE) {
		::WriteFile(hFile, sCmd, sCmd.GetLength(), &dwBytesWritten, NULL);
		::CloseHandle(hFile);
		DoCreateProcessEx("c:\\temp\\ForceClose.bat", 10);
	}

	// Attempt 3 - use remote server name
	CString sServer = GetFilePath(g_prefs.m_hiresPDFPath);
	if (sServer[0] == '\\')
		sServer == sServer.Mid(1);
	if (sServer[0] == '\\')
		sServer == sServer.Mid(1);

	int n = sServer.Find("\\");
	if (n != -1)
		sServer = sServer.Left(n);

	sCmd.Format("openfiles.exe /disconnect /s \"%s\" /a * /OP \"%s\"", sServer, sFilename);
	DoCreateProcessEx(sCmd, 10);

	::DeleteFile("c:\\temp\\ForceClose2.bat");
	hFile = CreateFile(_T("c:\\temp\\ForceClose2.bat"),
		GENERIC_WRITE, FILE_SHARE_READ,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile != INVALID_HANDLE_VALUE) {
		WriteFile(hFile, sCmd, sCmd.GetLength(), &dwBytesWritten, NULL);
		CloseHandle(hFile);
		DoCreateProcessEx("c:\\temp\\ForceClose2.bat", 60);
	}

	CString sPID = GetFilePID(sFilename);
	if (sPID != "") {
		CString pid = "";
		int m = sPID.Find("pid: ");
		if (m != -1) {
			pid = sPID.Mid(m + 5, 6);
			pid.Trim();
		}
		CString id = "";
		m = sPID.Find("type: File");
		if (m != -1) {
			id = sPID.Mid(m + 11);
			id.Trim();
			int m2 = id.Find(":");
			if (m2 != -1)
				id = id.Left(m2);
		}
		if (id != "" && pid != "") {
			sCmd.Format("\"%s\\handle.exe\" -c %s -y -p %s", g_prefs.m_apppath, id, pid);
			Logprintf("INFO:  %s", sCmd);
			::DeleteFile("c:\\temp\\ForceClose3.bat");
			hFile = CreateFile(_T("c:\\temp\\ForceClose3.bat"),
				GENERIC_WRITE, FILE_SHARE_READ,
				NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

			if (hFile != INVALID_HANDLE_VALUE) {
				WriteFile(hFile, sCmd, sCmd.GetLength(), &dwBytesWritten, NULL);
				CloseHandle(hFile);
				DoCreateProcessEx("c:\\temp\\ForceClose3.bat", 60);
			}
		}
	}

	Logprintf("INFO:  ForceHandlesClose called on file %s", sFilename);

	return TRUE;
}

int CUtils::IssueMessage(CDatabaseManager &DB, CJobAttributes &Job, int nStatus, CString sTargetFolder, int nType, CString &sErrorMessage)
{

	sErrorMessage = _T("");

	if (sTargetFolder == _T(""))
		return TRUE;

	BOOL bSuccessStatus = (nStatus == STATUSNUMBER_POLLED || nStatus == STATUSNUMBER_TRANSMITTED);
	BOOL bErrorStatus = (nStatus == STATUSNUMBER_POLLINGERROR || nStatus == STATUSNUMBER_TRANSMISSIONERROR);
	if (bErrorStatus)
		nType = 1036;
	if (bSuccessStatus && (g_prefs.m_messageonsuccess == FALSE || FileExist(g_prefs.m_messagetemplatesuccess) == FALSE))
		return TRUE;
	if (bErrorStatus && (g_prefs.m_messageonerror == FALSE || FileExist(g_prefs.m_messagetemplateerror) == FALSE))
		return TRUE;

	Logprintf("INFO: Preparing ack-file generation..");

	TCHAR buf[4097];
	DWORD dwBytesRead;
	DWORD nBytesWritten;
	HANDLE hndl;
	if (bSuccessStatus) {
		hndl = CreateFile(g_prefs.m_messagetemplatesuccess, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
		if (hndl == INVALID_HANDLE_VALUE) {
			sErrorMessage.Format("Message template file not found (%s)", g_prefs.m_messagetemplatesuccess);
			Logprintf("ERROR: %s\r\n", sErrorMessage);
			return FALSE;
		}
		if (!ReadFile(hndl, buf, 4096, &dwBytesRead, NULL)) {
			CloseHandle(hndl);
			sErrorMessage.Format("Error reading message template file (%s)", g_prefs.m_messagetemplatesuccess);
			Logprintf("ERROR: %s\r\n", sErrorMessage);
			return FALSE;
		}
		buf[dwBytesRead] = 0;
		CloseHandle(hndl);
	}
	if (bErrorStatus) {
		hndl = CreateFile(g_prefs.m_messagetemplateerror, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
		if (hndl == INVALID_HANDLE_VALUE) {
			sErrorMessage.Format("Message template file not found (%s)", g_prefs.m_messagetemplateerror);
			Logprintf("ERROR: %s\r\n", sErrorMessage);
			return FALSE;
		}
		if (!ReadFile(hndl, buf, 4096, &dwBytesRead, NULL)) {
			CloseHandle(hndl);
			sErrorMessage.Format("Error reading message template file (%s)", g_prefs.m_messagetemplateerror);
			Logprintf("ERROR: %s\r\n", sErrorMessage);
			return FALSE;
		}
		buf[dwBytesRead] = 0;
		CloseHandle(hndl);
	}

	CString sContent(buf);

	//util.FormName(sContent, g_prefs.m_messagedateformat, g_prefs.m_messagedateformat2, Job, g_prefs.m_messageusealiases, 0);

	CString sNewContent = sContent;
	CString sTime; // 2019-10-10T10:25:01+2:00
	CTime tNow = CTime::GetCurrentTime();
	int nGmt = 2;

	sTime.Format("%.4d-%.2d-%.2dT%.2d:%.2d:%.2d+%d:00", tNow.GetYear(), tNow.GetMonth(), tNow.GetDay(), tNow.GetHour(), tNow.GetMinute(), tNow.GetSecond(), nGmt);
	CString sF = Job.m_filename;
	sF.Replace(".pdf", "");
	sF.Replace(".PDF", "");
	sF.Replace("C1", "");
	sF.Replace("C2", "");
	sF.Replace("C3", "");
	sF.Replace("C4", "");
	sF.Replace("C5", "");
	sF.Replace("C6", "");
	sF.Replace("C7", "");
	sF.Replace("C8", "");
	sF.Replace("C9", "");

	sNewContent.Replace("%f", sF);
	sNewContent.Replace("%t", sTime);
	sNewContent.Replace("%e", sTime);
	sNewContent.Replace("%n", Int2String(nType));
	
	//CString sFileName = GetFileName(Job.m_filename, TRUE) + ".xml";// util.FormName(g_prefs.m_messagefilenamemask, g_prefs.m_messagedateformat, g_prefs.m_messagedateformat2, Job, g_prefs.m_messageusealiases, 0);
	CString sFileName = GetFileName(Job.m_filename, TRUE) + _T("-") + Int2String(nType) + ".xml";// util.FormName(g_prefs.m_messagefilenamemask, g_prefs.m_messagedateformat, g_prefs.m_messagedateformat2, Job, g_prefs.m_messageusealiases, 0);

	sFileName.Replace("/", "");
	sFileName.Replace(":", "");
	sFileName.Replace("*", "");
	sFileName.Replace("?", "");
	sFileName.Replace("|", "");
	sFileName.Replace("%", "");
	sFileName.Replace("<", "");
	sFileName.Replace(">", "");
	sFileName.Replace("\"", "");
	sFileName.Replace("\\", "");

	CString sFullFileName = sTargetFolder + _T("\\") + sFileName;
	Logprintf("INFO: Preparing ack-file generation %s", sFullFileName);

	::DeleteFile(sFullFileName);
	hndl = ::CreateFile(sFullFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hndl == INVALID_HANDLE_VALUE) {
		sErrorMessage.Format("Cannot create message file (%s)", sFullFileName);
		Logprintf("ERROR: %s\r\n", sErrorMessage);
		return FALSE;
	}
	if (::WriteFile(hndl, sNewContent.GetBuffer(4096), sNewContent.GetLength(), &nBytesWritten, NULL) == FALSE) {
		sErrorMessage.Format("Error writing message file (%s)", sFullFileName);
		sNewContent.ReleaseBuffer();
		::CloseHandle(hndl);
		Logprintf("ERROR: %s\r\n", sErrorMessage);
		return FALSE;
	}
	sNewContent.ReleaseBuffer();
	::CloseHandle(hndl);
	Logprintf("INFO: ack-file %s written.", sFullFileName);
	Logprintf("%s", sNewContent);


	return TRUE;
}

CString CUtils::GetModuleLoadPath()
{
	TCHAR ModName[_MAX_PATH];
	_tcscpy(ModName, _T(""));

	if (!::GetModuleFileName(NULL, ModName, sizeof(ModName)))
		return CString(_T(""));


	CString LoadPath(ModName);

	int Idx = LoadPath.ReverseFind(_T('\\'));

	if (Idx == -1)
		return CString(_T(""));

	//if (LoadPath.Find("exe") != -1) {
   //    return CString(_T(""));
	//}


	LoadPath = LoadPath.Mid(0, Idx);


	return LoadPath;
}

CString CUtils::GetComputerName()
{
	TCHAR buf[MAX_PATH];
	DWORD buflen = MAX_PATH;
	::GetComputerName(buf, &buflen);
	CString s(buf);

	return s;
}