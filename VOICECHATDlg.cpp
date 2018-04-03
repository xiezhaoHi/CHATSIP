
// VOICECHATDlg.cpp : 实现文件
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
static float gVolum = 1; //音量 [0-1]
static CString gstrAddr = "192.168.1.128";
static CString gstrAddrPort = "5060";
static CString gstrWndClsName; // 窗口类名
static CString gstrWndName; //窗口名
static HWND  gHwnd = nullptr; //本窗口句柄
static CString gstrUserID; //登陆用户唯一标识
static CString gstrCallUserID; //通话的用户ID
static CMap<CString, LPCTSTR, CString, LPCTSTR>	 gmapUserTOPhone; //用户ID 对应 号码
static CMap<CString, LPCTSTR, CString, LPCTSTR>  gmapPhoneToUser; //号码 对应 用户ID
static CMap<CString, LPCTSTR, int, int> gmapUserStatus;//用户对应的状态
typedef struct deviceStatus //语音设备状态
{
	CString strDeviceID; //设备ID
	int		iType; //设备类型 
	bool	bUsing; //使用状况 0 1
}deviceStatus;
static CMap<CString, LPCTSTR, deviceStatus, deviceStatus> gmapDeviceStatus; //设备状态
static int gUSERTYP =USERTYPE_2; //用户类型   默认是 普通用户
#define UNSIGNIFICANT_VOLUME (-23)
#define SMOOTH 0.15f
float gLastValue = 1.0;
static CVOICECHATDlg* gVoiceChatDlg = nullptr;

//声明
static void InitUserMap(); //填充 用户 与 号码的map
static void InitUserFriend(); //初始化 用户朋友
static void InitAudioDevice(); //初始化 语音设备
//向对接窗口发送消息
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

//添加 朋友消息通知

//发送错误消息
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

//参数 用户strAddr  sip:1000@192.168.1.5:15060  strProxy sip:192.168.1.5:15060
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

//sip 注册 回调函数
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
		//AfxMessageBox("注册失败");
		
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

	gLastValue = 1.0; //初始化
	g_newCall = call;
	float speakerGain = linphone_call_get_speaker_volume_gain(call);
	float playGain = linphone_call_get_microphone_volume_gain(call);
	int err = GetLastError();
	CString strLog;
	strLog.Format("语音通话,发送方音量增益[%f],接受方音量增益[%f]", speakerGain, playGain);
	CLogRecord::WriteRecordToFile(strLog);
	//设置音量
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

//呼叫 回调函数
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
	case LinphoneCallStreamsRunning: //通话进行中
		linphone_in_call_view_set_in_call(call);
		
		strMsg.Format("%d#%s#%d#", PRO_USER_STATUS, gstrUserID, USER_BUSY);
		sendMSGToWnd(strMsg);
		break;
	case LinphoneCallUpdatedByRemote:
		linphone_call_updated_by_remote(call);
		break;
	case LinphoneCallError:
		linphone_in_call_view_terminate(call, msg);
		CLogRecord::WriteRecordToFile("####通话失败####" + CString(msg));

		strMsg.Format("%d#%s#%d#",  PRO_USER_STATUS, gstrUserID, USER_CALL_ERR);
		sendMSGToWnd(strMsg);
		break;
	case LinphoneCallEnd:
		linphone_in_call_view_terminate(call, NULL);

		strMsg.Format("%d#%s#%d#",  PRO_USER_STATUS, gstrUserID, USER_CALL_OFF);
		sendMSGToWnd(strMsg);
		//linphone_status_icon_set_blinking(FALSE);
		break;
	case LinphoneCallIncomingReceived: //自动回答
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


//判断sip 地址 是否是 在现在这个系统中
static bool userInServer(CString const& strAddr,int sta)
{
	CString strSip = strAddr;
	strSip = strSip.Mid(4, 4);
	CString strUserID;
	if (!gmapPhoneToUser.Lookup(strSip, strUserID))
	{
		strUserID.Format("map中找不到对应的用户ID_[%s]", strAddr);
		CLogRecord::WriteRecordToFile(strUserID);
		return false;
	}
	//用户的ip地址  存在于现在的系统中   并且 用户存在
	if (-1 != strAddr.Find(gstrAddr) && !strUserID.IsEmpty()) 
	{
// 		int mapStatus;
// 		//用户状态 存在 并且 重复
// 		if (gmapUserStatus.Lookup(strUserID, mapStatus) && mapStatus == sta)
// 		{
// 			return TRUE;
// 		}
// 		gmapUserStatus.SetAt(strUserID, sta);
		CString strMsg;
		strMsg.Format("%d#%s#%d#",  PRO_USER_STATUS, strUserID, sta);
		CLogRecord::WriteRecordToFile("发送用户状态:"+strMsg);
		sendMSGToWnd(strMsg);
		return true;
	}
	return false;
}
//朋友通知系统
static void notify_presence_recv_updated(LinphoneCore *lc, LinphoneFriend *myfriend) {
	const LinphoneAddress* friend_address = linphone_friend_get_address(myfriend);

	if (friend_address != NULL) {
		char *str = linphone_address_as_string(friend_address);
		
		LinphoneOnlineStatus status = linphone_friend_get_status(myfriend);
		//status 0-离线	  1-在线   5-繁忙 通话中
		int sta = USER_ON;
		if (LinphoneStatusOffline == status) //离线
		{
			sta = USER_OFF;
		}
		if (LinphoneStatusBusy == status
			|| LinphoneStatusOnThePhone == status)//繁忙
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
}// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

//通话中 状态 回调函数
static void	linphoneCallStatusUpdate(LinphoneCore *lc, LinphoneCall *call, const LinphoneCallStats *stats)
{
	//最后收到接收方报告的抖动时间
	float revJitter = linphone_call_stats_get_receiver_interarrival_jitter(stats);
	//接收者丢失率
	float rcLossRate = linphone_call_stats_get_receiver_loss_rate(stats);
	//最后发送的发送方报告的抖动时间
	float sendJitter = linphone_call_stats_get_sender_interarrival_jitter(stats);
	//发送方的丢失率
	float sendLossRate = linphone_call_stats_get_sender_loss_rate(stats);

	// TODO: 在此添加控件通知处理程序代码
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

	str.Format("通话中:接受方抖动时间[%f],接受方丢失率[%f],发送方抖动时间[%f],发送方丢失率[%f]\,通话音量[%f],通话质量[%f]"
		, revJitter , rcLossRate ,gLastValue, sendJitter, sendLossRate,quality);
	//GetDlgItem(IDC_EDIT4)->SetWindowText(str);
	CLogRecord::WriteRecordToFile(str);
}


 //buf 参数:编码标识 例如"G722" 
 //enable : TRUE 启用 FALSE 禁用
 //clock_ :-1默认不使用  多个编码规格时 表示 频率
static int setAudioCode(char* buf, bool enable, int clock_ = -1)
{
	bctbx_list_t* codeclist, *elem;
	codeclist = bctbx_list_copy(linphone_core_get_audio_codecs(linphone_get_core()));
	struct _OrtpPayloadType *pt;
	if (-1 == clock_)
	{
		for (elem = codeclist; elem != NULL; elem = elem->next) {

			pt = (struct _OrtpPayloadType *)elem->data;

			if (pt != nullptr && 0 == strcmp(pt->mime_type, buf)) //找到相应类型的编码
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

			if (pt != nullptr && 0 == strcmp(pt->mime_type, buf) && clock_ == pt->clock_rate) //找到相应类型的编码
			{
				return linphone_core_enable_payload_type(linphone_get_core()
					, pt, enable);
			}

		}
	}

	return -1;
}

//设置默认编码
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

//用户注册
static void UserNew(CString strID)
{
	CString strLog;
	strLog.Format("[register]正在用户注册[%s]!", strID);
	CLogRecord::WriteRecordToFile(strLog);

	std::vector<std::vector<std::string> > data;

	std::string sqlstr = "SELECT * FROM usernext;";

	if (!CMyDataBase::GetInstance()->Select(sqlstr, data))
	{
		CString strErr = CMyDataBase::GetInstance()->GetErrorInfo();
		strErr.Format("注册失败,数据库查询失败:%s!", strErr);
		CLogRecord::WriteRecordToFile(strErr);
		sendErrInfo(strErr);
		return;
	}
		CString strNext;
		CString strSql, strSqlNext;
		if (data.size() > 0 && data[0].size() > 0) //有数据
		{
			 strNext = data[0][0].c_str();
		}
		if (strNext.IsEmpty()) //没有用户
		{
			strNext = "1000";
			strSqlNext.Format("insert into usernext values('%s');",strNext);	
		}
		
	
		strSql.Format("insert into usertable values('%s','%s');", strID, strNext);
		//注册失败
		if (!CMyDataBase::GetInstance()->Query(strSql.GetBuffer()))
		{
			strLog.Format("写入usertable失败,用户[%s]注册失败!", strID);
			CLogRecord::WriteRecordToFile(strLog);
			return sendErrInfo(strLog);
		}
		else //注册成功
		{
			CString strPhone = strNext;
			strNext.Format("%d", atoi(strNext) + 1);
			//1.修改远程数据库表
			if (strSqlNext.IsEmpty())
			{
				strSqlNext.Format("update  usernext set userNext='%s' where userNext='%s';"
					, strNext, strPhone);
			}
			if (CMyDataBase::GetInstance()->Query(strSqlNext.GetBuffer())) //修改成功
			{
				//2.修改内存 用户表
				gmapPhoneToUser.SetAt(strPhone, strID);
				gmapUserTOPhone.SetAt(strID, strPhone);
				strLog.Format("写入usertable成功,用户[%s]注册成功!", strID);
				CLogRecord::WriteRecordToFile(strLog);
			}
			else //修改失败
			{
				//注册失败,删除usertable 信息
				strSql.Format("delete from usernext where userID ='%s';", strID);
				CMyDataBase::GetInstance()->Query(strSql.GetBuffer());
				strLog.Format("修改usernext失败,回滚usertable 注册信息,用户[%s]注册失败!", strID);
				CLogRecord::WriteRecordToFile(strLog);
				return sendErrInfo(strLog);
			}
		}
	
}

//用户登陆
//strID 用户唯一标识
static void UserLogin(CString strID)
{
	CString strPhone;
	CString strLog;

	strLog.Format("[login]用户[%s]正在登陆!", strID);
	CLogRecord::WriteRecordToFile(strLog);
	
	if (!gmapUserTOPhone.Lookup(strID, strPhone))
	{
		strLog.Format("用户%s不存在,尝试刷新用户列表!", strID);
		CLogRecord::WriteRecordToFile(strLog);
		InitUserMap();
		//刷新后还是没有用户，尝试注册
		if (!gmapUserTOPhone.Lookup(strID, strPhone))
		{
			strLog.Format("用户还是%s不存在,尝试注册新用户!", strID);
			CLogRecord::WriteRecordToFile(strLog);
			UserNew(strID);
		}
	}
	//再次寻找用户注册信息
	if (gmapUserTOPhone.Lookup(strID, strPhone))
	{
		//1.初始化朋友
		InitUserFriend();


		//2.登陆
		CString strAddr;
		strAddr.Format("sip:%s@%s:%s", strPhone, gstrAddr, gstrAddrPort);
		CString strProxy;
		strProxy.Format("%s:%s", gstrAddr, gstrAddrPort);

		//记录当前登陆的用户
		gstrUserID = strID;

		linphone_proxy_ok(linphone_get_core(), strAddr.GetBuffer(), strProxy.GetBuffer());

		
	}
	else
	{
		strLog.Format("用户注册失败,传入的用户错误:[%s]-[%s]", strID, strPhone);
		CLogRecord::WriteRecordToFile(strLog);
	}
}

//用户 呼叫
//callID 对方ID
//callIp  对我ip
static void UserCall(CString callID,CString callIp)
{
	CString strLog;
	strLog.Format("[call]正在呼叫[%s]!", callID);
	CLogRecord::WriteRecordToFile(strLog);

	CString strPhone;

	if (!gmapUserTOPhone.Lookup(callID, strPhone) || strPhone.IsEmpty())
	{
		strLog.Format("用户%s不存在,尝试刷新用户列表!", callID);
		CLogRecord::WriteRecordToFile(strLog);
		InitUserMap();
		//刷新后还是没有用户，尝试注册
	}
	if (gmapUserTOPhone.Lookup(callID, strPhone))
	{
		CString strID;
		strID.Format("sip:%s@%s:%s", strPhone, gstrAddr, gstrAddrPort);
		linphone_start_call_do(strID);
	}
	else
	{
		strLog.Format("用户[%s]不存在", callID);
		CLogRecord::WriteRecordToFile(strLog);
		sendErrInfo(strLog);
	}
}
//结束正在进行的通话
static void UserCallEnd()
{
	CString strLog;
	strLog.Format("[call_end]结束通话[1]!");
	CLogRecord::WriteRecordToFile(strLog);
	gVoiceChatDlg->OnCallEnd();
}

//数据库初始化操作
//初始化用户 map
static void InitUserMap()
{
	// 读取数据

	std::vector<std::vector<std::string> > data;

	std::string sqlstr = "SELECT * FROM usertable;";

	if (!CMyDataBase::GetInstance()->Select(sqlstr, data))
	{
		CString strErr = CMyDataBase::GetInstance()->GetErrorInfo();
		strErr.Format("数据库查询失败:%s!", strErr);
		CLogRecord::WriteRecordToFile(strErr);
		sendErrInfo(strErr);
		return;
	}

	for (unsigned int i = 0; i < data.size(); ++i)
	{
		if (data[0].size() >= 2) //数据有效 两条以上
		{
			//userID -> userPhone
			gmapUserTOPhone.SetAt(data[i][0].c_str(), data[i][1].c_str());

			//userPhone ->  userID
			gmapPhoneToUser.SetAt(data[i][1].c_str(), data[i][0].c_str());
		}

	}
	
}

//初始化管理者 的朋友 
static void InitUserFriend()
{
	if (USERTYPE_1 == gUSERTYP) //管理者用户
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
			
			//status 0-离线	  1-在线   5-繁忙 通话中
			int sta = USER_ON;
			if (LinphoneStatusOffline == status) //离线
			{
				sta = USER_OFF;
			}
			if (LinphoneStatusBusy == status
				|| LinphoneStatusOnThePhone == status)//繁忙
			{
				sta = USER_BUSY;
			}
			int mapStatus;
			//用户状态 存在 并且 重复
			if (gmapUserStatus.Lookup(strKey, mapStatus) && mapStatus == sta)
			{
				continue;
			}
			gmapUserStatus.SetAt(strKey, sta);

			CString strMsg;
			strMsg.Format("%d#%s#%d#", PRO_USER_STATUS, strKey, sta);
			CLogRecord::WriteRecordToFile("发送用户状态:" + strMsg);
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
//初始化 语音设备
static void InitAudioDevice() 
{
	if (g_lc == nullptr)
	{
		return;
	}
	//所以 语音 输入 输出设备
	const char **sound_devices = linphone_core_get_sound_devices(g_lc);

	//现在选择的 采集 设备 和 播放设备 
	const char* capture = linphone_core_get_capture_device(g_lc);
	const char* playback = linphone_core_get_playback_device(g_lc);
	CString strMSG;
	deviceStatus ds; 
	for (;*sound_devices != nullptr; ++sound_devices)
	{
		CString strDevice = UTF8_To_GBK(*sound_devices).c_str();
		if (linphone_core_sound_device_can_capture(linphone_get_core(), *sound_devices))
		{//采集设备 
			strMSG.Format("%d#%d#%s#%d#", PRO_AUDIO_DEVICE, DEVICE_CAPTURE, strDevice
				, strcmp(*sound_devices, capture) == 0 ? 1 : 0);
			sendMSGToWnd(strMSG);
			ds.bUsing = strcmp(*sound_devices, capture) == 0 ? 1 : 0;
			ds.iType = DEVICE_CAPTURE;
			ds.strDeviceID = strDevice;
			gmapDeviceStatus.SetAt(strDevice, ds);
		}
		if (linphone_core_sound_device_can_playback(linphone_get_core(), *sound_devices))
		{//播放设备
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

//语音设备 选择

static void UserDeviceChoose(CString const& strType, CString const & strDevicID)
{
	deviceStatus ds;
	const char* deviceID = GBK_To_UTF8(strDevicID.GetString()).c_str();
	if (gmapDeviceStatus.Lookup(strDevicID,ds))
	{
		if (atoi(strType) == ds.iType)
		{
			ds.bUsing = TRUE;
			if (ds.iType == DEVICE_CAPTURE) //采集设备
			{
				linphone_core_set_capture_device(linphone_get_core(), deviceID);
			}
			if (ds.iType == DEVICE_PLAY) //播放设备
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
// // 对话框数据
// #ifdef AFX_DESIGN_TIME
// 	enum { IDD = IDD_ABOUTBOX };
// #endif
// 
// 	protected:
// 	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
// 
// // 实现
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

// CVOICECHATDlg 对话框



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


// CVOICECHATDlg 消息处理程序
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

	//用户类型
	memset(buffAddr, 0, MAX_PATH);
	GetPrivateProfileString("USERTYPE", "USERTYPE", "2"
		, buffAddr, MAX_PATH, strPath);
	gUSERTYP = atoi(buffAddr);

	if (gstrAddr == CString("1") || gstrAddrPort == CString("1")
		|| strWndName == CString("1") || strWndClsName == CString("1"))
	{
		CString strOut;
		strOut.Format("获取配置文件中的服务端ip=[%s],port=[%s],窗口名[%s],窗口类名[%s] 失败"
			, gstrAddr, gstrAddrPort,strWndName,strWndClsName);
		CLogRecord::WriteRecordToFile(strOut);
		MessageBox(NULL,strOut,"提示",MB_OK);
		CLogRecord::WriteRecordToFile(strOut);
		return FALSE;
	}
	gstrWndClsName = strWndClsName;
	gstrWndName = strWndName;
	CLogRecord::WriteRecordToFile("初始化配置完成!");
}

BOOL CVOICECHATDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
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

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

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
	//初始化配置
	InitConfig(gstrPath);

	//数据库连接初始化  尝试3次
	static int threeTime = 3;
	while (!CMyDataBase::GetInstance()->InitMyDataBase(gstrPath) && threeTime-- > 0)
	{
		CString strErr = CMyDataBase::GetInstance()->GetErrorInfo();
		strErr.Format("数据库打开失败:%s,正在重试!", strErr);
		CLogRecord::WriteRecordToFile(strErr);
		sendErrInfo(strErr);
		Sleep(3000);
	}

	//初始化用户map
	InitUserMap();

	// TODO: 在此添加额外的初始化代码
	//初始化
	{
		LinphoneCoreVTable vtable = { 0 };
		LinphoneCore *lc;
		LinphoneCall *call = NULL;
		const char *dest = NULL;


		vtable.registration_state_changed = linphone_registration_state_changed; //注册消息状态
		vtable.call_state_changed = linphone_call_state_changed;
		vtable.auth_info_requested = linphone_auth_info_requested; //身份认证

		vtable.notify_presence_received = notify_presence_recv_updated;//朋友 状态通知
		vtable.new_subscription_requested = new_subscription_requested;

		vtable.call_stats_updated = linphoneCallStatusUpdate;//通话 状态 回调

		CString strConfigPath = CLogRecord::GetAppPath() + "\\config\\voiceConfig.ini";
		lc = linphone_core_new(&vtable, NULL, NULL, NULL);
		g_lc = lc;
		the_core = lc;

		//禁用 视频
		_linphone_enable_video();

		//srtp 编码协议
		linphone_core_set_media_encryption(lc, LinphoneMediaEncryptionSRTP);

		//设置随机tcp udp 端口
		LinphoneSipTransports tp;
		linphone_core_get_sip_transports(lc, &tp);
		tp.udp_port = -1;
		tp.tcp_port = -1;
		linphone_core_set_sip_transports(lc, &tp);

		//设置朋友 数据库
		linphone_core_set_friends_database_path(lc, "friend.db");


		//设置心跳 间隔
		if (!linphone_core_keep_alive_enabled(lc))
		{
			OutputDebugString("no-----\n");
		}
		//设置语音编码
		setDefaultCode();


		//返回语音 设备信息
		InitAudioDevice();

		SetTimer(80, 30, 0);
		//SetTimer(90, 1000, 0);
		SetTimer(100, 10000, 0); //5 秒刷新新朋友表
	}

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。
void CVOICECHATDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CVOICECHATDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


//登陆
void CVOICECHATDlg::OnUserLogin()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strID;
	GetDlgItem(IDC_EDIT1)->GetWindowText(strID);
	CString strAddr;
	strAddr.Format("sip:%s@%s:%s", strID, gstrAddr,gstrAddrPort);
	CString strProxy;
	strProxy.Format("%s:%s",gstrAddr,gstrAddrPort) ;
	linphone_proxy_ok(g_lc, strAddr.GetBuffer(), strProxy.GetBuffer());
}

//添加朋友
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

//结束
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
	// TODO: 在此添加消息处理程序代码和/或调用默认值
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
		//初始化 朋友表
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

//处理 接口
BOOL CVOICECHATDlg::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (pCopyDataStruct->dwData == 1) //字符串
	{
		const char ch = '#';
		CString strRecv = (LPCTSTR)pCopyDataStruct->lpData;
		if (-1 != strRecv.Find(ch)) //表示 有数据
		{
			CString strErr, strLog;
			CStringArray ary;
			StringSplit(strRecv, ary, ch);
			//解析协议
			if (ary.GetCount() >= 2) //大于等于 2才有意义
			{
				switch (atoi(ary.GetAt(0))) //协议号
				{
				case PRO_LOGING: //登陆
					UserLogin(ary.GetAt(1));
					
					break;
				case PRO_CALL://呼叫
					
					if (ary.GetCount() >=3)
					{
						UserCall(ary.GetAt(1), ary.GetAt(2));
					}
					else
					{
						strErr.Format("[call]协议错误:%s", strRecv);
						sendErrInfo(strErr);
						CLogRecord::WriteRecordToFile(strErr);
					}
					break;
				case PRO_NEW_USER://注册
				
					UserNew(ary.GetAt(1));
					break;
				case PRO_CALL_END: //结束通话
					
					UserCallEnd();
					break;
				case  PRO_AUDIO_CHOOSE://选择 语音 设备
					if (ary.GetCount() >= 3)
					{
						UserDeviceChoose(ary.GetAt(1), ary.GetAt(2));
					}
					else
					{
						strErr.Format("[device_choose]协议错误:%s", strRecv);
						sendErrInfo(strErr);
						CLogRecord::WriteRecordToFile(strErr);
					}
					break;
				case PRO_EXIT: //软件退出
					DestroyWindow();
					break;
				default:
					break;
				}
			}
			else
			{
				strErr.Format("[ALL]协议错误:%s", strRecv);
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
	// TODO: 在此处添加消息处理程序代码
}
