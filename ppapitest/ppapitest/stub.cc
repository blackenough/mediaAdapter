#include <stdint.h>
#include <stdlib.h>
#include <string.h>
//#include <Windows.h>
#include <tchar.h>
#include <AFXWIN.h>
#include < string >

#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_module.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/ppb.h"
#include "ppapi/c/ppb_core.h"
#include "ppapi/c/ppb_graphics_2d.h"
#include "ppapi/c/ppb_image_data.h"
#include "ppapi/c/ppb_instance.h"
#include "ppapi/c/ppb_view.h"
#include "ppapi/c/ppp.h"
#include "ppapi/c/ppp_instance.h"
#include "ppapi/c/ppb_input_event.h"
#include "ppapi/c/ppp_input_event.h"

#include "AGVideoWnd.h"

PPB_GetInterface g_get_browser_interface = NULL;

const PPB_Core* g_core_interface;
const PPB_Graphics2D* g_graphics_2d_interface;
const PPB_ImageData* g_image_data_interface;
const PPB_Instance* g_instance_interface;
const PPB_View* g_view_interface;
const PPB_InputEvent *g_input_interface;
const PPB_MouseInputEvent *g_mouse_interface;

/******foruok: create native window begin******/

//static HWND g_child_window = NULL;




struct CreateChildWinParam {
	struct PP_Rect r;
	PP_Instance instance;
	HWND hWndParent;
};



struct InstanceInfo {

	HANDLE g_hThread = NULL;
	DWORD g_dwThreadId = 0;
	PP_Instance pp_instance;
	struct PP_Size last_size;
	char id[32];
	PP_Resource graphics;
	CAGVideoWnd* wnd;
	struct InstanceInfo* next;
};

/** Linked list of all live instances. */
struct InstanceInfo* all_instances = NULL;



/** Returns the info for the given instance, or NULL if it's not found. */
struct InstanceInfo* FindInstance(PP_Instance instance) {
	struct InstanceInfo* cur = all_instances;
	while (cur) {
		if (cur->pp_instance == instance)
			return cur;
		cur = cur->next;
	}
	return NULL;
}


void print(LPCWSTR lpOutputString)
{
	CString ss;
	ss.Append(_T("[PPAPI_DD]"));
	ss.Append(lpOutputString);
	OutputDebugString(ss);
}

CWnd m_localWnd;

DWORD WINAPI ThreadProc(LPVOID lpParam)
{


	MSG msg;
	TCHAR szLog[256] = { 0 };

	struct CreateChildWinParam *para = (struct CreateChildWinParam *)lpParam;

	
	/*g_child_window = CreateWindowEx(0, _T("_ChildWindowClass"), _T("ChildWindow"),
		para->hWndParent == NULL ? (WS_OVERLAPPEDWINDOW | WS_VISIBLE) : (WS_CHILD | WS_VISIBLE  | WS_DLGFRAME),
		para->r.point.x, para->r.point.y, para->r.size.width, para->r.size.height,
		para->hWndParent, NULL, GetModuleHandle(NULL), NULL);*/

	struct InstanceInfo* info=FindInstance(para->instance);
	if (!info)
	{
		print(_T("threadnull"));
		return 0;
	}
	print(_T("in ThreadProc"));
		
	info->wnd->m_bcreate = TRUE;
	USES_CONVERSION;
	//g_child_window.CreateEx(NULL,NULL, _T("remotemedia"), WS_VISIBLE | WS_CHILD | WS_DLGFRAME, para->r.point.x, para->r.point.y, para->r.size.width, para->r.size.height, para->hWndParent,NULL);
	info->wnd->CreateEx(NULL, NULL,A2W(info->id), WS_VISIBLE | WS_CHILD | WS_DLGFRAME, 0,0,0,0, para->hWndParent, NULL);

	/*_stprintf_s(szLog, 256, _T("create child window(standalone msg loop) at (%d, %d) size(%d,%d) parent=0x%08x child = 0x%08x\r\n"),
		para->r.point.x, para->r.point.y, para->r.size.width, para->r.size.height, para->hWndParent,
		g_child_window);*/
	_stprintf_s(szLog, 256, _T("create child window(standalone msg loop) at (%d, %d) size(%d,%d) parent=0x%08x child = 0x%08x\r\n"),
		0,0,0,0, para->hWndParent,
		info->wnd);

	print(szLog);

	BOOL fGotMessage;
	while ((fGotMessage = GetMessage(&msg, (HWND)NULL, 0, 0)) != 0 && fGotMessage != -1)
	{
		TranslateMessage(&msg);
		if (msg.message == WM_USER && msg.hwnd == NULL)
		{
			print(_T("child window message loop quit\r\n"));
			info->g_dwThreadId = 0;
			info->g_hThread = NULL;
			//g_child_window=NULL;
			return 0;
		}
		DispatchMessage(&msg);
	}
	return msg.wParam;
}



void CreateChildWindow(PP_Instance instance)
{
	HWND hwnd = FindWindowEx(NULL, NULL, _T("CefBrowserWindow"), NULL);
	HWND hwndWeb = FindWindowEx(hwnd, NULL, _T("Chrome_WidgetWin_0"), NULL);

	/*
	if (hwndWeb)
	{
	hwndWeb = FindWindowEx(hwndWeb, NULL, _T("Chrome_RenderWidgetHostHWND"), NULL); //web contents
	}*/
	if (hwndWeb != NULL)print(_T("Got Chrome_RenderWidgetHostHWND\r\n"));

	//struct CreateChildWinParam *para = (struct PP_Rect *)malloc(sizeof(struct CreateChildWinParam));

	struct CreateChildWinParam *para = (CreateChildWinParam *)malloc(sizeof(struct CreateChildWinParam));

	para->instance = instance;
	para->hWndParent = hwndWeb;

	
	struct InstanceInfo* info = FindInstance(instance);
	

	info->g_hThread = CreateThread(NULL, 0, ThreadProc, para, 0, &info->g_dwThreadId);
	if (info->g_hThread != NULL)
	{
		print(_T("Launch child window thread.\r\n"));
	}
	else
	{
		print(_T("Launch child window thread FAILED!\r\n"));
	}
	
}

/* PPP_Instance implementation -----------------------------------------------*/



PP_Bool Instance_DidCreate(PP_Instance instance,
	uint32_t argc,
	const char* argn[],
	const char* argv[]) {
	struct InstanceInfo* info =
		(struct InstanceInfo*)malloc(sizeof(struct InstanceInfo));
	info->pp_instance = instance;
	info->last_size.width = 0;
	info->last_size.height = 0;
	info->graphics = 0;
	info->wnd = new CAGVideoWnd();

	/* Insert into linked list of live instances. */
	info->next = all_instances;
	all_instances = info;

	//g_input_interface->RequestInputEvents(instance, PP_INPUTEVENT_CLASS_MOUSE);
	//g_input_interface->RequestFilteringInputEvents(instance, PP_INPUTEVENT_CLASS_MOUSE);

	

	


	TCHAR szLog[256] = { 0 };
	
	int len;
	for (int i = 0; i < argc; i++)
	{

		len = MultiByteToWideChar(CP_ACP, 0, argn[i], -1, NULL, 0);
		wchar_t *w_n = new wchar_t[len];
		memset(w_n, 0, sizeof(wchar_t)*len);
		MultiByteToWideChar(CP_ACP, 0, argn[i], -1, w_n, len);

		len = MultiByteToWideChar(CP_ACP, 0, argv[i], -1, NULL, 0);
		wchar_t *w_string = new wchar_t[len];
		memset(w_string, 0, sizeof(wchar_t)*len);
		MultiByteToWideChar(CP_ACP, 0, argv[i], -1, w_string, len);
		_stprintf_s(szLog, 256, _T("Argument(%d,%s, %s)\r\n"),i, w_n, w_string);
		//print(szLog);

		if (std::strcmp("id", argn[i])==0)
		{
			strncpy_s(info->id, argv[i], 31);
			
			print(szLog);
			
		}

		

	}


	CreateChildWindow(instance);
	
	print(_T("Instance_DidCreate\r\n"));

	return PP_TRUE;
}

void Instance_DidDestroy(PP_Instance instance) {
	/* Find the matching item in the linked list, delete it, and patch the
	* links.
	*/
	struct InstanceInfo** prev_ptr = &all_instances;
	struct InstanceInfo* cur = all_instances;
	while (cur) {
		if (instance == cur->pp_instance) {
			*prev_ptr = cur->next;
			g_core_interface->ReleaseResource(cur->graphics);
			SendMessage(cur->wnd->GetSafeHwnd(), WM_CLOSE, 0, 0);
			if (cur->g_dwThreadId != 0)
			{
				print(_T("Plugin was destroyed, tell child window close and thread exit\r\n"));
				PostThreadMessage(cur->g_dwThreadId, WM_USER, 0, 0);
			}
			delete cur->wnd;
			free(cur);
			return;
		}
		prev_ptr = &cur->next;
		cur = cur->next;
	}

	/**foruok: close native window**/


}

void Instance_DidChangeView(PP_Instance pp_instance,
	PP_Resource view) {
	struct PP_Rect position;
	struct InstanceInfo* info = FindInstance(pp_instance);
	if (!info)
		return;

	if (g_view_interface->GetRect(view, &position) == PP_FALSE)
		return;

	//if (info->last_size.width != position.size.width ||
	//	info->last_size.height != position.size.height) {
	//	/* Got a resize, repaint the plugin. */
	//	Repaint(info, &position.size);
	//	info->last_size.width = position.size.width;
	//	info->last_size.height = position.size.height;

	//	/**foruok: call create window**/
	//
	//}

	if (info->wnd->m_bcreate)
	{
		MoveWindow(info->wnd->GetSafeHwnd(), position.point.x, position.point.y, position.size.width, position.size.height, TRUE);
	}
	
	print(_T("Instance_DidChangeView\r\n"));
}

void Instance_DidChangeFocus(PP_Instance pp_instance, PP_Bool has_focus) {
}

PP_Bool Instance_HandleDocumentLoad(PP_Instance pp_instance,
	PP_Resource pp_url_loader) {
	return PP_FALSE;
}

static PPP_Instance instance_interface = {
	&Instance_DidCreate,
	&Instance_DidDestroy,
	&Instance_DidChangeView,
	&Instance_DidChangeFocus,
	&Instance_HandleDocumentLoad
};

PP_Bool InputEvent_HandleInputEvent(PP_Instance instance, PP_Resource input_event)
{
	struct PP_Point pt;
	TCHAR szLog[512] = { 0 };
	switch (g_input_interface->GetType(input_event))
	{
	case PP_INPUTEVENT_TYPE_MOUSEDOWN:
		pt = g_mouse_interface->GetPosition(input_event);
		_stprintf_s(szLog, 512, _T("InputEvent_HandleInputEvent, mouse down at [%d, %d]\r\n"), pt.x, pt.y);
		print(szLog);
		break;
		/*
		case PP_INPUTEVENT_TYPE_MOUSEUP:
		print(_T("InputEvent_HandleInputEvent, mouse up\r\n"));
		break;

		case PP_INPUTEVENT_TYPE_MOUSEMOVE:
		print(_T("InputEvent_HandleInputEvent, mouse move\r\n"));
		break;
		case PP_INPUTEVENT_TYPE_MOUSEENTER:
		print(_T("InputEvent_HandleInputEvent, mouse enter\r\n"));
		break;
		case PP_INPUTEVENT_TYPE_MOUSELEAVE:
		print(_T("InputEvent_HandleInputEvent, mouse leave\r\n"));
		break;
		*/
	default:
		return PP_FALSE;
	}
	struct InstanceInfo* info = FindInstance(instance);
	if (info && info->last_size.width > 0)
	{

	//	Repaint(info, &info->last_size);
	}
	return PP_TRUE;
}

static PPP_InputEvent input_interface = {
	&InputEvent_HandleInputEvent
};

/* Global entrypoints --------------------------------------------------------*/

PP_EXPORT int32_t PPP_InitializeModule(PP_Module module,
	PPB_GetInterface get_browser_interface) {

	/**foruok: register window class**/
	
	//RegisterVideoWindowClass();

	g_get_browser_interface = get_browser_interface;

	g_core_interface = (const PPB_Core*)
		get_browser_interface(PPB_CORE_INTERFACE);
	g_instance_interface = (const PPB_Instance*)
		get_browser_interface(PPB_INSTANCE_INTERFACE);
	g_image_data_interface = (const PPB_ImageData*)
		get_browser_interface(PPB_IMAGEDATA_INTERFACE);
	g_graphics_2d_interface = (const PPB_Graphics2D*)
		get_browser_interface(PPB_GRAPHICS_2D_INTERFACE);
	g_view_interface = (const PPB_View*)
		get_browser_interface(PPB_VIEW_INTERFACE);
	g_input_interface = (const PPB_InputEvent*)get_browser_interface(PPB_INPUT_EVENT_INTERFACE);
	g_mouse_interface = (const PPB_MouseInputEvent*)get_browser_interface(PPB_MOUSE_INPUT_EVENT_INTERFACE);

	if (!g_core_interface || !g_instance_interface || !g_image_data_interface ||
		!g_graphics_2d_interface || !g_view_interface ||
		!g_input_interface || !g_mouse_interface)
		return -1;

	print(_T("PPP_InitializeModule\r\n"));
	return PP_OK;
}

PP_EXPORT void PPP_ShutdownModule() {
}

PP_EXPORT const void* PPP_GetInterface(const char* interface_name) {
	if (strcmp(interface_name, PPP_INSTANCE_INTERFACE) == 0)
	{
		print(_T("PPP_GetInterface, instance_interface\r\n"));
		return &instance_interface;
	}
	else if (strcmp(interface_name, PPP_INPUT_EVENT_INTERFACE) == 0)
	{
		print(_T("PPP_GetInterface, input_interface\r\n"));
		return &input_interface;
	}

	return NULL;
}