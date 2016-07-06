
// FFMpeg_transcoderDlg.h : header file
//

#pragma once

#include "FFMpeg_decoder.h"
#include "FFMPEGClass.h"
#include "afxcmn.h"
#include "afxwin.h"

// CFFMpeg_transcoderDlg dialog
class CFFMpeg_transcoderDlg : public CDialogEx
{
// Construction
public:
	CFFMpeg_transcoderDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_FFMPEG_TRANSCODER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnLoad();
	// 추가변수
public:
	FFMpeg_decoder m_decoder;
	FFMPEG m_encoder;
	bool m_isVideoLoaded;
	int m_width;
	int m_height;
	int m_totalFrame;
	int64_t m_dur;
	double m_fps;
	int m_bitrate;
public:
	CProgressCtrl m_progCtrl;
	afx_msg void OnBnClickedBtnSave();
	CStatic m_txtStart;
	CStatic m_txtEnd;
};
