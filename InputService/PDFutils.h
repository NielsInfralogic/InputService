#pragma once

BOOL IsPDF(CString sFileName);
BOOL GetPDFinfo(CString sInputFileName, int &nPageCount, double &fSizeX, double &fSizeY);
BOOL SplitPDF(CString sInputFileName, CString sFileTitle, CString sOutputFolder, int nPageNumberOffset, CString &sInfo, int &nPageCount, CString &sErrorMessage);
BOOL LogPDFinfo(CString sInputFileName, BOOL &bIsNotGrayOnly, CString &sReport);