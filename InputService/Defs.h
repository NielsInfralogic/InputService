#pragma once
#include "stdafx.h"

/////////////////////////////////////////
//
// Generel definitions
//
/////////////////////////////////////////

typedef void(*DATEOFWEEK)(int nYear, int nWeek, int nDayOfWeek, int *nDay, int *nMonth);

#define SERVICETYPE_PLANIMPORT 1
#define SERVICETYPE_FILEIMPORT 2
#define SERVICETYPE_PROCESSING 3
#define SERVICETYPE_EXPORT     4
#define SERVICETYPE_DATABASE   5
#define SERVICETYPE_FILESERVER 6
#define SERVICETYPE_MAINTENANCE 7


#define SERVICESTATUS_STOPPED 0
#define SERVICESTATUS_RUNNING 1
#define SERVICESTATUS_HASERROR 2

#define EVENTCODE_CRC_PRINTPDF	 770
#define EVENTCODE_CRC_HIGHRESPDF 771
#define EVENTCODE_CRC_LOWRESPDF	 772


#define PROCESSSTATE_UNKNOWN		-2
#define PROCESSSTATE_OFFLINE		0
#define PROCESSSTATE_READY			1
#define PROCESSSTATE_ERROR			2
#define PROCESSSTATE_WARNING		3
#define PROCESSSTATE_WORKING		4


#define POLLDBSTATUS_DBERROR				-1
#define POLLDBSTATUS_UNKNOWNPAGE			 0
#define POLLDBSTATUS_KNOWNPAGE				 1
#define POLLDBSTATUS_SUBEDITIONPAGEADDED	 5
#define POLLDBSTATUS_ISSUEPAGEADDED			 6
#define POLLDBSTATUS_NEWPAGE				 7
#define POLLDBSTATUS_PAGEINPROGRESS			 9
#define POLLDBSTATUS_COLORNOTINPRODUCTION	10
#define POLLDBSTATUS_PRODUCTIONLOCKED		11
#define POLLDBSTATUS_PRODUCTIONCOLORLOCKED	12
#define POLLDBSTATUS_GOT_TIMED_EDITION		20
#define POLLDBSTATUS_ILLEGALCOLORPAGE		 8

#define EVENTCODE_POLLERROR			6
#define EVENTCODE_FTP_OK			130
#define EVENTCODE_FTP_ERROR			136
#define EVENTCODE_FTP_WARNING		137
#define EVENTCODE_TAC_OK			140
#define EVENTCODE_TAC_ERROR			146
#define EVENTCODE_DENSITY			149
#define EVENTCODE_RECEIVE_OK		170
#define EVENTCODE_RECEIVE_ERROR		176
#define EVENTCODE_RECEIVE_WARNING		177
#define EVENTCODE_UNREAD_INFO_MESSAGE				500
#define EVENTCODE_UNREAD_SEVERE_MESSAGE				501
#define EVENTCODE_READ_INFO_MESSAGE					510
#define EVENTCODE_READ_SEVERE_MESSAGE				511
#define EVENTCODE_UNREAD_INFO_MESSAGE_PRESSRUN		520
#define EVENTCODE_UNREAD_SEVERE_MESSAGE_PRESSRUN	521
#define EVENTCODE_READ_INFO_MESSAGE_PRESSRUN		530
#define EVENTCODE_READ_SEVERE_MESSAGE_PRESSRUN		531
#define EVENTCODE_PDFINFO	710
#define EVENTCODE_TRANSMITNAME	700
#define EVENTCODE_CHANNELTXSTART	805
#define EVENTCODE_CHANNELTXERROR	806
#define EVENTCODE_CHANNELTXEND		810
#define EVENTCODE_TRUEMONO			250
#define EVENTCODE_CHANNELSTATUSMISSING			750
#define EVENTCODE_CHANNELSTATUSOK			    751
#define EVENTCODE_THUMBNAILSIZE 421
#define EVENTCODE_POLLED 10

#define SCHEDULE_ALWAYS	0
#define SCHEDULE_WINDOW 1

#define MAILTYPE_DBERROR		0
#define MAILTYPE_FILEERROR	1
#define MAILTYPE_FOLDERERROR 2

#define MAILTYPE_TXERROR		4
#define MAILTYPE_POLLINGERROR		5


#define REAPPROVEUPDATES_NOCHANGE				0
#define REAPPROVEUPDATES_REAPPROVEUNLESSAUTO	1
#define REAPPROVEUPDATES_REAPPROVEALWAYS		2



#define APPROVAL_AUTO					-1
#define APPROVAL_NOTAPPROVED			0
#define APPROVAL_APPROVED				1
#define APPROVAL_REJECTED				2

#define HOLDSTATE_RELEASED				0
#define HOLDSTATE_HELD					1

#define TEMPFILE_NONE	0
#define TEMPFILE_NAME	1
#define TEMPFILE_FOLDER	2

#define MAX_ERRMSG 16384
#define MAXEMAILFILES 1000

#define PLANLOCK_LOCK		1
#define PLANLOCK_NOLOCK		0
#define PLANLOCK_UNLOCK		0
#define PLANLOCK_UNKNOWN	-1

#define FOLDERSCANORDER_ALPHABETIC	0
#define FOLDERSCANORDER_FIFO		1
#define FOLDERSCANORDER_REGEXPRIO	2
#define FOLDERSCANORDER_REGEXRENAME	3

#define LOCKCHECKMODE_NONE	0
#define LOCKCHECKMODE_READ	1
#define LOCKCHECKMODE_READWRITE	2
#define LOCKCHECKMODE_RANGELOCK	3

#define FOLDERSTATE_RUNNING			0
#define FOLDERSTATE_RUNNING_OVER	1
#define FOLDERSTATE_STOPPED			2
#define FOLDERSTATE_STOPPED_OVER	3

#define FOLDERSTATUS_IDLE		0
#define FOLDERSTATUS_ERROR		1
#define FOLDERSTATUS_INPROGRESS	2
#define FOLDERSTATUS_STOPPED	3
#define FOLDERSTATUS_SCANNING   4
#define FOLDERSTATUS_CONNECTING 5
#define FOLDERSTATUS_WAITING   6


#define MAXITEMS 48
#define MAXFILES 1000
#define MAXFILESPERTHREAD 500
//#define MAXCOPIES 3
#define MAXSPOOLERS 150

#define MAXLOCATIONS 1




#define JMFSTATUS_OK 0
#define JMFSTATUS_ERROR 1
#define JMFSTATUS_WARNING 2
#define JMFSTATUS_UNKNOWN -1


#define JMFNETWORK_SHARE	0
#define JMFNETWORK_FTP		1

#define NETWORKTYPE_SHARE	0
#define NETWORKTYPE_FTP		1
#define NETWORKTYPE_EMAIL	2

#define COPYTHREAD_STARTING	            10000
#define COPYTHREAD_STARTED              10001
#define COPYTHREAD_INPROGRESS           10002
#define COPYTHREAD_DONE                 10003
#define COPYTHREAD_ABORT                10004
#define COPYTHREAD_ERROR				10005
#define COPYTHREAD_TIMEOUT				10006

#define MAXREGEXP 100

#define REC_BUFFER_SIZE 65535
#define TOP_BUFFER_SIZE 2048

#define MESSAGETIMEOUT 1000*5

#define REMOTEFILECHECK_NONE		0
#define REMOTEFILECHECK_EXIST		1
#define REMOTEFILECHECK_SIZE		2
#define REMOTEFILECHECK_READBACK	3


#define PDFSCALEMODE_CUSTOM			0
#define PDFSCLAEMODE_FIXEDHEIGHT	1
#define PDFSCLAEMODE_MAXHEIGHT		2
#define PDFSCLAEMODE_FIXEDWIDTH		3
#define PDFSCLAEMODE_MAXWIDTH		4

#define PDFCOMPRESSION_RETAIN	0
#define PDFCOMPRESSION_FLATE	1
#define PDFCOMPRESSION_JPEG		2
#define PDFCOMPRESSION_JPEG2000	3
#define PDFCOMPRESSION_NONE		4
#define PDFCOMPRESSION_JBIG2	5


#define FTPTYPE_NORMAL	0
#define FTPTYPE_FTPES	1
#define FTPTYPE_FTPS	2
#define FTPTYPE_SFTP	3

#define MAXTRANSMITQUEUE		1000

/////////////////////////////////////////
// Page table definitions
/////////////////////////////////////////


#define POLLINPUTTYPE_LOWRESPDF 0
#define POLLINPUTTYPE_HIRESPDF 1
#define POLLINPUTTYPE_PRINTPDF 2


#define TXNAMEOPTION_REMOVESPACES		1
#define TXNAMEOPTION_ALLSMALLCPAS		2
#define TXNAMEOPTION_ZIPPAGES			4
//#define TXNAMEOPTION_MERGEPAGES			8

#define TXNAMEOPTION_MERGEPAGES			2
#define TXNAMEOPTION_GENERATEXML		4
#define TXNAMEOPTION_SENDCOMMONPAGES	8

#define PAGETYPE_NORMAL					0
#define PAGETYPE_PANORAMA				1
#define PAGETYPE_ANTIPANORAMA			2
#define PAGETYPE_DUMMY					3



/////////////////////////////////////////
// Internal status definitions
/////////////////////////////////////////

#define STATUS_QUEUED				0
#define STATUS_RESAMPLING			1
#define STATUS_READY				2
#define STATUS_ERROR				3
#define STATUS_UNKNOWN				4
#define STATUS_INPROGRESS			5
#define STATUS_TRANSMITTING			6
#define STATUS_TRANSMITTED			7
#define STATUS_TRANSMITERROR		8

#define STATE_RELEASED				0
#define STATE_LOCKED				1
#define STATE_INPROGRESS			2


/////////////////////////////////////////
// Status codes
/////////////////////////////////////////

#define STATUSNUMBER_MISSING			0
#define STATUSNUMBER_POLLING			5
#define STATUSNUMBER_POLLINGERROR		6
#define STATUSNUMBER_POLLED				10
#define STATUSNUMBER_RESAMPLING			15
#define STATUSNUMBER_RESAMPLINGERROR	16
#define STATUSNUMBER_READY				20
#define STATUSNUMBER_TRANSMITTING		25
#define STATUSNUMBER_TRANSMISSIONERROR	26
#define STATUSNUMBER_REMOTETRANSMITTED	29
#define STATUSNUMBER_TRANSMITTED		30
#define STATUSNUMBER_ASSEMBLING			35
#define STATUSNUMBER_ASSEMBLINGERROR	36
#define STATUSNUMBER_ASSEMBLED			40
#define STATUSNUMBER_IMAGING			45
#define STATUSNUMBER_IMAGINGERROR		46
#define STATUSNUMBER_IMAGED				50
#define STATUSNUMBER_VERIFYING			55
#define STATUSNUMBER_VERIFYERROR		56
#define STATUSNUMBER_VERIFIED			60
#define STATUSNUMBER_KILLED				99

#define STATUSNUMBER_FLATTRANSMITTING	47
#define STATUSNUMBER_FLATTRANSMISSIONERROR	48
#define STATUSNUMBER_FLATTRANSMITTED	50
#define STATUSNUMBER_FLATREMOTETRANSMITTED 50


#define PROOFSTATUSNUMBER_NOTPROOFED	0
#define PROOFSTATUSNUMBER_PROOFING		5
#define PROOFSTATUSNUMBER_PROOFERROR	6
#define PROOFSTATUSNUMBER_PROOFED		10

#define PROOFSTATUSNUMBER_READVIEWPROOFING		15
#define PROOFSTATUSNUMBER_READVIEWPROOFERROR	16
#define PROOFSTATUSNUMBER_READVIEWPROOFED		20

#define EVENTCODE_EXPORTQUEUED		25
#define EVENTCODE_EXPORTQUEUED_ERROR		27


typedef struct PROGRESSBARINFOTAG {
	int nSetupIndex;
	int nHandle;
	DWORD dwFileSize;
	CString sFileName;
	CTime tStartTime;
	int nState;
} PROGRESSBARINFO;

typedef struct LOGLISTITEMTAG
{
	CString sStatus;
	CString	sFolder;
	CString	sFileName;
	CString	sMessage;
	int     nSetupIndex;
	DWORD	dwGUID;
} LOGLISTITEM;

typedef struct PROGRESSLOGITEMTAG {
	int nTick;
	CString	sFileName;
	int nSetupIndex;
	int nStatus;
	CString sMessage;
	BOOL bEnabled;
} PROGRESSLOGITEM;

typedef struct FILEINFOSTRUCTTAG {
	CString	sFileName;
	CString	sFolder;
	CTime	tJobTime;
	CTime	tWriteTime;
	DWORD	nFileSize;
} FILEINFOSTRUCT;

typedef CList <FILEINFOSTRUCT,FILEINFOSTRUCT&> FILELISTTYPE;

typedef struct FILECOPYINFOTAG {
	CString	sSrcFileName;
	CString	sDstFileName;
	int		nStatus;
	CString sErrorMessage;
	int		nTimeOut;
} FILECOPYINFO;

class ALIASSTRUCT {
public:
	ALIASSTRUCT() { sType = _T("Color"); sLongName = _T(""); sShortName = _T(""); };
	CString	sType;
	CString sLongName;
	CString sShortName;
};
typedef CArray <ALIASSTRUCT, ALIASSTRUCT&> ALIASLIST;

class REGEXPSTRUCT {
	public:
	REGEXPSTRUCT() {	m_useExpression = FALSE; 
						m_matchExpression = _T("");
						m_formatExpression = _T("");
						m_partialmatch = FALSE;
						m_comment = _T("");
	 };

	BOOL		m_useExpression;
	CString		m_matchExpression;
	CString		m_formatExpression;
	BOOL		m_partialmatch;
	CString		m_comment;
};

typedef CArray <REGEXPSTRUCT, REGEXPSTRUCT&> REGEXPLIST;

class INPUTPROCESSSTRUCT {
public:
	INPUTPROCESSSTRUCT() {
		m_ID = -1;
		m_inputname = _T("");
		m_inputtype = 0;
		m_enabled = TRUE;
		m_localenabled = TRUE;
		m_locationID = 0;
		m_inputpath = _T("");
		m_searchmask = _T("*.*");

		m_stabletime = 1;
		m_polltime = 1;
		m_useregexp = FALSE;
		m_namingmask = _T("");
		m_masktype = 0;

		m_defaultpublicationID = 1;
		m_defaultsectionID = 1;
		m_defaulteditionID = 1;
		m_defaulttemplateID = 1;
		m_defaultrunID = 1;
		m_defaultprooftemplateID = 1;
		m_defaultlocationID = 1;
		m_defaultcopies = 1;

		m_mayaddcolor = FALSE;
		m_mayaddpage = FALSE;
		m_approvalrequired = FALSE;
		m_putonhold = FALSE;
		m_priority = 50;
		m_uselatestdate = FALSE;

		m_guessedition = FALSE;
		m_guesssection = FALSE;
		m_guessrun = FALSE;
		m_guesslocation = FALSE;

		m_useFTP = FALSE;
		m_FTPserver = _T("");
		m_FTPusername = _T("");
		m_FTPpassword = _T("");
		m_FTPfolder = _T("");
		m_FTPport = 21;
		m_ftpxcrc = TRUE;
		m_ftptls = TRUE;

		m_zipPDF = FALSE;
		m_splitPDF = FALSE;

		m_useweekreference = TRUE;
		m_copyeditions = TRUE;

		m_usecurrentuser = TRUE;
		m_username = _T("");
		m_password = _T("");

		m_folderscanorder = FOLDERSCANORDER_FIFO;
		m_killemptyseps = FALSE;
		m_checkimagesize = FALSE;

		m_callscript = FALSE;
		m_scriptfile = _T("");

		m_saveoldversions = FALSE;

		for (int i = 0; i < MAXLOCATIONS; i++) {
			m_locationenable[i] = FALSE;
			m_locationtemplate[i] = _T("");
		}

		m_NumberOfRegExps = 0;
		m_pagination = 0;
		m_changeifgray = TRUE;
		m_pubdateplus = 1;
		m_rollovertime = CTime(1975, 1, 1, 6, 0, 0);

		for (int i = 0; i < 4; i++) {
			m_markgroupflags[i] = -1;
			m_markgroupsymbols[i] = _T("");
			m_markgroups[i] = _T("");
		}

		m_overruleproofer = FALSE;
		m_overruledproofconfigurationID = 1;
		m_overrulepageformat = FALSE;
		m_overruledpageformatID = 1;

		m_registerunknownpublications = FALSE;
		m_registerunknownedition = FALSE;
		m_registerunknownsections = FALSE;
		m_registerunknownissues = FALSE;
		m_registerunknowncolors = FALSE;

		m_graycolorname = "Gray";

		m_detectpanorama = FALSE;

		m_overruledrelease_putonhold = FALSE;
		m_overruledapproval_approvalrequired = FALSE;
		m_overruleapproval = FALSE;
		m_overrulerelease = FALSE;


		m_flowdrive = FALSE;
		m_transmitoriginalname = FALSE;
		m_txafterapproval = FALSE;
		m_rejectidentical = FALSE;

		m_allowpastpubdate = FALSE;
		m_onlygraymononame = FALSE;

		m_callrenamessp = FALSE;
		m_unplannedrequiresapply = FALSE;

		m_copyfolder = _T("");
		m_makecopy = FALSE;
		m_autoinvert = FALSE;

		m_makecopylocationname = FALSE;

		m_ftppasv = FALSE;
		m_makecopyalways = FALSE;

		m_ignoreupdatesonalreadyapproved = FALSE;
		m_allowtimededitions = FALSE;

		m_rejectifnoregularexpressionmatch = FALSE;

		m_retryunknownfiles = FALSE;

		m_SortNumberOfRegExps = 0;

		m_lowpriorityfolder = FALSE;

		m_reapproveaction = REAPPROVEUPDATES_NOCHANGE;

		m_autogenerateplate = FALSE;
		m_autogenerateplatehold = TRUE;
		m_autogenerateplateapproval = TRUE;
		m_autogenerateplatefreedevice = TRUE;


		m_transmitoriginalpdf = FALSE;

		m_callconditionalcopyscript = FALSE;
		m_conditionalcopysp = _T("");

		m_fixedpressgroup = FALSE;
		m_defaultpressgroup = _T("");

		m_mailuseimap = FALSE;
		m_mailserver = _T("");
		m_mailusername = _T("");
		m_mailpassword = _T("");
		m_mailusessl = FALSE;
		m_mailimapinbox = _T("");
		m_mailfromlistfilter = _T("");
		m_mailport = 25;

		m_sendackfile = FALSE;
		m_ackfilefolder = _T("");
		m_ackflagvalue = 1035;
	};

	int		m_ID;
	CString m_inputname;
	int		m_inputtype;
	BOOL	m_enabled;
	BOOL	m_localenabled;
	int		m_locationID;
	CString m_inputpath;
	CString m_searchmask;

	UINT	m_stabletime;
	UINT	m_polltime;
	BOOL	m_useregexp;
	CString m_namingmask;
	int		m_masktype;

	int		m_defaultpublicationID;
	int		m_defaultsectionID;
	int		m_defaulteditionID;
	int		m_defaulttemplateID;
	int		m_defaultrunID;
	int		m_defaultprooftemplateID;
	int		m_defaultlocationID;
	int		m_defaultcopies;

	BOOL	m_mayaddcolor;
	BOOL	m_mayaddpage;				// Used for poll without productions
	BOOL	m_approvalrequired;
	BOOL	m_putonhold;
	int		m_priority;
	BOOL	m_uselatestdate;
	int		m_pagination;
	BOOL	m_guessedition;
	BOOL	m_guesssection;
	BOOL	m_guessrun;
	BOOL	m_guesslocation;

	BOOL	m_useFTP;
	CString m_FTPserver;
	CString m_FTPusername;
	CString	m_FTPpassword;
	CString m_FTPfolder;
	UINT	m_FTPport;
	BOOL	m_ftppasv;
	BOOL	m_ftpxcrc;
	int		m_ftptls;

	BOOL	m_zipPDF;
	BOOL	m_splitPDF;

	BOOL	m_copyeditions;
	BOOL	m_useweekreference;

	BOOL	m_usecurrentuser;
	CString m_username;
	CString m_password;

	int m_folderscanorder;
	BOOL m_killemptyseps;
	BOOL m_checkimagesize;

	BOOL	m_callscript;
	CString m_scriptfile;

	BOOL	m_locationenable[1];
	CString m_locationtemplate[1];

	CString m_separators;

	int		m_NumberOfRegExps;
	REGEXPSTRUCT	m_RegExpList[MAXREGEXP];


	int		m_SortNumberOfRegExps;
	REGEXPSTRUCT	m_SortRegExpList[MAXREGEXP];

	BOOL	m_changeifgray;

	int		m_pubdateplus;
	CTime	m_rollovertime;

	int		m_markgroupflags[4];
	CString	m_markgroupsymbols[4];
	CString	m_markgroups[4];

	BOOL	m_overruleproofer;
	int		m_overruledproofconfigurationID;


	BOOL	m_registerunknownpublications;
	BOOL	m_registerunknownedition;
	BOOL	m_registerunknownsections;
	BOOL	m_registerunknownissues;
	BOOL	m_registerunknowncolors;

	CString m_graycolorname;
	BOOL	m_detectpanorama;

	int		m_overruledrelease_putonhold;
	int		m_overruledapproval_approvalrequired;
	BOOL	m_overruleapproval;
	BOOL	m_overrulerelease;

	BOOL	m_overrulepageformat;
	int		m_overruledpageformatID;

	BOOL	m_flowdrive;
	BOOL	m_transmitoriginalname;
	BOOL	m_txafterapproval;

	BOOL	m_rejectidentical;
	BOOL	m_allowpastpubdate;
	BOOL	m_onlygraymononame;

	BOOL	m_callrenamessp;
	BOOL	m_unplannedrequiresapply;

	BOOL	m_saveoldversions;

	CString m_copyfolder;
	BOOL	m_makecopy;

	BOOL	m_autoinvert;

	BOOL	m_makecopylocationname;
	BOOL	m_makecopyalways;
	BOOL	m_ignoreupdatesonalreadyapproved;
	BOOL	m_allowtimededitions;

	BOOL	m_rejectifnoregularexpressionmatch;

	BOOL	m_retryunknownfiles;

	BOOL	m_lowpriorityfolder;
	int		m_reapproveaction;

	BOOL	m_autogenerateplate;
	BOOL	m_autogenerateplatehold;
	BOOL	m_autogenerateplateapproval;

	BOOL	m_autogenerateplatefreedevice;

	BOOL	m_transmitoriginalpdf;

	BOOL	m_callconditionalcopyscript;
	CString m_conditionalcopysp;

	BOOL	m_fixedpressgroup;
	CString m_defaultpressgroup;

	BOOL m_mailuseimap;
	CString m_mailserver;
	CString m_mailusername;
	CString m_mailpassword;
	BOOL m_mailusessl;
	CString m_mailimapinbox;
	CString m_mailfromlistfilter;
	int m_mailport;

	BOOL m_sendackfile;
	CString m_ackfilefolder;
	int m_ackflagvalue;

};

typedef CArray <INPUTPROCESSSTRUCT, INPUTPROCESSSTRUCT&> INPUTPROCESSLIST;



typedef struct ITEMSTRUCTTAG {
	int m_ID;
	CString m_name;
} ITEMSTRUCT;

typedef CArray <ITEMSTRUCT,ITEMSTRUCT&> ITEMLIST;





class STATUSSTRUCT {
public:
	STATUSSTRUCT() { m_statusnumber = 0; m_statusname = _T(""); m_statuscolor = _T("FFFFFF"); };
	int	m_statusnumber;
	CString m_statusname;
	CString m_statuscolor;
};

typedef CArray <STATUSSTRUCT, STATUSSTRUCT&> STATUSLIST;


typedef struct PUBLICATIONSTRUCTTAG {
	int			m_ID;
	CString		m_name;
	int			m_PageFormatID;
	int			m_TrimToFormat;
	double		m_LatestHour;
	int			m_ProofID;
	int			m_HardProofID;
	int			m_Approve;
//	PUBLICATIONPRESSDEFAULTSSTRUCT m_PressDefaults[MAXPRESSES];
	int			m_autoPurgeKeepDays;
	CString		m_emailrecipient;
	CString		m_emailCC;
	CString		m_emailSubject;
	CString		m_emailBody;
	int			m_customerID;
	CString		m_uploadfolder;
	CTime		m_deadline;
	int			m_tempdefafaultpressID;
	CString		m_channelIDList;
	BOOL		m_autoassignchannelstounplanned;
	BOOL		m_allowunplanned;


	CString		m_annumtext;

	int			m_releasedays;
	int			m_releasetimehour;
	int			m_releasetimeminute;


	BOOL		m_pubdatemove;
	int  		m_pubdatemovedays;
	CString		m_alias;
	CString		m_extendedalias;
	int m_publisherID;

} PUBLICATIONSTRUCT;

typedef CArray <PUBLICATIONSTRUCT, PUBLICATIONSTRUCT&> PUBLICATIONLIST;

class CHANNELSTRUCT {
public:
	CHANNELSTRUCT() {
		m_channelID = 0;
		m_channelname = _T("");
		m_enabled = TRUE;
		m_channelgroupID = 0;
		m_publisherID = 0;

		m_usereleasetime = FALSE;
		m_releasetimehour = 0;
		m_releasetimeminute = 0;
	};

	int		m_channelID;
	CString m_channelname;

	BOOL	m_enabled;

	int		m_channelgroupID;
	int     m_publisherID;
	BOOL	m_usereleasetime;
	int		m_releasetimehour;
	int		m_releasetimeminute;
};
typedef CArray <CHANNELSTRUCT, CHANNELSTRUCT&> CHANNELLIST;


class CHANNELGROUPSTRUCT {
public:
	CHANNELGROUPSTRUCT() {
		m_channelgroupID = 0;
		m_channelgroupname = _T("");
		m_channelgroupnamealias = _T("");
		m_transmitnameconv = _T("");
		m_transmitdateformat = _T("");
		m_transmitnameoptions = 0;
		m_transmitnameuseabbr = TRUE;
		m_pdftype = POLLINPUTTYPE_LOWRESPDF;
		m_editionstogenerate = 0;
		m_subfoldernameconv = _T("");
		m_sendCommonPages = FALSE;
		m_mergePDF = FALSE;


	};

	int		m_channelgroupID;
	CString m_channelgroupname;
	CString m_channelgroupnamealias;

	CString m_transmitnameconv;
	CString m_transmitdateformat;
	int		m_transmitnameoptions;
	int		m_transmitnameuseabbr;
	int		m_pdftype;
	BOOL	m_mergePDF;

	int		m_editionstogenerate;
	BOOL	m_sendCommonPages;
	CString m_subfoldernameconv;
};

typedef CArray <CHANNELGROUPSTRUCT, CHANNELGROUPSTRUCT&> CHANNELGROUPLIST;


typedef struct FILESERVERSTRUCTTAG {
	CString m_servername;
	CString m_sharename;
	int	m_servertype;
	CString m_IP;
	CString m_username;
	CString m_password;
	int	m_locationID;
	BOOL m_uselocaluser;
} FILESERVERSTRUCT;

typedef CArray <FILESERVERSTRUCT, FILESERVERSTRUCT&> FILESERVERLIST;

class CJobAttributes {
public:
	CJobAttributes() {};

	//~CJobAttributes() {
		//m_pagename.Empty();
	//};

	void InitJobAttributes(INPUTPROCESSSTRUCT *ip) {
		Init();
		m_ccdatafilename = _T("");
		m_inputID = ip->m_ID;
		m_guessedition = ip->m_guessedition;
		m_guesssection = ip->m_guesssection;
		m_guessrun = ip->m_guessrun;
		m_guesslocation = ip->m_guesslocation;

		m_sectionID = m_guesssection ? 0 : ip->m_defaultsectionID;
		m_editionID = m_guessedition ? 0 : ip->m_defaulteditionID;
		m_pressrunID = m_guessrun ? 0 : ip->m_defaultrunID;
		m_locationID = ip->m_guesslocation ? 0 : ip->m_defaultlocationID;
		m_commoneditionID = 0;
		m_pressID = 1;
		m_pressgroupID = 0;
		m_publicationID = ip->m_defaultpublicationID;
		m_templateID = ip->m_defaulttemplateID;
		m_prooftemplateID = ip->m_defaultprooftemplateID;

		m_copies = ip->m_defaultcopies;
		CTime tm = CTime(1975,1,1,0,0,0);
		//			if (tm.GetHour() <= 6)
		//	tm += CTimeSpan( 1, 0, 0, 0 );
		m_pubdate = tm;
		m_originalfilename = _T("");
		m_previousstatus = 0;
		m_previousactive = 1;
		m_weekreference = 0;
		m_version = 0;
		m_locationgroupIDList = _T("");
		m_locationgroup = _T("");
		m_miscstring1 = _T("");
		m_miscstring2 = _T("");
		m_miscstring3 = _T("");
		m_annumtext = _T("");
	}

	void Init()
	{
		m_planname = "";
		m_ccdatafilename = "";

		m_sectionID = 0;
		m_editionID = 0;
		m_pressrunID = 0;
		m_locationID = 0;
		m_publicationID = 0;
		m_templateID = 0;
		m_prooftemplateID = 0;
		m_copies = 1;

		m_deviceID = 0;
		m_copynumber = 1;
		m_pagename = _T("1");
		m_colorname = _T("K");
		CTime t = CTime::GetCurrentTime();	// Default to neext day
		t += CTimeSpan(1, 0, 0, 0);

		m_pubdate = t;
		m_auxpubdate = t;
		m_pubdateweekstart = t;
		m_pubdateweekend = t;
		m_hasweekpubdate = FALSE;

		m_version = 0;

		m_mastercopyseparationset = 1;
		m_copyseparationset = 1;
		m_separationset = 101;
		m_separation = 10101;
		m_copyflatseparationset = 1;
		m_flatseparationset = 101;
		m_flatseparation = 10101;

		m_numberofcolors = 1;
		m_colornumber = 1;

		m_layer = 1;
		m_filename = _T("");
		m_pagination = 0;
		m_pageindex = 0;

		m_priority = 50;
		m_comment = _T("");
		m_pageposition = 1;
		m_pagetype = PAGETYPE_NORMAL;
		m_pagesonplate = 1;
		m_hold = FALSE;
		m_approved = FALSE;
		m_active = TRUE;

		m_status = STATUSNUMBER_MISSING;
		m_externalstatus = 0;
		CTime t2;
		m_deadline = t2;
		m_pressID = 0;
		m_presssectionnumber = 0;
		m_sortnumber = 0;
		m_pubdatefound = FALSE;

		m_presstower = _T("1");
		m_presszone = _T("1");
		m_presscylinder = _T("1");
		m_presshighlow = _T("H");
		m_productionID = 0;
		m_inputID = 0;
		m_jobname = _T("");
		m_markgroup = _T("");
		m_includemarkgroup = 0;
		m_currentmarkgroup = _T("");

		m_commoneditionID = 0;

		m_originalfilename = _T("");

		m_previousstatus = 0;
		m_previousactive = TRUE;
		m_pressruncomment = "";
		m_pressruninkcomment = "";
		m_pressrunordernumber = "";

		m_customername = _T("");
		m_customeralias = _T("");
		m_customeremail = _T("");
		m_originalmastercopyseparationset = 0;
		m_panomate = _T("");
		m_miscstring1 = _T("");
		m_miscstring2 = _T("");
		m_miscstring3 = _T("");

		m_locationgroupIDList = _T("");
		m_locationgroup = _T("");

		m_publisherID = 0;

		m_annumtext = _T("");


};

	CString m_planname;
	CString m_ccdatafilename;

	CTime	m_pubdate;
	CTime	m_auxpubdate;
	CTime	m_pubdateweekstart;
	CTime	m_pubdateweekend;
	BOOL	m_hasweekpubdate;

	CString	m_pagename;	// page number!
	CString m_colorname;
	int		m_publicationID;
	int		m_sectionID;
	int		m_editionID;
	int		m_pressrunID;
	int		m_templateID;
	int		m_deviceID;
	int		m_prooftemplateID;
	int		m_locationID;
	int		m_pressID;
	int		m_inputID;
	int		m_pressgroupID;

	int		m_copies;
	int		m_copynumber;
	int		m_version;
	int		m_layer;
	CString m_filename;
	int		m_pagination;
	int		m_pageindex;
	int		m_priority;
	CString	m_comment;

	int		m_pageposition;
	int		m_pagetype;
	int		m_pagesonplate;
	int		m_colornumber;
	int		m_numberofcolors;

	BOOL	m_hold;
	int		m_approved;
	BOOL	m_active;

	int		m_sheetnumber;
	int		m_sheetside;

	int		m_mastercopyseparationset;
	int		m_copyseparationset;
	int		m_separationset;
	int		m_separation;
	int		m_copyflatseparationset;
	int		m_flatseparationset;
	int		m_flatseparation;

	int		m_status;
	int		m_externalstatus;
	CTime	m_deadline;
	int		m_presssectionnumber;
	int		m_sortnumber;
	BOOL	m_pubdatefound;

	CString	m_presstower;
	CString	m_presszone;
	CString	m_presscylinder;
	CString	m_presshighlow;

	int		m_productionID;

	int		m_guessedition;
	int		m_guesssection;
	int		m_guessrun;
	int		m_guesslocation;
	CString m_jobname;

	CString m_currentmarkgroup;
	CString m_markgroup;
	int		m_includemarkgroup;

	CString m_polledpublicationname;
	CString m_polledsectionname;
	CString m_pollededitionname;
	CString m_polledissuename;

	int		m_commoneditionID;
	CString m_originalfilename;

	int		m_previousstatus;
	BOOL	m_previousactive;

	int		m_weekreference;

	CString m_pressruncomment;
	CString m_pressruninkcomment;
	CString m_pressrunordernumber;

	CString m_customername;
	CString m_customeralias;
	CString m_customeremail;
	int		m_originalmastercopyseparationset;

	CString m_panomate;
	CString m_locationgroupIDList;
	CString m_locationgroup;

	CString m_miscstring1;
	CString m_miscstring2;
	CString	m_miscstring3;

	int m_publisherID;

	CString m_annumtext;


};

typedef CArray <CJobAttributes, CJobAttributes&> CJobAttributesList;



