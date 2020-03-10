#include "stdafx.h"
#include "Defs.h"
#include "Utils.h"
#include "Prefs.h"
#include "math.h"
#include <iostream>
#include "pdflib.h"
#include "PDFutils.h"
#include "DatabaseManager.h"

extern CPrefs g_prefs;
extern CUtils g_util;

//static CString PDFLIB_LICENSE = "W900202-010080-132518-UR4KH2-WDXFD2";
static CString PDFLIB_LICENSE = "W900202-010092-132518-5CDVH2-68MH72"; //- 29. Aug 2019


BOOL IsPDF(CString sFileName)
{
	FILE* fp;
	char data[8];

	TCHAR filename[MAX_PATH];
	strcpy(filename, sFileName);
	
	if ((fp = fopen( filename, "rb")) == 0) {
		return FALSE;
    }

//	%PDF-1.xx	

	if (fread(data, 1, 7, fp) != 7) {
		fclose(fp);
		return FALSE;
	}

	data[7] = 0;

	fclose(fp);

	if (!strcmp("%PDF-1.", data)) {
		return TRUE;
    }

	return FALSE;
}




BOOL GetPDFinfo(CString sInputFileName, int &nPageCount, double &fSizeX, double &fSizeY)
{
	CUtils util;
	BOOL bRet = TRUE;
	CString sErrorMessage;

	TCHAR szFeature[200];
	_tcscpy(szFeature, "");

	int doc = -1;
	int page = -1;
	char *searchpath = (char *) "../data";

	PDF *p = PDF_new();
	if (p == (PDF *)0) {
		sErrorMessage.Format( "ERROR: Couldn't create PDFlib object (out of memory)!");
		util.Logprintf("ERROR: PDFInfo() %s", sErrorMessage);
		return FALSE;
	}
	PDF_TRY(p) {

		PDF_set_option(p, "errorpolicy=return");
		PDF_set_option(p, "license=" + PDFLIB_LICENSE);

		/*if (PDF_begin_document(p, g_util.GetTempFolder() + _T("\\dummy.pdf"), 0, "") == -1) {
			util.Logprintf("ERROR: PDF_begin_document failed");
			PDF_delete(p);
			return FALSE;
		}*/


		doc = PDF_open_pdi_document(p, sInputFileName, 0, "");
		if (doc == -1) {
			sErrorMessage.Format( "Couldn't open PDF input file '%s'", sInputFileName);
			util.Logprintf("ERROR: PDFInfo() %s", sErrorMessage);
			PDF_delete(p);
			return FALSE;
		}

		int  numberofpages = (int)PDF_pcos_get_number(p, doc, "/Root/Pages/Count");
		nPageCount = numberofpages;

		// Get documnet information from input file
	//	util.Logprintf("INFO: PDFlib PDF_open_pdi called");

	/*	page = PDF_open_pdi_page(p, doc, 1, "");
		if (page == -1) {
			sErrorMessage.Format( "Couldn't open page 1 of PDF file '%s'", sInputFileName);
			util.Logprintf("ERROR: PDFInfo() %s", sErrorMessage);
			PDF_delete(p);
			return FALSE;
		}*/
		//	util.Logprintf("INFO: PDFlib PDF_open_pdi_page called");

		double width = PDF_pcos_get_number(p, doc, "pages[0]/width");
		double height = PDF_pcos_get_number(p, doc, "pages[0]/height");

		fSizeX = width * 25.4 / 72.0;
		fSizeY = height * 25.4 / 72.0;
			util.Logprintf("INFO: PDF size %.4f,%.4f", width, height);


	//	PDF_begin_page_ext(p, width, height, "");
			
	//	PDF_fit_pdi_page(p, page, 0, 0, "");
		PDF_close_pdi_document(p, doc);

	//	PDF_end_page_ext(p, "");
	//	PDF_end_document(p, "");
	//	DeleteFile("c:\\temp\\dummy.pdf");

	}
	PDF_CATCH(p) {
		sErrorMessage.Format("PDFlib exception occurred: [%d] %s: %s",
			PDF_get_errnum(p), PDF_get_apiname(p), PDF_get_errmsg(p));
		PDF_delete(p);
		util.Logprintf("ERROR: PDFInfo() %s", sErrorMessage);
		return FALSE;
	}
	PDF_delete(p);
	return TRUE;

}




BOOL SplitPDF(CString sInputFileName, CString sFileTitle, CString sOutputFolder, int nPageNumberOffset, CString &sInfo, int &nPageCount, CString &sErrorMessage)
{
	
	BOOL bRet = TRUE;

	sErrorMessage = _T("");
	sInfo = _T( "");
	nPageCount = 0;

	if (sFileTitle == _T(""))
		sFileTitle = g_util.GetFileName(sInputFileName, TRUE);

	TCHAR szFeature[200];
	TCHAR szOptions[100];
	_tcscpy(szFeature, "");

	CString sTempPDF, sFinalName;

	int doc = -1;
	int page = -1;
	char *searchpath = (char *) "../data";


	PDF *p = PDF_new();
	if (p == (PDF *)0) {
		sErrorMessage.Format("ERROR: Couldn't create PDFlib object (out of memory)!");
		g_util.Logprintf("ERROR: %s", sErrorMessage);
		return FALSE;
	}
	PDF_TRY(p) {


		PDF_set_option(p, "errorpolicy=return");
		PDF_set_option(p, "license=" + PDFLIB_LICENSE);


		doc = PDF_open_pdi_document(p, sInputFileName, 0, "");
		if (doc == -1) {
			sErrorMessage.Format( "Couldn't open PDF input file '%s'", sInputFileName);
			g_util.Logprintf("ERROR: %s", sErrorMessage);
			PDF_delete(p);
			return FALSE;
		}

		// Get documnet information from input file

		nPageCount = (int)PDF_pcos_get_number(p, doc, "/Root/Pages/Count");
		double version = PDF_pcos_get_number(p, doc, "pdfversion");


		if (nPageCount == 0) {
			sErrorMessage.Format("Couldn't not get PDF page count from PDF file '%s'", sInputFileName);
			g_util.Logprintf("ERROR: %s", sErrorMessage);
			PDF_delete(p);
			return FALSE;
		}

		/*if (nPageCount == 1) {
			PDF_delete(p);
			return TRUE;
		}*/

		for (int i = 1; i <= nPageCount; i++) {

			// Form output name of this page
			CString sEndFileName;
			if (nPageNumberOffset == 0)
				sEndFileName.Format("-%d.pdf", i);
			else
				sEndFileName.Format("-%d.pdf", i + nPageNumberOffset - 1);

			sTempPDF.Format("%s\\%s%s", g_util.GetTempFolder(), g_util.GetFileName(sFileTitle,TRUE), sEndFileName);
			
			
			sFinalName = sOutputFolder + _T("\\") + g_util.GetFileName(sTempPDF, TRUE) + _T(".pdf");
			
			sprintf(szOptions, "compatibility=%.1f", version / 10.0);

			if (PDF_begin_document(p, sTempPDF, 0, szOptions) == -1) {

				sErrorMessage.Format("%s\n", PDF_get_errmsg(p));
				//	PDF_close_pdi_page(p, page);
				g_util.Logprintf("ERROR: %s", sErrorMessage);
				PDF_delete(p);
				return FALSE;
			}

			PDF_set_info(p, "Creator", "InfraLogic");
			PDF_set_info(p, "Author", "PDFsplitter");
			PDF_set_info(p, "Title", sFileTitle);

			page = PDF_open_pdi_page(p, doc, i, "cloneboxes");

			if (page == -1) {
				sErrorMessage.Format("Couldn't open page %d of PDF file '%s'", i, sInputFileName);
				g_util.Logprintf("ERROR: %s", sErrorMessage);
				PDF_delete(p);
				return FALSE;
			}

			int infolen = 0;

			TCHAR szStr[40];
			sprintf(szStr, "pages[%d]/width", i - 1);
			double width = PDF_pcos_get_number(p, doc, szStr);
			sprintf(szStr, "pages[%d]/height", i - 1);
			double height = PDF_pcos_get_number(p, doc, szStr);

		
			PDF_begin_page_ext(p, 10, 10, "");

			PDF_fit_pdi_page(p, page, 0, 0, "cloneboxes");
			PDF_close_pdi_page(p, page);

			PDF_end_page_ext(p, "");
			PDF_end_document(p, "");


			if (::MoveFileEx(sTempPDF, sFinalName, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == FALSE) {
				::Sleep(500);
				if (::MoveFileEx(sTempPDF, sFinalName, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == FALSE) {
					sErrorMessage.Format("Unable to move file %s to %s", sTempPDF, sFinalName);
					g_util.Logprintf("ERROR: %s", sErrorMessage);
					return -1;
				}
			}
		}
	}
	PDF_CATCH(p) {
		sErrorMessage.Format("PDFlib exception occurred: [%d] %s: %s",
			PDF_get_errnum(p), PDF_get_apiname(p), PDF_get_errmsg(p));
		PDF_delete(p);
		g_util.Logprintf("ERROR %s", sErrorMessage);
		return FALSE;
	}
	PDF_delete(p);
	return TRUE;
}

BOOL LogPDFinfo(CString sInputFileName, BOOL &bIsNotGrayOnly, CString &sReport)
{
	CUtils util;
	BOOL bRet = TRUE;
	TCHAR szErrorMessage[4096];

	sReport = _T("");
	CString sInfo;

	TCHAR szFeature[200];
	_tcscpy(szFeature, "");

	bIsNotGrayOnly = FALSE;

	if (g_prefs.m_bypasspdfinspection)
		return TRUE;

	int doc = -1;
	int page = -1;
	char *searchpath = (char *) "../data";

	//	util.Logprintf("\r\nINFO: PDF information for file %s :", sInputFileName);
	sInfo.Format("PDF information for file %s :", sInputFileName);
	sReport += sInfo + "\r\n";

	PDF *p = PDF_new();
	if (p == (PDF *)0) {
		_stprintf(szErrorMessage, "ERROR: Couldn't create PDFlib object (out of memory)!");
		util.Logprintf("ERROR: %s", szErrorMessage);
		return FALSE;
	}
	PDF_TRY(p) {
		PDF_set_option(p, "errorpolicy=return");
		PDF_set_option(p, "license=" + PDFLIB_LICENSE);

		TCHAR filename[MAX_PATH];
		_tcscpy(filename, sInputFileName);

		doc = PDF_open_pdi_document(p, filename, 0, "");
		if (doc == -1) {
			_stprintf(szErrorMessage, "Couldn't open PDF input file '%s'", filename);
			util.Logprintf("ERROR: %s", szErrorMessage);
			PDF_delete(p);
			return FALSE;
		}

		int pcosmode = (int)PDF_pcos_get_number(p, doc, "pcosmode");
		util.Logprintf("INFO: PDF pcos mode %d", pcosmode);

		CString s = PDF_pcos_get_string(p, doc, "pdfversionstring");
		util.Logprintf("INFO: PDF version %s", s);
		sInfo.Format("PDF version %s", s);
		sReport += sInfo + "\r\n";
		s = PDF_pcos_get_string(p, doc, "encrypt/description");
		util.Logprintf("INFO: PDF Encryption %s", s);
		sInfo.Format("Encryption %s", s);
		sReport += sInfo + "\r\n";

		int  numberofpages = (int)PDF_pcos_get_number(p, doc, "length:pages");
		util.Logprintf("INFO: PDF number of pages %d", numberofpages);
		sInfo.Format("Number of pages %d", numberofpages);
		sReport += sInfo + "\r\n";

		double width = PDF_pcos_get_number(p, doc, "pages[0]/width");
		double height = PDF_pcos_get_number(p, doc, "pages[0]/height");
		util.Logprintf("INFO: PDF width %.4f, height %.4f", width*25.4 / 72.0, height*25.4 / 72.0);
		sInfo.Format("Page width %.4f, height %.4f", width*25.4 / 72.0, height*25.4 / 72.0);
		sReport += sInfo + "\r\n";

		int count = (int)PDF_pcos_get_number(p, doc, "length:fonts");
		util.Logprintf("INFO: PDF No. of fonts: %d", count);
		sInfo.Format("Number of fonts: %d", count);
		sReport += sInfo + "\r\n";

		for (int i = 0; i < count; i++) {
			CString s1 = "";
			if (PDF_pcos_get_number(p, doc, "fonts[%d]/embedded", i) != 0)
				s1 = "Embedded ";
			else
				s1 = "Unembedded ";
			CString s2 = PDF_pcos_get_string(p, doc, "fonts[%d]/type", i);
			util.Logprintf("INFO:  %s %s font %s", s1, s2, PDF_pcos_get_string(p, doc, "fonts[%d]/name", i));
			sInfo.Format("  %s %s font %s", s1, s2, PDF_pcos_get_string(p, doc, "fonts[%d]/name", i));
			if (sReport.GetLength() < 2048)
				sReport += sInfo + "\r\n";
		}


		int plainmetadata = PDF_pcos_get_number(p, doc, "encrypt/plainmetadata") != 0;
		if (plainmetadata) {
			util.Logprintf("INFO: PDF Plain meta data available");
			sInfo.Format("Plain meta data available");
			sReport += sInfo + "\r\n";
		}

		count = (int)PDF_pcos_get_number(p, doc, "length:colorspaces");
		util.Logprintf("INFO: PDF No. of colorspaces: %d", count);
		sInfo.Format("Number of colorspaces: %d", count);
		if (sReport.GetLength() < 2048)
			sReport += sInfo + "\r\n";

		//	if (count > 10) {
		//		count = 10;
		//      util.Logprintf("WARNING: Limiting listed colorspaces to 10);
		//	}

		CStringArray asColorSpaces;
		for (int i = 0; i < count; i++) {
			CString sColorSpace = PDF_pcos_get_string(p, doc, "colorspaces[%d]/name", i);
			BOOL bFound = FALSE;
			for (int j = 0; j < asColorSpaces.GetCount(); j++) {
				if (asColorSpaces[j] == sColorSpace) {
					bFound = TRUE;
					break;
				}
			}
			if (bFound == FALSE)
				asColorSpaces.Add(sColorSpace);
		}
		for (int i = 0; i < asColorSpaces.GetCount(); i++) {
			CString sColorSpace = asColorSpaces[i];
			util.Logprintf("INFO:  Colorspace name %s", sColorSpace);
			sInfo.Format("  Colorspace name %s", sColorSpace);
			if (sReport.GetLength() < 2048)
				sReport += sInfo + "\r\n";
			sColorSpace.MakeUpper();
			if (sColorSpace != "DEVICEGRAY")
				bIsNotGrayOnly = TRUE;
			if (i >= 10) {
				util.Logprintf("WARNING: Limiting listed colorspaces to 10");
				break;
			}
		}

		count = (int)PDF_pcos_get_number(p, doc, "length:/Info");
		if (count > 0) {
			util.Logprintf("INFO: PDF Information : ");
			sInfo.Format("PDF Information : ");
			sReport += sInfo + "\r\n";
		}
		for (int i = 0; i < count; i++) {
			TCHAR sz[512];
			int objtype = (int)PDF_pcos_get_number(p, doc, "type:/Info[%d]", i);
			sprintf(sz, "%12s: ", PDF_pcos_get_string(p, doc, "/Info[%d].key", i));

			/* Info entries can be stored as string or name objects */
			if (objtype == pcos_ot_name || objtype == pcos_ot_string)
				sInfo.Format("  %s : '%s'", sz, PDF_pcos_get_string(p, doc, "/Info[%d]", i));
			else
				sInfo.Format("  %s : (%s object)", sz, PDF_pcos_get_string(p, doc, "type:/Info[%d]", i));
			if (sReport.GetLength() < 2048)
				sReport += sInfo + "\r\n";

		}

		int objtype = (int)PDF_pcos_get_number(p, doc, "type:/Root/Metadata");
		if (objtype == pcos_ot_stream) {
			const unsigned char *contents;
			int len;
			contents = PDF_pcos_get_stream(p, doc, &len, "", "/Root/Metadata");
			sInfo.Format("XMP metadata: %d bytes", len);
			sReport += sInfo + "\r\n";
			CString sContent(contents);
			sContent.Replace('\t', ' ');
			sInfo.Format("  XMP metadata:\r\b%s", sContent);
		}
		else {
			sInfo.Format("XMP metadata not present");
			if (sReport.GetLength() < 2048)
				sReport += sInfo + "\r\n";
		}

		PDF_close_pdi_document(p, doc);

	}
	PDF_CATCH(p) {
		_stprintf(szErrorMessage, "PDFlib exception occurred: [%d] %s: %s",
			PDF_get_errnum(p), PDF_get_apiname(p), PDF_get_errmsg(p));
		PDF_delete(p);
		util.Logprintf("ERROR %s", szErrorMessage);
		return FALSE;
	}
	PDF_delete(p);
	if (sReport.GetLength() < 2048)
		sReport = sReport.Left(2047);
	return TRUE;

}
