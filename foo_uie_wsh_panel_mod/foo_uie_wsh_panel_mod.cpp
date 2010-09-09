#include "stdafx.h"
#include "simple_thread.h"
#include "popup_msg.h"
#include "panel_notifier.h"
#include "user_message.h"
#include "version.h"


// Script TypeLib
ITypeLibPtr g_typelib;

namespace
{
	DECLARE_COMPONENT_VERSION(
		WSPM_NAME,
		WSPM_VERSION_NUMBER,
		"Windows Scripting Host Panel Modded\n"
		"Modded by T.P. Wang\n\n"
		"Build: "  __TIME__ ", " __DATE__ "\n"
		"Columns UI API Version: " UI_EXTENSION_VERSION "\n\n\n\n"
		"Scintilla and SciTE\n"
		"Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>\n\n\n"
		"Text Designer Outline Text Library\n"
		"Copyright (c) 2009 Wong Shao Voon\n\n\n"
		"box blur filter\n"
		"Copyright 2006 Jerry Huxtable\n\n\n"
		"CPropertyList - A Property List control\n"
		"Copyright (c) 2001-2003 Bjarke Viksoe\n\n\n"
	);

	VALIDATE_COMPONENT_FILENAME("foo_uie_wsh_panel_mod.dll");

	// Is there anything not correctly loaded?
	enum t_load_status_error
	{
		E_OK = 0,
		E_TYPELIB = 1 << 0,
		E_SCINTILLA = 1 << 1,
		E_GDIPLUS = 1 << 2,
	};

	static int g_load_status = E_OK;

	class wsh_initquit : public initquit
	{
	public:
		void on_init()
		{
			// HACK: popup_message services will not be initialized soon after start.
			popup_msg::g_set_service_initialized();
			check_error_();
			popup_msg::g_process_pendings();
		}

		void on_quit()
		{
			simple_thread_manager::instance().remove_all();
			panel_notifier_manager::instance().send_msg_to_all(UWM_SCRIPT_TERM, 0, 0);
		}

	private:
		void check_error_() 
		{
			// Check and show error message
			pfc::string8 err_msg;

			if (IS_EXPIRED(__DATE__))
			{
				err_msg = "This version of WSH Panel Mod is expired, please get a new version now.\n\n";
			}
			else if (g_load_status != E_OK)
			{
				err_msg = "If you see this error message, that means this component will not function properly:\n\n";

				if (g_load_status & E_TYPELIB)
					err_msg += "Type Library: Load TypeLib Failed.\n\n";

				if (g_load_status & E_SCINTILLA)
					err_msg += "Scintilla: Load Scintilla Failed.\n\n";

				if (g_load_status & E_GDIPLUS)
					err_msg += "Gdiplus: Load Gdiplus Failed.\n\n";
			}

			if (!err_msg.is_empty())
				popup_msg::g_show(err_msg, "WSH Panel Mod", popup_message::icon_error);
		}
	};

	static initquit_factory_t<wsh_initquit> g_initquit;
	CAppModule _Module;

	extern "C" BOOL WINAPI DllMain(HINSTANCE ins, DWORD reason, LPVOID lp)
	{
		static ULONG_PTR g_gdip_token;

		switch (reason)
		{
		case DLL_PROCESS_ATTACH:
			{
				// Load TypeLib
				TCHAR path[MAX_PATH + 4];
				DWORD len = GetModuleFileName(ins, path, MAX_PATH);

				path[len] = 0;

				if (FAILED(LoadTypeLibEx(path, REGKIND_NONE, &g_typelib)))
					g_load_status |= E_TYPELIB;

				// Load Scintilla
				if (!Scintilla_RegisterClasses(ins))
					g_load_status |= E_SCINTILLA;

				// Init GDI+
				Gdiplus::GdiplusStartupInput gdip_input;
				if (Gdiplus::GdiplusStartup(&g_gdip_token, &gdip_input, NULL) != Gdiplus::Ok)
					g_load_status |= E_GDIPLUS;

				// WTL
				_Module.Init(NULL, ins);
			}
			break;

		case DLL_PROCESS_DETACH:
			{
				// Term WTL
				_Module.Term();

				// Shutdown GDI+
				Gdiplus::GdiplusShutdown(g_gdip_token);

				// Free Scintilla resource
				Scintilla_ReleaseResources();
			}
			break;
		}

		return TRUE;
	}

}
