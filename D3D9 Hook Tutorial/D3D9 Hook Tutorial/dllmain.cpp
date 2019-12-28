#pragma comment(lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")
#pragma comment(lib, "detours.lib")
#include <Windows.h>
#include <iostream>
#include <string>
#include <Detours.h>
#include <imgui.h>
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>
#include <d3dx9.h>
#include "mem.h"

typedef HRESULT(__stdcall* EndScene)(LPDIRECT3DDEVICE9 pDevice);
HRESULT __stdcall hkEndScene(LPDIRECT3DDEVICE9 pDevice);
EndScene oEndScene;
LPDIRECT3DDEVICE9	g_pd3dDevice = NULL;
IDirect3D9 *		g_pD3D = NULL;
HWND				window = NULL;
WNDPROC				wndproc_orig = NULL;
/**********************************************************************/
uintptr_t client = (uintptr_t)GetModuleHandle(TEXT("client_panorama.dll"));
uintptr_t dwGlowObjectManager = 0x527DF60;
uintptr_t m_iGlowIndex = 0xA428;
uintptr_t dwEntityList = 0x4D3C68C;
uintptr_t dwLocalPlayer = 0xD28B1C;
uintptr_t m_bDormant = 0xED;
uintptr_t m_iTeamNum = 0xF4;
/**********************************************************************/
uintptr_t pLocalPlayer = client + dwLocalPlayer;
uintptr_t pGlowObjectManager = client + dwGlowObjectManager;
uintptr_t pEntityList = client + dwEntityList;
/**********************************************************************/
bool show_menu = false;
bool glow = false;
bool shoot = false;
bool bhop = false;
void aglow();
void ashoot();
void abhop();
/**********************************************************************///KEEP!

BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
{
	DWORD wndProcId;
	GetWindowThreadProcessId(handle, &wndProcId);

	if (GetCurrentProcessId() != wndProcId)
		return TRUE; // skip to next window

	window = handle;
	return FALSE; // window found abort search
}
HWND GetProcessWindow()
{
	window = NULL;
	EnumWindows(EnumWindowsCallback, NULL);
	return window;
}
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (show_menu && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
	{
		return true;
	}

	return CallWindowProc(wndproc_orig, hWnd, msg, wParam, lParam);
}
bool GetD3D9Device(void ** pTable, size_t Size)
{
	if (!pTable)
		return false;

	IDirect3D9 * g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);

	if (!g_pD3D)
		return false;

	// options to create dummy device
	D3DPRESENT_PARAMETERS d3dpp = {};
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = GetProcessWindow();
	d3dpp.Windowed = false;

	g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice);

	if (!g_pd3dDevice)
	{
		// may fail in windowed fullscreen mode, trying again with windowed mode
		d3dpp.Windowed = !d3dpp.Windowed;

		g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &g_pd3dDevice);

		if (!g_pd3dDevice)
		{
			g_pD3D->Release();
			return false;
		}
	}

	memcpy(pTable, *reinterpret_cast<void***>(g_pd3dDevice), Size);

	g_pd3dDevice->Release();
	g_pD3D->Release();
	return true;
}
void CleanupDeviceD3D()
{
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
	if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}
HRESULT __stdcall hkEndScene(LPDIRECT3DDEVICE9 pDevice)
{
	static bool init = false;

	if (GetAsyncKeyState(VK_INSERT) & 1)
	{
		show_menu = !show_menu;
	}

	if (!init)
	{
		MessageBoxA(0, "HOOKED", "hooked successfully", 0);
		wndproc_orig = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();

		ImGui::StyleColorsDark();

		ImGui_ImplWin32_Init(window);
		ImGui_ImplDX9_Init(pDevice);

		init = true;
	}

	if (init)
	{
		if (show_menu)
		{
			ImGui_ImplDX9_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			ImGui::Begin("Menu", &show_menu);
			{
				ImGui::Checkbox("GLOW", &glow);
				ImGui::Checkbox("TRIGGERBOT", &shoot);
				ImGui::Checkbox("BHOP", &bhop);
			}

			ImGui::Render();
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		}

	}

	return oEndScene(pDevice);
}
DWORD WINAPI mainThread(PVOID base)
{
	void * d3d9Device[119];

	if (GetD3D9Device(d3d9Device, sizeof(d3d9Device)))
	{
		oEndScene = (EndScene)d3d9Device[42];
		oEndScene = (EndScene)Detours::X86::DetourFunction((Detours::uint8_t*)d3d9Device[42], (Detours::uint8_t*)hkEndScene);
		while (true)
		{
			if (GetAsyncKeyState(VK_F10))
			{
				break; //crash
			}

			if (glow)
			{
				aglow();
			}
			Sleep(5);

			if (shoot)
			{
				ashoot();
			}
			Sleep(5);

			if (bhop)
			{
				abhop();
			}
			Sleep(5);
		}
		FreeLibraryAndExitThread(static_cast<HMODULE>(base), 1);
	}
}
void aglow()
{ 
/*******************************************************************************/
	uintptr_t LocalPlayer = *(uintptr_t*)pLocalPlayer; 
	uintptr_t GlowObjectManager = *(uintptr_t*)pGlowObjectManager;
	uintptr_t EntityList = *(uintptr_t*)pEntityList;
/*******************************************************************************/ 

	if (LocalPlayer != NULL && GlowObjectManager != NULL && EntityList != NULL)
	{
		int myTeamNum = *(int*)(LocalPlayer + m_iTeamNum);
		for (short i = 0; i < 64; i++)
		{
			uintptr_t entity = *(uintptr_t*)(pEntityList + i * 0x10);
			if (entity == NULL) continue;
			if (*(bool*)(entity + m_bDormant)) continue;

			int glowIndex = *(int*)(entity + m_iGlowIndex);
			int entTeamNum = *(int*)(entity + m_iTeamNum);

			if (entTeamNum == myTeamNum)
			{
				// Teammate
				*(float*)((GlowObjectManager)+((glowIndex * 0x38) + 0x4)) = 0.f;
				*(float*)((GlowObjectManager)+((glowIndex * 0x38) + 0x8)) = 0.f;
				*(float*)((GlowObjectManager)+((glowIndex * 0x38) + 0xC)) = 1.f;
				*(float*)((GlowObjectManager)+((glowIndex * 0x38) + 0x10)) = 1.7f;
			}
			else
			{
				// Enemy
				*(float*)((GlowObjectManager)+((glowIndex * 0x38) + 0x4)) = 1.f;
				*(float*)((GlowObjectManager)+((glowIndex * 0x38) + 0x8)) = 0.f;
				*(float*)((GlowObjectManager)+((glowIndex * 0x38) + 0xC)) = 0.f;
				*(float*)((GlowObjectManager)+((glowIndex * 0x38) + 0x10)) = 1.7f;
			}
			*(bool*)((GlowObjectManager)+((glowIndex * 0x38) + 0x24)) = true;
			*(bool*)((GlowObjectManager)+((glowIndex * 0x38) + 0x25)) = false;
		}
	}
}
void ashoot()
{
	uintptr_t looking = mem::FindDMAAddy(client + dwLocalPlayer, { 0xB3D4 });
	int* look = (int*)looking;
	if (*look >= 2 && *look <= 64)
	{
		mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
		mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
	}
	Sleep(5);
}
void abhop()
{
	if (GetAsyncKeyState(VK_SPACE) & 0x8000 || (GetAsyncKeyState(VK_SPACE) && (GetAsyncKeyState('A') & 0x8000 || (GetAsyncKeyState(VK_SPACE) && (GetAsyncKeyState('D') & 0x8000)))))
	{
		keybd_event(VK_SPACE, 0x39, NULL, NULL);
        keybd_event(VK_SPACE, 0x39, KEYEVENTF_KEYUP, NULL);
	}
	Sleep(5);
}

BOOL APIENTRY DllMain( HMODULE hModule,DWORD  ul_reason_for_call,LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		CreateThread(nullptr, NULL, mainThread, hModule, NULL, nullptr);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

