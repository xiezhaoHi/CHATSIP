
// VOICECHATDlg.h : ͷ�ļ�
//

#pragma once


// CVOICECHATDlg �Ի���
class CVOICECHATDlg : public CDialogEx
{
// ����
public:
	CVOICECHATDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_VOICECHAT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnUserLogin();
	afx_msg void OnAddFriend();
	afx_msg void OnCallUser();
	afx_msg void OnCallEnd();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
};
