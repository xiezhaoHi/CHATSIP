
// VOICECHATDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "VOICECHAT.h"
#include "VOICECHATDlg.h"
#include "afxdialogex.h"
#include "logrecord/LogRecord.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "MyDataBase.h"
#include "linphone/lpconfig.h"
#include "linphone/linphonecore.h"
#include "ortp/payloadtype.h"
#include <bctoolbox/vfs.h>
#include "linphone/presence.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include <afxtempl.h>
#include "linphone/core.h"
// #include "linphone.h"
// #include "linphone/lpconfig.h"
// #include "liblinphone_gitversion.h"
// #include <bctoolbox/vfs.h>
#include <stdio.h>
#include <string>
#include <vector>
using namespace std;

static LinphoneCore* g_lc;
#pragma wanning(disable:4996)

static LinphoneCall* g_newCall = nullptr;
static LinphoneCore* the_core = NULL;
static float gVolum = 1; //���� [0-1]
static CString gstrAddr = "192.168.1.128";
static CString gstrAddrPort = "5060";
static CString gstrWndClsName; // ��������
static CString gstrWndName; //������
static HWND  gHwnd = nullptr; //�����ھ��
static CString gstrUserID; //��½�û�Ψһ��ʶ
static CString gstrCallUserID; //ͨ�����û�ID
static CMap<CString, LPCTSTR, CString, LPCTSTR>	 gmapUserTOPhone; //�û�ID ��Ӧ ����
static CMap<CString, LPCTSTR, CString, LPCTSTR>  gmapPhoneToUser; //���� ��Ӧ �û�ID
static CMap<CString, LPCTSTR, int, int> gmapUserStatus;//�û���Ӧ��״̬
typedef struct deviceStatus //�����豸״̬
{
	CString strDeviceID; //�豸ID
	int		iType; //�豸���� 
	bool	bUsing; //ʹ��״�� 0 1
}deviceStatus;
static CMap<CString, LPCTSTR, deviceStatus, deviceStatus> gmapDeviceStatus; //�豸״̬
static int gUSERTYP =USERTYPE_2; //�û�����   Ĭ���� ��ͨ�û�
#define UNSIGNIFICANT_VOLUME (-23)
#define SMOOTH 0.15f
float gLastValue = 1.0;
static CVOICECHATDlg* gVoiceChatDlg = nullptr;

//����
static void InitUserMap(); //��� �û� �� �����map
static void InitUserFriend(); //��ʼ�� �û�����
static void InitAudioDevice(); //��ʼ�� �����豸
//��ԽӴ��ڷ�����Ϣ
static void sendMSGToWnd(CString strMSG)
{
	HWND hWnd = FindWindow(NULL, gstrWndName);
	if (hWnd)
	{
		COPYDATASTRUCT copyData = { 0 };
		copyData.dwData = 1;
		copyData.lpData = strMSG.GetBuffer();
		copyData.cbData = strMSG.GetLength()+1;
		DWORD  ret;
		::SendMessageTimeout(hWnd, WM_COPYDATA, (WPARAM)gHwnd, (LPARAM)&copyData
			, SMTO_NORMAL,100,&ret);

	}
}

//��� ������Ϣ֪ͨ

//���ʹ�����Ϣ
static void sendErrInfo(CString strErr)
{
	
	CString strMsg;
	strMsg.Format("%d#%s#", PRO_ERR, strErr);
	sendMSGToWnd(strMsg);
}



static LinphoneCore *linphone_get_core(void) {
	return the_core;
}
//call
static bool linphone_start_call_do(CString strSIPAddr) {
	const char *entered = strSIPAddr.GetString();
	LinphoneCore *lc = linphone_get_core();
	LinphoneAddress *addr = linphone_core_interpret_url(lc, entered);
	//AfxMessageBox("1");
	if (addr != NULL) {
		LinphoneCallParams *params = linphone_core_create_call_params(lc, NULL);
		//gchar *record_file = linphone_get_record_path(addr, FALSE);
		//linphone_call_params_set_record_file(params, record_file);
		//AfxMessageBox("2");
		linphone_core_invite_address_with_params(lc, addr, params);
		//AfxMessageBox("3");
		//completion_add_text(GTK_ENTRY(uri_bar), entered);
		linphone_address_unref(addr);
		linphone_call_params_unref(params);
		//AfxMessageBox("4");

	}

	return FALSE;
}


static void linphone_auth_info_requested(LinphoneCore *lc, const char *realm, const char *username, const char *domain) {
	// 	GtkWidget *w=linphone_create_window("password", the_ui);
	// 	GtkWidget *label=linphone_get_widget(w,"message");
	LinphoneAuthInfo *info;
	// 	gchar *msg;
	// 	GtkWidget *mw=linphone_get_main_window();
	// 
	// 	if (mw && g_object_get_data(G_OBJECT(mw), "login_frame") != NULL){
	// 		/*don't prompt for authentication when login frame is visible*/
	// 		linphone_core_abort_authentication(lc,NULL);
	// 		return;
	// 	}
	// 
	// 	msg=g_strdup_printf(_("Please enter your password for username <i>%s</i>\n at realm <i>%s</i>:"),
	// 		username,realm);
	// 	gtk_label_set_markup(GTK_LABEL(label),msg);
	// 	g_free(msg);
	// 	gtk_entry_set_text(GTK_ENTRY(linphone_get_widget(w,"userid_entry")),username);
	info = linphone_auth_info_new(username, username, NULL, NULL, realm, domain);

	linphone_auth_info_set_passwd(info, "xiezhao");
	linphone_auth_info_set_userid(info, username);
	linphone_core_add_auth_info(lc, info);

	// 	g_object_set_data(G_OBJECT(w),"auth_info",info);
	// 	g_object_weak_ref(G_OBJECT(w),(GWeakNotify)linphone_auth_info_destroy,info);
	// 	gtk_widget_show(w);
	// 	auth_timeout_new(w);
}

//���� �û�strAddr  sip:1000@192.168.1.5:15060  strProxy sip:192.168.1.5:15060
static void linphone_proxy_ok(LinphoneCore *lc, string strAddr, string strProxy) {

#if 1
	//linphone_core_clear_proxy_config(lc);
	BOOL  bFirst = FALSE;
	LinphoneProxyConfig *cfg = linphone_core_get_default_proxy_config(lc);
	//LinphoneProxyConfig *cfg = linphone_proxy_config_new();
	if (cfg == nullptr)
	{
		cfg = linphone_proxy_config_new();
		bFirst = TRUE;
	}
// 	else
// 	{
// 		linphone_core_set_presence_model(lc
// 			, linphone_presence_model_new_with_activity(LinphonePresenceActivityOffline, NULL));
// 	}
	
	int index = 0; //udp
	LinphoneTransportType tport = (LinphoneTransportType)index;

	linphone_proxy_config_set_identity(cfg, strAddr.c_str());
	if (linphone_proxy_config_set_server_addr(cfg, strProxy.c_str()) == 0) {
		if (index != -1) {
			/*make sure transport was added to proxy address*/
			LinphoneAddress *laddr = linphone_address_new(linphone_proxy_config_get_addr(cfg));
			if (laddr) {
				if (linphone_address_get_transport(laddr) != tport) {
					char *tmp;
					linphone_address_set_transport(laddr, tport);
					tmp = linphone_address_as_string(laddr);
					linphone_proxy_config_set_server_addr(cfg, tmp);
					ms_free(tmp);
				}
				linphone_address_unref(laddr);
			}
		}
	}
	// 	linphone_proxy_config_set_route(cfg,
	// 		gtk_entry_get_text(GTK_ENTRY(linphone_get_widget(w, "route"))));
	// 	linphone_proxy_config_set_contact_parameters(cfg,
	// 		gtk_entry_get_text(GTK_ENTRY(linphone_get_widget(w, "params"))));
	// 	linphone_proxy_config_set_expires(cfg,
	// 		(int)gtk_spin_button_get_value(
	// 			GTK_SPIN_BUTTON(linphone_get_widget(w, "regperiod"))));
	linphone_proxy_config_enable_publish(cfg, TRUE);
	linphone_proxy_config_enable_register(cfg,
		TRUE);
	// 	linphone_proxy_config_enable_avpf(cfg,
	// 		gtk_toggle_button_get_active(
	// 			GTK_TOGGLE_BUTTON(linphone_get_widget(w, "avpf"))));
	linphone_proxy_config_set_avpf_rr_interval(cfg,
		5);

	/* check if tls was asked but is not enabled in transport configuration*/
	if (tport == LinphoneTransportTls) {
		LCSipTransports tports;
		linphone_core_get_sip_transports(lc, &tports);
		if (tports.tls_port == LC_SIP_TRANSPORT_DISABLED) {
			tports.tls_port = LC_SIP_TRANSPORT_RANDOM;
		}
		linphone_core_set_sip_transports(lc, &tports);
	}

	if (!bFirst) {
		const bctbx_list_t* list = linphone_core_get_proxy_config_list(lc);
		const bctbx_list_t* elem;
		for (elem = list; elem != NULL; elem = elem->next) {
			LinphoneProxyConfig* pTemp = (LinphoneProxyConfig*)elem->data;
			if (pTemp == cfg && linphone_proxy_config_done(cfg) == -1)
				return;
		}
		
	}
	else {
		
		if (linphone_core_add_proxy_config(lc, cfg) == -1) return;
		linphone_core_set_default_proxy(lc, cfg);
	}
#endif

	
}

//sip ע�� �ص�����
static void linphone_registration_state_changed(LinphoneCore *lc, LinphoneProxyConfig *cfg,
	LinphoneRegistrationState rs, const char *msg) {

	LinphoneFriend* my_friend;
	CString strMsg;
	switch (rs) {
	case LinphoneRegistrationOk:
		if (cfg) {
			
			strMsg.Format("%d#%s#%d#", PRO_USER_STATUS, gstrUserID, USER_ON);
			sendMSGToWnd(strMsg);

			//linphone_core_set_presence_model(g_lc, linphone_presence_model_new_with_activity((LinphonePresenceActivityType)1, NULL));
		}
		break;
	case LinphoneRegistrationProgress:
		if (!gstrUserID.IsEmpty())
		{
			strMsg.Format("%d#%s#%d#", PRO_USER_STATUS, gstrUserID, USER_BEGIN);
			sendMSGToWnd(strMsg);
		}
		break;
	case LinphoneRegistrationCleared:

		break;
	case LinphoneRegistrationFailed:
		//AfxMessageBox("ע��ʧ��");
		
		strMsg.Format("%d#%s#%d#", PRO_USER_STATUS, gstrUserID, USER_OFF);
		sendMSGToWnd(strMsg);
		break;
	default:
		break;
	}
	CLogRecord::WriteRecordToFile(strMsg);
	//update_registration_status(cfg, rs);
}

void linphone_create_in_call_view(LinphoneCall *call) {

	linphone_call_ref(call);
	return;

}

void linphone_in_call_view_set_calling(LinphoneCall *call) {
	// 	GtkWidget *callview = (GtkWidget*)linphone_call_get_user_pointer(call);
	// 	GtkWidget *status = linphone_get_widget(callview, "in_call_status");
	// 	GtkWidget *callee = linphone_get_widget(callview, "in_call_uri");
	// 	GtkWidget *duration = linphone_get_widget(callview, "in_call_duration");
	// 
	// 	gtk_label_set_markup(GTK_LABEL(status), _("<b>Calling...</b>"));
	// 	display_peer_name_in_label(callee, linphone_call_get_remote_address(call));
	// 
	// 	gtk_label_set_text(GTK_LABEL(duration), _("00:00:00"));
	// 	linphone_in_call_set_animation_spinner(callview);
}
void linphone_in_call_view_set_in_call(LinphoneCall *call) {

	gLastValue = 1.0; //��ʼ��
	g_newCall = call;
	float speakerGain = linphone_call_get_speaker_volume_gain(call);
	float playGain = linphone_call_get_microphone_volume_gain(call);
	int err = GetLastError();
	CString strLog;
	strLog.Format("����ͨ��,���ͷ���������[%f],���ܷ���������[%f]", speakerGain, playGain);
	CLogRecord::WriteRecordToFile(strLog);
	//��������
	linphone_call_set_speaker_volume_gain(call, (float)gVolum);

	linphone_call_set_microphone_volume_gain(call, (float)gVolum);

	 speakerGain = linphone_call_get_speaker_volume_gain(call);
	 playGain = linphone_call_get_microphone_volume_gain(call);
	 err = GetLastError();
}
static void linphone_call_updated_by_remote(LinphoneCall *call) {
	LinphoneCore *lc = linphone_call_get_core(call);
	const LinphoneVideoPolicy *pol = linphone_core_get_video_policy(lc);
	const LinphoneCallParams *rparams = linphone_call_get_remote_params(call);
	const LinphoneCallParams *current_params = linphone_call_get_current_params(call);
	bool video_requested = linphone_call_params_video_enabled(rparams);
	bool video_used = linphone_call_params_video_enabled(current_params);

	if (!video_used && video_requested && !pol->automatically_accept) {
		linphone_core_defer_call_update(lc, call);
		{
			const LinphoneAddress *addr = linphone_call_get_remote_address(call);

			const char *dname = linphone_address_get_display_name(addr);
			if (dname == NULL) dname = linphone_address_get_username(addr);
			if (dname == NULL) dname = linphone_address_get_domain(addr);
			// 			dialog = gtk_message_dialog_new(GTK_WINDOW(linphone_get_main_window()),
			// 				GTK_DIALOG_DESTROY_WITH_PARENT,
			// 				GTK_MESSAGE_WARNING,
			// 				GTK_BUTTONS_YES_NO,
			// 				_("%s proposed to start video. Do you accept ?"), dname);
			// 			g_object_set_data_full(G_OBJECT(dialog), "call", linphone_call_ref(call), (GDestroyNotify)linphone_call_unref);
			// 			g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(on_call_updated_response), NULL);
			// 			g_timeout_add(20000, (GSourceFunc)on_call_updated_timeout, dialog);
			// 			gtk_widget_show(dialog);
		}
	}
}
void linphone_in_call_view_terminate(LinphoneCall *call, const char *error_msg) {
	linphone_core_terminate_call(linphone_get_core(), call);
	// 	GtkWidget *status;
	// 	GtkWidget *video_window;
	// 	bool in_conf;
	// 	guint taskid;
	// 	if (callview == NULL) return;
	// 	video_window = (GtkWidget*)g_object_get_data(G_OBJECT(callview), "video_window");
	// 	status = linphone_get_widget(callview, "in_call_status");
	// 	taskid = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(callview), "taskid"));
	// 	in_conf = (linphone_call_get_conference(call) != NULL);
	// 	if (video_window) gtk_widget_destroy(video_window);
	// 	if (status == NULL) return;
	// 	if (error_msg == NULL)
	// 		gtk_label_set_markup(GTK_LABEL(status), _("<b>Call ended.</b>"));
	// 	else {
	// 		char *msg = g_markup_printf_escaped("<span color=\"red\"><b>%s</b></span>", error_msg);
	// 		gtk_label_set_markup(GTK_LABEL(status), msg);
	// 		g_free(msg);
	// 	}
	// 
	// 	linphone_in_call_set_animation_image(callview, linphone_get_ui_config("stop_call_icon_name", "linphone-stop-call"));
	// 	linphone_in_call_view_hide_encryption(call);
	// 
	// 	gtk_widget_hide(linphone_get_widget(callview, "answer_decline_panel"));
	// 	gtk_widget_hide(linphone_get_widget(callview, "record_hbox"));
	// 	gtk_widget_hide(linphone_get_widget(callview, "buttons_panel"));
	// 	gtk_widget_hide(linphone_get_widget(callview, "incall_audioview"));
	// 	gtk_widget_hide(linphone_get_widget(callview, "quality_indicator"));
	// 	linphone_enable_mute_button(
	// 		GTK_BUTTON(linphone_get_widget(callview, "incall_mute")), FALSE);
	// 	linphone_enable_hold_button(call, FALSE, TRUE);

	// 	if (taskid != 0) g_source_remove(taskid);
	// 	g_timeout_add_seconds(2, (GSourceFunc)in_call_view_terminated, call);
	// 	if (in_conf)
	// 		linphone_terminate_conference_participant(call);
}
void linphone_call_update_tab_header(LinphoneCall *call, bool pause) {
	//	GtkWidget *w = (GtkWidget*)linphone_call_get_user_pointer(call);
	// 	GtkWidget *main_window = linphone_get_main_window();
	// 	GtkNotebook *notebook = GTK_NOTEBOOK(linphone_get_widget(main_window, "viewswitch"));
	// 	gint call_index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "call_index"));
	// 	GtkWidget *new_label = gtk_hbox_new(FALSE, 0);
	// 	GtkWidget *i = NULL;
	// 	GtkWidget *l;
	// 	gchar *text;
	// 
	// 	if (pause) {
	// 		i = gtk_image_new_from_icon_name("linphone-hold-off", GTK_ICON_SIZE_BUTTON);
	// 	}
	// 	else {
	// 		i = gtk_image_new_from_icon_name("linphone-start-call", GTK_ICON_SIZE_BUTTON);
	// 	}
	// 
	// 	text = g_strdup_printf(_("Call #%i"), call_index);
	// 	l = gtk_label_new(text);
	// 	gtk_box_pack_start(GTK_BOX(new_label), i, FALSE, FALSE, 0);
	// 	gtk_box_pack_end(GTK_BOX(new_label), l, TRUE, TRUE, 0);
	// 
	// 	gtk_notebook_set_tab_label(notebook, w, new_label);
	// 	gtk_widget_show_all(new_label);
	// 	g_free(text);
}


void _linphone_enable_video(bool val = FALSE) {
	LinphoneVideoPolicy policy = { 0 };
	policy.automatically_initiate = policy.automatically_accept = val;
	linphone_core_enable_video_capture(linphone_get_core(), TRUE);
	linphone_core_enable_video_display(linphone_get_core(), TRUE);
	linphone_core_set_video_policy(linphone_get_core(), &policy);
}

//���� �ص�����
static void linphone_call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cs, const char *msg) {
	LinphoneCallParams *params = nullptr;
	CString strMsg;
	switch (cs) {
	case LinphoneCallOutgoingInit:
		linphone_create_in_call_view(call);
		break;
	case LinphoneCallOutgoingProgress:
		linphone_in_call_view_set_calling(call);

		strMsg.Format("%d#%s#%d#",  PRO_USER_STATUS, gstrUserID, USER_BUSY);
		sendMSGToWnd(strMsg);
		break;
	case LinphoneCallStreamsRunning: //ͨ��������
		linphone_in_call_view_set_in_call(call);
		
		strMsg.Format("%d#%s#%d#", PRO_USER_STATUS, gstrUserID, USER_BUSY);
		sendMSGToWnd(strMsg);
		break;
	case LinphoneCallUpdatedByRemote:
		linphone_call_updated_by_remote(call);
		break;
	case LinphoneCallError:
		linphone_in_call_view_terminate(call, msg);
		CLogRecord::WriteRecordToFile("####ͨ��ʧ��####" + CString(msg));

		strMsg.Format("%d#%s#%d#",  PRO_USER_STATUS, gstrUserID, USER_CALL_ERR);
		sendMSGToWnd(strMsg);
		break;
	case LinphoneCallEnd:
		linphone_in_call_view_terminate(call, NULL);

		strMsg.Format("%d#%s#%d#",  PRO_USER_STATUS, gstrUserID, USER_CALL_OFF);
		sendMSGToWnd(strMsg);
		//linphone_status_icon_set_blinking(FALSE);
		break;
	case LinphoneCallIncomingReceived: //�Զ��ش�
									   //linphone_create_in_call_view(call);
									   //linphone_in_call_view_set_incoming(call);
									   //linphone_status_icon_set_blinking(TRUE);

		linphone_call_ref(call);
		params = linphone_core_create_call_params(lc, call);
		linphone_core_accept_call_with_params(lc, call, params);
		linphone_call_params_unref(params);

		strMsg.Format("%d#%s#%d#",PRO_USER_STATUS, gstrUserID, USER_BUSY);
		sendMSGToWnd(strMsg);
		break;
	case LinphoneCallResuming:
		//linphone_enable_hold_button(call, TRUE, TRUE);
		linphone_in_call_view_set_in_call(call);
		break;
	case LinphoneCallPausing:
		//linphone_enable_hold_button(call, TRUE, FALSE);
		linphone_call_update_tab_header(call, FALSE);
	case LinphoneCallPausedByRemote:
		//linphone_in_call_view_set_paused(call);
		linphone_call_update_tab_header(call, TRUE);

		strMsg.Format("%d#%s#%d#",  PRO_USER_STATUS, gstrUserID, USER_CALL_OFF);
		sendMSGToWnd(strMsg);
		break;
	case LinphoneCallConnected:
		//linphone_enable_hold_button(call, TRUE, TRUE);
		//linphone_status_icon_set_blinking(FALSE);
		break;
	default:
		break;
	}
	CLogRecord::WriteRecordToFile(strMsg);
	// 	linphone_notify(call, NULL, msg);
	// 	linphone_update_call_buttons(call);
}


//�ж�sip ��ַ �Ƿ��� ���������ϵͳ��
static bool userInServer(CString const& strAddr,int sta)
{
	CString strSip = strAddr;
	strSip = strSip.Mid(4, 4);
	CString strUserID;
	if (!gmapPhoneToUser.Lookup(strSip, strUserID))
	{
		strUserID.Format("map���Ҳ�����Ӧ���û�ID_[%s]", strAddr);
		CLogRecord::WriteRecordToFile(strUserID);
		return false;
	}
	//�û���ip��ַ  ���������ڵ�ϵͳ��   ���� �û�����
	if (-1 != strAddr.Find(gstrAddr) && !strUserID.IsEmpty()) 
	{
// 		int mapStatus;
// 		//�û�״̬ ���� ���� �ظ�
// 		if (gmapUserStatus.Lookup(strUserID, mapStatus) && mapStatus == sta)
// 		{
// 			return TRUE;
// 		}
// 		gmapUserStatus.SetAt(strUserID, sta);
		CString strMsg;
		strMsg.Format("%d#%s#%d#",  PRO_USER_STATUS, strUserID, sta);
		CLogRecord::WriteRecordToFile("�����û�״̬:"+strMsg);
		sendMSGToWnd(strMsg);
		return true;
	}
	return false;
}
//����֪ͨϵͳ
static void notify_presence_recv_updated(LinphoneCore *lc, LinphoneFriend *myfriend) {
	const LinphoneAddress* friend_address = linphone_friend_get_address(myfriend);

	if (friend_address != NULL) {
		char *str = linphone_address_as_string(friend_address);
		
		LinphoneOnlineStatus status = linphone_friend_get_status(myfriend);
		//status 0-����	  1-����   5-��æ ͨ����
		int sta = USER_ON;
		if (LinphoneStatusOffline == status) //����
		{
			sta = USER_OFF;
		}
		if (LinphoneStatusBusy == status
			|| LinphoneStatusOnThePhone == status)//��æ
		{
			sta = USER_BUSY;
		}
	
		userInServer(str, sta);
		ms_free(str);
	}
	
}
static void new_subscription_requested(LinphoneCore *lc, LinphoneFriend *myfriend, const char* url) {
	const LinphoneAddress* friend_address = linphone_friend_get_address(myfriend);
	if (friend_address != NULL) {
		char *str = linphone_address_as_string(friend_address);

		CString strOut;
		strOut.Format(" [%s] wants to see your status, accepting\n", str);
// 		OutputDebugString("requested------------");
// 		OutputDebugString(strOut);
// 		OutputDebugString("------------\n");
		CLogRecord::WriteRecordToFile(strOut);
		ms_free(str);
	}
	linphone_friend_edit(myfriend); /* start editing friend */
	linphone_friend_set_inc_subscribe_policy(myfriend, LinphoneSPAccept); /* Accept incoming subscription request for this friend*/
	linphone_friend_done(myfriend); /*commit change*/
	linphone_core_add_friend(lc, myfriend); /* add this new friend to the buddy list*/
}// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

//ͨ���� ״̬ �ص�����
static void	linphoneCallStatusUpdate(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallStats *stats)
{
	//����յ����շ�����Ķ���ʱ��
	float revJitter = linphone_call_stats_get_receiver_interarrival_jitter(stats);
	//�����߶�ʧ��
	float rcLossRate = linphone_call_stats_get_receiver_loss_rate(stats);
	//����͵ķ��ͷ�����Ķ���ʱ��
	float sendJitter = linphone_call_stats_get_sender_interarrival_jitter(stats);
	//���ͷ��Ķ�ʧ��
	float sendLossRate = linphone_call_stats_get_sender_loss_rate(stats);

	// TODO: �ڴ���ӿؼ�֪ͨ����������
	float volumeSpeaker = linphone_call_get_play_volume(g_newCall);
	float volumeMic = linphone_call_get_record_volume(g_newCall);

	float volume_db = volumeMic;
	float frac = (volume_db - UNSIGNIFICANT_VOLUME) / (float)(-UNSIGNIFICANT_VOLUME - 3.0);
	if (frac < 0) frac = 0;
	if (frac > 1.0) frac = 1.0;
	if (frac < gLastValue) {
		frac = (frac*SMOOTH) + (gLastValue*(1 - SMOOTH));
	}
	gLastValue = frac;
	CString str;
	float quality = linphone_call_get_current_quality(g_newCall);

	str.Format("ͨ����:���ܷ�����ʱ��[%f],���ܷ���ʧ��[%f],���ͷ�����ʱ��[%f],���ͷ���ʧ��[%f]\,ͨ������[%f],ͨ������[%f]"
		, revJitter , rcLossRate ,gLastValue, sendJitter, sendLossRate,quality);
	//GetDlgItem(IDC_EDIT4)->SetWindowText(str);
	CLogRecord::WriteRecordToFile(str);
}


 //buf ����:�����ʶ ����"G722" 
 //enable : TRUE ���� FALSE ����
 //clock_ :-1Ĭ�ϲ�ʹ��  ���������ʱ ��ʾ Ƶ��
static int setAudioCode(char* buf, bool enable, int clock_ = -1)
{
	bctbx_list_t* codeclist, *elem;
	codeclist = bctbx_list_copy(linphone_core_get_audio_codecs(linphone_get_core()));
	struct _OrtpPayloadType *pt;
	if (-1 == clock_)
	{
		for (elem = codeclist; elem != NULL; elem = elem->next) {

			pt = (struct _OrtpPayloadType *)elem->data;

			if (pt != nullptr && 0 == strcmp(pt->mime_type, buf)) //�ҵ���Ӧ���͵ı���
			{
				return linphone_core_enable_payload_type(linphone_get_core()
					, pt, enable);
			}

		}
	}
	else
	{
		for (elem = codeclist; elem != NULL; elem = elem->next) {

			pt = (struct _OrtpPayloadType *)elem->data;

			if (pt != nullptr && 0 == strcmp(pt->mime_type, buf) && clock_ == pt->clock_rate) //�ҵ���Ӧ���͵ı���
			{
				return linphone_core_enable_payload_type(linphone_get_core()
					, pt, enable);
			}

		}
	}

	return -1;
}

//����Ĭ�ϱ���
static int setDefaultCode()
{
	//return 0;
	int ret = 0;
	ret += setAudioCode("G722", TRUE);
	ret += setAudioCode("speex", FALSE, 16000);

	ret += setAudioCode("opus", FALSE);
	ret += setAudioCode("PCMU", FALSE);
	ret += setAudioCode("PCMA", FALSE);
	return ret;
}

//�û�ע��
static void UserNew(CString strID)
{
	CString strLog;
	strLog.Format("[register]�����û�ע��[%s]!", strID);
	CLogRecord::WriteRecordToFile(strLog);

	std::vector<std::vector<std::string> > data;

	std::string sqlstr = "SELECT * FROM usernext;";

	if (!CMyDataBase::GetInstance()->Select(sqlstr, data))
	{
		CString strErr = CMyDataBase::GetInstance()->GetErrorInfo();
		strErr.Format("ע��ʧ��,���ݿ��ѯʧ��:%s!", strErr);
		CLogRecord::WriteRecordToFile(strErr);
		sendErrInfo(strErr);
		return;
	}
		CString strNext;
		CString strSql, strSqlNext;
		if (data.size() > 0 && data[0].size() > 0) //������
		{
			 strNext = data[0][0].c_str();
		}
		if (strNext.IsEmpty()) //û���û�
		{
			strNext = "1000";
			strSqlNext.Format("insert into usernext values('%s');",strNext);	
		}
		
	
		strSql.Format("insert into usertable values('%s','%s');", strID, strNext);
		//ע��ʧ��
		if (!CMyDataBase::GetInstance()->Query(strSql.GetBuffer()))
		{
			strLog.Format("д��usertableʧ��,�û�[%s]ע��ʧ��!", strID);
			CLogRecord::WriteRecordToFile(strLog);
			return sendErrInfo(strLog);
		}
		else //ע��ɹ�
		{
			CString strPhone = strNext;
			strNext.Format("%d", atoi(strNext) + 1);
			//1.�޸�Զ�����ݿ��
			if (strSqlNext.IsEmpty())
			{
				strSqlNext.Format("update  usernext set userNext='%s' where userNext='%s';"
					, strNext, strPhone);
			}
			if (CMyDataBase::GetInstance()->Query(strSqlNext.GetBuffer())) //�޸ĳɹ�
			{
				//2.�޸��ڴ� �û���
				gmapPhoneToUser.SetAt(strPhone, strID);
				gmapUserTOPhone.SetAt(strID, strPhone);
				strLog.Format("д��usertable�ɹ�,�û�[%s]ע��ɹ�!", strID);
				CLogRecord::WriteRecordToFile(strLog);
			}
			else //�޸�ʧ��
			{
				//ע��ʧ��,ɾ��usertable ��Ϣ
				strSql.Format("delete from usernext where userID ='%s';", strID);
				CMyDataBase::GetInstance()->Query(strSql.GetBuffer());
				strLog.Format("�޸�usernextʧ��,�ع�usertable ע����Ϣ,�û�[%s]ע��ʧ��!", strID);
				CLogRecord::WriteRecordToFile(strLog);
				return sendErrInfo(strLog);
			}
		}
	
}

//�û���½
//strID �û�Ψһ��ʶ
static void UserLogin(CString strID)
{
	CString strPhone;
	CString strLog;

	strLog.Format("[login]�û�[%s]���ڵ�½!", strID);
	CLogRecord::WriteRecordToFile(strLog);
	
	if (!gmapUserTOPhone.Lookup(strID, strPhone))
	{
		strLog.Format("�û�%s������,����ˢ���û��б�!", strID);
		CLogRecord::WriteRecordToFile(strLog);
		InitUserMap();
		//ˢ�º���û���û�������ע��
		if (!gmapUserTOPhone.Lookup(strID, strPhone))
		{
			strLog.Format("�û�����%s������,����ע�����û�!", strID);
			CLogRecord::WriteRecordToFile(strLog);
			UserNew(strID);
		}
	}
	//�ٴ�Ѱ���û�ע����Ϣ
	if (gmapUserTOPhone.Lookup(strID, strPhone))
	{
		//1.��ʼ������
		InitUserFriend();


		//2.��½
		CString strAddr;
		strAddr.Format("sip:%s@%s:%s", strPhone, gstrAddr, gstrAddrPort);
		CString strProxy;
		strProxy.Format("%s:%s", gstrAddr, gstrAddrPort);

		//��¼��ǰ��½���û�
		gstrUserID = strID;

		linphone_proxy_ok(linphone_get_core(), strAddr.GetBuffer(), strProxy.GetBuffer());

		
	}
	else
	{
		strLog.Format("�û�ע��ʧ��,������û�����:[%s]-[%s]", strID, strPhone);
		CLogRecord::WriteRecordToFile(strLog);
	}
}

//�û� ����
//callID �Է�ID
//callIp  ����ip
static void UserCall(CString callID,CString callIp)
{
	CString strLog;
	strLog.Format("[call]���ں���[%s]!", callID);
	CLogRecord::WriteRecordToFile(strLog);

	CString strPhone;

	if (!gmapUserTOPhone.Lookup(callID, strPhone) || strPhone.IsEmpty())
	{
		strLog.Format("�û�%s������,����ˢ���û��б�!", callID);
		CLogRecord::WriteRecordToFile(strLog);
		InitUserMap();
		//ˢ�º���û���û�������ע��
	}
	if (gmapUserTOPhone.Lookup(callID, strPhone))
	{
		CString strID;
		strID.Format("sip:%s@%s:%s", strPhone, gstrAddr, gstrAddrPort);
		linphone_start_call_do(strID);
	}
	else
	{
		strLog.Format("�û�[%s]������", callID);
		CLogRecord::WriteRecordToFile(strLog);
		sendErrInfo(strLog);
	}
}
//�������ڽ��е�ͨ��
static void UserCallEnd()
{
	CString strLog;
	strLog.Format("[call_end]����ͨ��[1]!");
	CLogRecord::WriteRecordToFile(strLog);
	gVoiceChatDlg->OnCallEnd();
}

//���ݿ��ʼ������
//��ʼ���û� map
static void InitUserMap()
{
	// ��ȡ����

	std::vector<std::vector<std::string> > data;

	std::string sqlstr = "SELECT * FROM usertable;";

	if (!CMyDataBase::GetInstance()->Select(sqlstr, data))
	{
		CString strErr = CMyDataBase::GetInstance()->GetErrorInfo();
		strErr.Format("���ݿ��ѯʧ��:%s!", strErr);
		CLogRecord::WriteRecordToFile(strErr);
		sendErrInfo(strErr);
		return;
	}

	for (unsigned int i = 0; i < data.size(); ++i)
	{
		if (data[0].size() >= 2) //������Ч ��������
		{
			//userID -> userPhone
			gmapUserTOPhone.SetAt(data[i][0].c_str(), data[i][1].c_str());

			//userPhone ->  userID
			gmapPhoneToUser.SetAt(data[i][1].c_str(), data[i][0].c_str());
		}

	}
	
}

//��ʼ�������� ������ 
static void InitUserFriend()
{
	if (USERTYPE_1 == gUSERTYP) //�������û�
	{
		POSITION pos = gmapUserTOPhone.GetStartPosition();
		CString strKey, strValue, strID;
		while (pos)
		{
			gmapUserTOPhone.GetNextAssoc(pos, strKey, strValue);


			strID.Format("%s@%s:%s", strValue, gstrAddr, gstrAddrPort);
			LinphoneFriend *lf = NULL;
			LinphoneFriend *lf2;
			bool show_presence = FALSE, allow_presence = FALSE;
			const char *name, *uri;
			LinphoneAddress* friend_address;
			if (lf == NULL) {
				lf = linphone_core_create_friend(linphone_get_core());
				// 		if (linphone_get_ui_config_int("use_subscribe_notify", 1) == 1) {
				// 			show_presence = FALSE;
				// 			allow_presence = FALSE;
				// 		}
				linphone_friend_set_inc_subscribe_policy(lf, LinphoneSPAccept);
				linphone_friend_send_subscribe(lf, TRUE);
			}

			uri = strID;
			friend_address = linphone_core_interpret_url(linphone_get_core(), uri);
			if (friend_address == NULL) {

				return;
			}

			linphone_friend_set_address(lf, friend_address);
			linphone_friend_set_name(lf, strValue);
			linphone_friend_send_subscribe(lf, TRUE);
			linphone_friend_enable_subscribes(lf, TRUE);
			linphone_friend_set_inc_subscribe_policy(lf, LinphoneSPAccept);
			if (linphone_friend_in_list(lf)) {
				linphone_friend_done(lf);
			}
			else {
				char *uri = linphone_address_as_string_uri_only(friend_address);
				lf2 = linphone_core_get_friend_by_address(linphone_get_core(), uri);
				ms_free(uri);
				if (lf2 == NULL) {
					linphone_core_add_friend(linphone_get_core(), lf);
				}
			}
			linphone_address_unref(friend_address);

			LinphoneOnlineStatus status;
			if (lf2)
			{
				status = linphone_friend_get_status(lf2);
			}
			else
				status = linphone_friend_get_status(lf);
			
			//status 0-����	  1-����   5-��æ ͨ����
			int sta = USER_ON;
			if (LinphoneStatusOffline == status) //����
			{
				sta = USER_OFF;
			}
			if (LinphoneStatusBusy == status
				|| LinphoneStatusOnThePhone == status)//��æ
			{
				sta = USER_BUSY;
			}
			int mapStatus;
			//�û�״̬ ���� ���� �ظ�
			if (gmapUserStatus.Lookup(strKey, mapStatus) && mapStatus == sta)
			{
				continue;
			}
			gmapUserStatus.SetAt(strKey, sta);

			CString strMsg;
			strMsg.Format("%d#%s#%d#", PRO_USER_STATUS, strKey, sta);
			CLogRecord::WriteRecordToFile("�����û�״̬:" + strMsg);
			sendMSGToWnd(strMsg);

		}


	}
}


std::string UTF8_To_GBK(const std::string &source)
{
	enum { GB2312 = 936 };

	unsigned long len = ::MultiByteToWideChar(CP_UTF8, NULL, source.c_str(), -1, NULL, NULL);
	if (len == 0)
		return std::string();
	wchar_t *wide_char_buffer = new wchar_t[len];
	::MultiByteToWideChar(CP_UTF8, NULL, source.c_str(), -1, wide_char_buffer, len);

	len = ::WideCharToMultiByte(GB2312, NULL, wide_char_buffer, -1, NULL, NULL, NULL, NULL);
	if (len == 0)
	{
		delete[] wide_char_buffer;
		return std::string();
	}
	char *multi_byte_buffer = new char[len];
	::WideCharToMultiByte(GB2312, NULL, wide_char_buffer, -1, multi_byte_buffer, len, NULL, NULL);

	std::string dest(multi_byte_buffer);
	delete[] wide_char_buffer;
	delete[] multi_byte_buffer;
	return dest;
}
std::string GBK_To_UTF8(const std::string &source)
{
	enum { GB2312 = 936 };

	unsigned long len = ::MultiByteToWideChar(GB2312, NULL, source.c_str(), -1, NULL, NULL);
	if (len == 0)
		return std::string();
	wchar_t *wide_char_buffer = new wchar_t[len];
	::MultiByteToWideChar(GB2312, NULL, source.c_str(), -1, wide_char_buffer, len);

	len = ::WideCharToMultiByte(CP_UTF8, NULL, wide_char_buffer, -1, NULL, NULL, NULL, NULL);
	if (len == 0)
	{
		delete[] wide_char_buffer;
		return std::string();
	}
	char *multi_byte_buffer = new char[len];
	::WideCharToMultiByte(CP_UTF8, NULL, wide_char_buffer, -1, multi_byte_buffer, len, NULL, NULL);

	std::string dest(multi_byte_buffer);
	delete[] wide_char_buffer;
	delete[] multi_byte_buffer;
	return dest;
}
//��ʼ�� �����豸
static void InitAudioDevice() 
{
	if (g_lc == nullptr)
	{
		return;
	}
	//���� ���� ���� ����豸
	const char **sound_devices = linphone_core_get_sound_devices(g_lc);

	//����ѡ��� �ɼ� �豸 �� �����豸 
	const char* capture = linphone_core_get_capture_device(g_lc);
	const char* playback = linphone_core_get_playback_device(g_lc);
	CString strMSG;
	deviceStatus ds; 
	for (;*sound_devices != nullptr; ++sound_devices)
	{
		CString strDevice = UTF8_To_GBK(*sound_devices).c_str();
		if (linphone_core_sound_device_can_capture(linphone_get_core(), *sound_devices))
		{//�ɼ��豸 
			strMSG.Format("%d#%d#%s#%d#", PRO_AUDIO_DEVICE, DEVICE_CAPTURE, strDevice
				, strcmp(*sound_devices, capture) == 0 ? 1 : 0);
			sendMSGToWnd(strMSG);
			ds.bUsing = strcmp(*sound_devices, capture) == 0 ? 1 : 0;
			ds.iType = DEVICE_CAPTURE;
			ds.strDeviceID = strDevice;
			gmapDeviceStatus.SetAt(strDevice, ds);
		}
		if (linphone_core_sound_device_can_playback(linphone_get_core(), *sound_devices))
		{//�����豸
			strMSG.Format("%d#%d#%s#%d#", PRO_AUDIO_DEVICE, DEVICE_PLAY, strDevice
				, strcmp(*sound_devices, playback) == 0 ? 1 : 0);
			sendMSGToWnd(strMSG);
			ds.bUsing = strcmp(*sound_devices, playback) == 0 ? 1 : 0;
			ds.iType = DEVICE_PLAY;
			ds.strDeviceID = strDevice;
			gmapDeviceStatus.SetAt(strDevice, ds);
		}
	}
}

//�����豸 ѡ��

static void UserDeviceChoose(CString const& strType, CString const & strDevicID)
{
	deviceStatus ds;
	const char* deviceID = GBK_To_UTF8(strDevicID.GetString()).c_str();
	if (gmapDeviceStatus.Lookup(strDevicID,ds))
	{
		if (atoi(strType) == ds.iType)
		{
			ds.bUsing = TRUE;
			if (ds.iType == DEVICE_CAPTURE) //�ɼ��豸
			{
				linphone_core_set_capture_device(linphone_get_core(), deviceID);
			}
			if (ds.iType == DEVICE_PLAY) //�����豸
			{
				linphone_core_set_playback_device(linphone_get_core(), deviceID);
			}
			
		}
	}
	
}

// class CAboutDlg : public CDialogEx
// {
// public:
// 	CAboutDlg();
// 
// // �Ի�������
// #ifdef AFX_DESIGN_TIME
// 	enum { IDD = IDD_ABOUTBOX };
// #endif
// 
// 	protected:
// 	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
// 
// // ʵ��
// protected:
// 	DECLARE_MESSAGE_MAP()
// public:
// 	afx_msg void OnDestroy();
// };
// 
// CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
// {
// }
// 
// void CAboutDlg::DoDataExchange(CDataExchange* pDX)
// {
// 	CDialogEx::DoDataExchange(pDX);
// }
// 
// BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
// 	ON_WM_DESTROY()
// END_MESSAGE_MAP()
// 

// CVOICECHATDlg �Ի���



CVOICECHATDlg::CVOICECHATDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_VOICECHAT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CVOICECHATDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CVOICECHATDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CVOICECHATDlg::OnUserLogin)
	ON_BN_CLICKED(IDC_BUTTON2, &CVOICECHATDlg::OnAddFriend)
	ON_BN_CLICKED(IDC_BUTTON3, &CVOICECHATDlg::OnCallUser)
	ON_BN_CLICKED(IDC_BUTTON4, &CVOICECHATDlg::OnCallEnd)
	ON_WM_TIMER()
	ON_WM_COPYDATA()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CVOICECHATDlg ��Ϣ�������
static BOOL InitConfig(CString const strPath)
{
	char buffAddr[MAX_PATH] = { 0 };
	GetPrivateProfileString("ADDRIP", "ADDRIP", "-1"
		, buffAddr, MAX_PATH, strPath);
	gstrAddr = buffAddr;

	memset(buffAddr, 0, MAX_PATH);
	GetPrivateProfileString("ADDRPORT", "ADDRPORT", "-1"
		, buffAddr, MAX_PATH, strPath);
	gstrAddrPort = buffAddr;

	memset(buffAddr, 0, MAX_PATH);
	GetPrivateProfileString("WNDNAME", "WNDNAME", "-1"
		, buffAddr, MAX_PATH, strPath);
	CString strWndName = buffAddr;

	memset(buffAddr, 0, MAX_PATH);
	GetPrivateProfileString("WNDCLSNAME", "WNDCLSNAME", "-1"
		, buffAddr, MAX_PATH, strPath);
	CString strWndClsName = buffAddr;

	//�û�����
	memset(buffAddr, 0, MAX_PATH);
	GetPrivateProfileString("USERTYPE", "USERTYPE", "2"
		, buffAddr, MAX_PATH, strPath);
	gUSERTYP = atoi(buffAddr);

	if (gstrAddr == CString("1") || gstrAddrPort == CString("1")
		|| strWndName == CString("1") || strWndClsName == CString("1"))
	{
		CString strOut;
		strOut.Format("��ȡ�����ļ��еķ����ip=[%s],port=[%s],������[%s],��������[%s] ʧ��"
			, gstrAddr, gstrAddrPort,strWndName,strWndClsName);
		CLogRecord::WriteRecordToFile(strOut);
		MessageBox(NULL,strOut,"��ʾ",MB_OK);
		CLogRecord::WriteRecordToFile(strOut);
		return FALSE;
	}
	gstrWndClsName = strWndClsName;
	gstrWndName = strWndName;
	CLogRecord::WriteRecordToFile("��ʼ���������!");
}

BOOL CVOICECHATDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	gHwnd = m_hWnd;

	if (gstrGUI != CString("1"))
	{
		WINDOWPLACEMENT wp;
		wp.length = sizeof(WINDOWPLACEMENT);
		wp.flags = WPF_RESTORETOMAXIMIZED;
		wp.showCmd = SW_HIDE;
		SetWindowPlacement(&wp);
	}
	
	gVoiceChatDlg = this;
	//��ʼ������
	InitConfig(gstrPath);

	//���ݿ����ӳ�ʼ��  ����3��
	static int threeTime = 3;
	while (!CMyDataBase::GetInstance()->InitMyDataBase(gstrPath) && threeTime-- > 0)
	{
		CString strErr = CMyDataBase::GetInstance()->GetErrorInfo();
		strErr.Format("���ݿ��ʧ��:%s,��������!", strErr);
		CLogRecord::WriteRecordToFile(strErr);
		sendErrInfo(strErr);
		Sleep(3000);
	}

	//��ʼ���û�map
	InitUserMap();

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	//��ʼ��
	{
		LinphoneCoreVTable vtable = { 0 };
		LinphoneCore *lc;
		LinphoneCall *call = NULL;
		const char *dest = NULL;


		vtable.registration_state_changed = linphone_registration_state_changed; //ע����Ϣ״̬
		vtable.call_state_changed = linphone_call_state_changed;
		vtable.auth_info_requested = linphone_auth_info_requested; //�����֤

		vtable.notify_presence_received = notify_presence_recv_updated;//���� ״̬֪ͨ
		vtable.new_subscription_requested = new_subscription_requested;

		vtable.call_stats_updated = linphoneCallStatusUpdate;//ͨ�� ״̬ �ص�

		CString strConfigPath = CLogRecord::GetAppPath() + "\\config\\voiceConfig.ini";
		lc = linphone_core_new(&vtable, NULL, NULL, NULL);
		g_lc = lc;
		the_core = lc;

		//���� ��Ƶ
		_linphone_enable_video();

		//srtp ����Э��
		linphone_core_set_media_encryption(lc, LinphoneMediaEncryptionSRTP);

		//�������tcp udp �˿�
		LinphoneSipTransports tp;
		linphone_core_get_sip_transports(lc, &tp);
		tp.udp_port = -1;
		tp.tcp_port = -1;
		linphone_core_set_sip_transports(lc, &tp);

		//�������� ���ݿ�
		linphone_core_set_friends_database_path(lc, "friend.db");


		//�������� ���
		if (!linphone_core_keep_alive_enabled(lc))
		{
			OutputDebugString("no-----\n");
		}
		//������������
		setDefaultCode();


		//�������� �豸��Ϣ
		InitAudioDevice();

		SetTimer(80, 30, 0);
		//SetTimer(90, 1000, 0);
		SetTimer(100, 10000, 0); //5 ��ˢ�������ѱ�
	}

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CVOICECHATDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
	
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�
void CVOICECHATDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CVOICECHATDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


//��½
void CVOICECHATDlg::OnUserLogin()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CString strID;
	GetDlgItem(IDC_EDIT1)->GetWindowText(strID);
	CString strAddr;
	strAddr.Format("sip:%s@%s:%s", strID, gstrAddr,gstrAddrPort);
	CString strProxy;
	strProxy.Format("%s:%s",gstrAddr,gstrAddrPort) ;
	linphone_proxy_ok(g_lc, strAddr.GetBuffer(), strProxy.GetBuffer());
}

//�������
void CVOICECHATDlg::OnAddFriend()
{
	CWnd* edit = GetDlgItem(IDC_EDIT2);
	CString strID, strName;
	edit->GetWindowText(strName);
	strID.Format("%s@%s:%s", strName, gstrAddr,gstrAddrPort);
	LinphoneFriend *lf = NULL;
	LinphoneFriend *lf2;
	bool show_presence = FALSE, allow_presence = FALSE;
	const char *name, *uri;
	LinphoneAddress* friend_address;
	if (lf == NULL) {
		lf = linphone_core_create_friend(g_lc);
		// 		if (linphone_get_ui_config_int("use_subscribe_notify", 1) == 1) {
		// 			show_presence = FALSE;
		// 			allow_presence = FALSE;
		// 		}
		linphone_friend_set_inc_subscribe_policy(lf, LinphoneSPAccept);
		linphone_friend_send_subscribe(lf, TRUE);
	}

	uri = strID;
	friend_address = linphone_core_interpret_url(linphone_get_core(), uri);
	if (friend_address == NULL) {

		return;
	}

	linphone_friend_set_address(lf, friend_address);
	linphone_friend_set_name(lf, strName);
	linphone_friend_send_subscribe(lf, TRUE);
	linphone_friend_set_inc_subscribe_policy(lf, LinphoneSPAccept);
	if (linphone_friend_in_list(lf)) {
		linphone_friend_done(lf);
	}
	else {
		char *uri = linphone_address_as_string_uri_only(friend_address);
		lf2 = linphone_core_get_friend_by_address(linphone_get_core(), uri);
		ms_free(uri);
		if (lf2 == NULL) {
			linphone_core_add_friend(linphone_get_core(), lf);
		}
	}
	linphone_address_unref(friend_address);
}

//call
void CVOICECHATDlg::OnCallUser()
{
	CString strID;
	GetDlgItem(IDC_EDIT3)->GetWindowText(strID);
	strID.Format("sip:%s@%s:%s", strID, gstrAddr,gstrAddrPort);
	linphone_start_call_do(strID);
}

//����
void CVOICECHATDlg::OnCallEnd()
{
	if (g_newCall)
	{
		linphone_core_terminate_call(linphone_get_core(), g_newCall);
		g_newCall = nullptr;
	}
}

void CVOICECHATDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	if (80 == nIDEvent)
	{
		linphone_core_iterate(g_lc);
	}
	if (90 == nIDEvent && g_newCall != nullptr)
	{
		if (g_newCall == nullptr)
		{
			return;
		}
	
	}
	if (100 == nIDEvent)
	{
		InitUserMap();
		//��ʼ�� ���ѱ�
		InitUserFriend();
	}
	CDialogEx::OnTimer(nIDEvent);
}

void StringSplit(CString source, CStringArray& dest, char division)
{
	if (source.IsEmpty())
	{

	}
	else
	{
		int pos = source.Find(division);
		if (pos == -1)
		{
			dest.Add(source);
		}
		else
		{
			dest.Add(source.Left(pos));
			source = source.Mid(pos + 1);
			StringSplit(source, dest, division);
		}
	}
}

//���� �ӿ�
BOOL CVOICECHATDlg::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	if (pCopyDataStruct->dwData == 1) //�ַ���
	{
		const char ch = '#';
		CString strRecv = (LPCTSTR)pCopyDataStruct->lpData;
		if (-1 != strRecv.Find(ch)) //��ʾ ������
		{
			CString strErr, strLog;
			CStringArray ary;
			StringSplit(strRecv, ary, ch);
			//����Э��
			if (ary.GetCount() >= 2) //���ڵ��� 2��������
			{
				switch (atoi(ary.GetAt(0))) //Э���
				{
				case PRO_LOGING: //��½
					UserLogin(ary.GetAt(1));
					
					break;
				case PRO_CALL://����
					
					if (ary.GetCount() >=3)
					{
						UserCall(ary.GetAt(1), ary.GetAt(2));
					}
					else
					{
						strErr.Format("[call]Э�����:%s", strRecv);
						sendErrInfo(strErr);
						CLogRecord::WriteRecordToFile(strErr);
					}
					break;
				case PRO_NEW_USER://ע��
				
					UserNew(ary.GetAt(1));
					break;
				case PRO_CALL_END: //����ͨ��
					
					UserCallEnd();
					break;
				case  PRO_AUDIO_CHOOSE://ѡ�� ���� �豸
					if (ary.GetCount() >= 3)
					{
						UserDeviceChoose(ary.GetAt(1), ary.GetAt(2));
					}
					else
					{
						strErr.Format("[device_choose]Э�����:%s", strRecv);
						sendErrInfo(strErr);
						CLogRecord::WriteRecordToFile(strErr);
					}
					break;
				case PRO_EXIT: //����˳�
					DestroyWindow();
					break;
				default:
					break;
				}
			}
			else
			{
				strErr.Format("[ALL]Э�����:%s", strRecv);
				sendErrInfo(strErr);
				CLogRecord::WriteRecordToFile(strErr);
			}
		}
	}
	return CDialogEx::OnCopyData(pWnd, pCopyDataStruct);
}


void CVOICECHATDlg::OnDestroy()
{
	if (g_lc)
	{
		KillTimer(80);
		KillTimer(100);

		linphone_core_destroy(g_lc);
	}
	CDialogEx::OnDestroy();
	// TODO: �ڴ˴������Ϣ����������
}
