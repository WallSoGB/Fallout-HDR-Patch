#include "nvse/PluginAPI.h"

NVSEInterface* g_nvseInterface{};

void __forceinline SafeWrite32(UInt32 addr, UInt32 data)
{
	UInt32	oldProtect;

	VirtualProtect((void*)addr, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
	*((UInt32*)addr) = data;
	VirtualProtect((void*)addr, 4, oldProtect, &oldProtect);
}

void __forceinline ReplaceCall(UInt32 jumpSrc, UInt32 jumpTgt)
{
	SafeWrite32(jumpSrc + 1, jumpTgt - jumpSrc - 1 - 4);
}

bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info)
{
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "HDR Patch";
	info->version = 100;

	return true;
}

typedef bool(__cdecl* DisableFormatUpgradeFunc)();
typedef bool(__cdecl* EnableFormatUpgradeFunc)();

void* (__cdecl* CreateBSRenderedTexture)(void*, const UInt32, const UInt32, void*, UInt32, bool, void*, UInt32, UInt32) = (void* (__cdecl*)(void*, const UInt32, const UInt32, void*, UInt32, bool, void*, UInt32, UInt32))0xB6B610;

void* __cdecl CreateSaveTextureHook(void* apName, const UInt32 uiWidth, const UInt32 uiHeight,
	void* kPrefs, UInt32 eMSAAPref,
	bool bUseDepthStencil, void* pkDSBuffer, UInt32 a7, UInt32 uiBackgroundColor) {
	HMODULE hDLL = GetModuleHandle("d3d9.dll");
	DisableFormatUpgradeFunc disable = reinterpret_cast<DisableFormatUpgradeFunc>(GetProcAddress(hDLL, "DXVK_D3D9_HDR_DisableRenderTargetUpgrade"));
	EnableFormatUpgradeFunc enable = reinterpret_cast<DisableFormatUpgradeFunc>(GetProcAddress(hDLL, "DXVK_D3D9_HDR_EnableRenderTargetUpgrade"));

	if (disable)
		disable();
	void* pTexture = CreateBSRenderedTexture(apName, uiWidth, uiHeight, kPrefs, eMSAAPref, bUseDepthStencil, pkDSBuffer, a7, uiBackgroundColor);
	if (enable) {
		enable();
	}

	return pTexture;
}

bool NVSEPlugin_Load(NVSEInterface* nvse) {
	if (!nvse->isEditor) {
		ReplaceCall(0x879061, (UInt32)CreateSaveTextureHook);
	}

	return true;
}