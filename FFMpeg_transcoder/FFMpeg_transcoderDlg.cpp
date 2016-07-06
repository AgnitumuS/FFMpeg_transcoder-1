
// FFMpeg_transcoderDlg.cpp : implementation file
//

#include "stdafx.h"
#include "FFMpeg_transcoder.h"
#include "FFMpeg_transcoderDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CFFMpeg_transcoderDlg dialog




CFFMpeg_transcoderDlg::CFFMpeg_transcoderDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CFFMpeg_transcoderDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFFMpeg_transcoderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS1, m_progCtrl);
	DDX_Control(pDX, IDC_TXT_END, m_txtStart);
	DDX_Control(pDX, IDC_TXT_START, m_txtEnd);
}

BEGIN_MESSAGE_MAP(CFFMpeg_transcoderDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_LOAD, &CFFMpeg_transcoderDlg::OnBnClickedBtnLoad)
	ON_BN_CLICKED(IDC_BTN_SAVE, &CFFMpeg_transcoderDlg::OnBnClickedBtnSave)
END_MESSAGE_MAP()


// CFFMpeg_transcoderDlg message handlers

BOOL CFFMpeg_transcoderDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	m_isVideoLoaded = false;
	
	av_register_all(); // FFmpeg의 모든 코덱을 등록한다.
	avcodec_register_all();
	m_progCtrl.SetRange(0, 1);
	m_progCtrl.SetPos(0);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CFFMpeg_transcoderDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CFFMpeg_transcoderDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CFFMpeg_transcoderDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CFFMpeg_transcoderDlg::OnBnClickedBtnLoad()
{
	// TODO: Add your control notification handler code here
	CFileDialog dlg(true, NULL, NULL, NULL, NULL, NULL);

	if( dlg.DoModal() == IDOK )
	{
		CString strFilePath = dlg.GetPathName();
		//const char* str2 = (CStringA)strFilePath;
		bool ret = false;
		m_decoder.Clear();
		ret = m_decoder.OpenInputFile(strFilePath);
		ret = m_decoder.FindStreamInfo();
		ret = m_decoder.FindVideoStream();
		ret = m_decoder.FindVideoCodec();
		ret = m_decoder.OpenVideoCodec();
		ret = m_decoder.GetVideoSize(m_width, m_height);
		m_isVideoLoaded = ret;
		m_totalFrame = m_decoder.m_totalFrame;
		m_dur = m_decoder.m_dur;
		m_fps = m_decoder.m_fps;
		m_bitrate = m_decoder.m_bitrate;
	}
	if(m_isVideoLoaded){
		AfxMessageBox("Success to load video");
		m_progCtrl.SetRange(0, 100);
		m_progCtrl.SetPos(0);
		char buf[1024];
		itoa(m_totalFrame, buf, 10);
		m_txtEnd.SetWindowText(buf);
	}
	else
		AfxMessageBox("Failed to load video");

}

UINT Thread_Run_Save(LPVOID dParam){
	CFFMpeg_transcoderDlg* dlg = (CFFMpeg_transcoderDlg*)dParam;

	dlg->m_decoder.ReadFrame(&dlg->m_encoder);
	dlg->m_isVideoLoaded = false;
	::AfxMessageBox("Finisied to encode video");
	return 1;
}

UINT Thread_Get_Progress(LPVOID dParam){
	CFFMpeg_transcoderDlg* dlg = (CFFMpeg_transcoderDlg*)dParam;

	char buf[1024];
	while ( dlg->m_isVideoLoaded ){
		dlg->m_progCtrl.SetPos(((double)dlg->m_decoder.m_progress/dlg->m_totalFrame)*100);
		
		itoa(dlg->m_decoder.m_progress, buf, 10);
		dlg->m_txtStart.SetWindowText(buf);
		//dlg->Invalidate(false);
	}

	return 1;
}

void CFFMpeg_transcoderDlg::OnBnClickedBtnSave()
{
	// TODO: Add your control notification handler code here
	if( !m_isVideoLoaded )
		return;
	
	CFileDialog dlg(false, NULL, NULL, NULL, NULL, NULL);

	if( dlg.DoModal() == IDOK )
	{
		CString strFilePath = dlg.GetPathName();
		m_encoder.CloseVideo();
		m_encoder.SetupVideo(strFilePath.GetBuffer(), m_width, m_height, m_fps, m_fps, m_bitrate);
		AfxBeginThread(Thread_Run_Save, this);
		AfxBeginThread(Thread_Get_Progress, this);
	}
}
