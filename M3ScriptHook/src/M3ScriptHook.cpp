/*
 * Copyright (c) 2010 Barzakh (martinjk 'at' outlook 'dot' com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.

 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.

 * 3. This notice may not be removed or altered from any source
 * distribution.

 * 4. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <windows.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <algorithm>

#include <M3ScriptHook.h>
#include <Common.h>
#include <LuaFunctions.h>
#include <ScriptSystem.h>
#include <hooking/hooking.h>

#ifdef _WIN64
#define DLLPATH "\\\\.\\GLOBALROOT\\SystemRoot\\System32\\dxgi.dll"
#else
#define DLLPATH "\\\\.\\GLOBALROOT\\SystemRoot\\SysWOW64\\dxgi.dll"
#endif // _WIN64

#pragma comment(linker, "/EXPORT:ApplyCompatResolutionQuirking=" DLLPATH ".ApplyCompatResolutionQuirking")
#pragma comment(linker, "/EXPORT:CompatString=" DLLPATH ".CompatString")
#pragma comment(linker, "/EXPORT:CompatValue=" DLLPATH ".CompatValue")
#pragma comment(linker, "/EXPORT:CreateDXGIFactory=" DLLPATH ".CreateDXGIFactory")
#pragma comment(linker, "/EXPORT:CreateDXGIFactory1=" DLLPATH ".CreateDXGIFactory1")
#pragma comment(linker, "/EXPORT:CreateDXGIFactory2=" DLLPATH ".CreateDXGIFactory2")
#pragma comment(linker, "/EXPORT:DXGID3D10CreateDevice=" DLLPATH ".DXGID3D10CreateDevice")
#pragma comment(linker, "/EXPORT:DXGID3D10CreateLayeredDevice=" DLLPATH ".DXGID3D10CreateLayeredDevice")
#pragma comment(linker, "/EXPORT:DXGID3D10GetLayeredDeviceSize=" DLLPATH ".DXGID3D10GetLayeredDeviceSize")
#pragma comment(linker, "/EXPORT:DXGID3D10RegisterLayers=" DLLPATH ".DXGID3D10RegisterLayers")
#pragma comment(linker, "/EXPORT:DXGIDeclareAdapterRemovalSupport=" DLLPATH ".DXGIDeclareAdapterRemovalSupport")
#pragma comment(linker, "/EXPORT:DXGIDisableVBlankVirtualization=" DLLPATH ".DXGIDisableVBlankVirtualization")
#pragma comment(linker, "/EXPORT:DXGIDumpJournal=" DLLPATH ".DXGIDumpJournal")
#pragma comment(linker, "/EXPORT:DXGIGetDebugInterface1=" DLLPATH ".DXGIGetDebugInterface1")
#pragma comment(linker, "/EXPORT:DXGIReportAdapterConfiguration=" DLLPATH ".DXGIReportAdapterConfiguration")
#pragma comment(linker, "/EXPORT:PIXBeginCapture=" DLLPATH ".PIXBeginCapture")
#pragma comment(linker, "/EXPORT:PIXEndCapture=" DLLPATH ".PIXEndCapture")
#pragma comment(linker, "/EXPORT:PIXGetCaptureState=" DLLPATH ".PIXGetCaptureState")
#pragma comment(linker, "/EXPORT:SetAppCompatStringPointer=" DLLPATH ".SetAppCompatStringPointer")
#pragma comment(linker, "/EXPORT:UpdateHMDEmulationStatus=" DLLPATH ".UpdateHMDEmulationStatus")

M3ScriptHook::M3ScriptHook()
{
	Log(__FUNCTION__);
	hooking::hooking_helpers::SetExecutableAddress((uintptr_t)GetModuleHandle(0)); 
	hooking::ExecutableInfo::instance()->EnsureExecutableInfo();
	hooking::ExecutableInfo::instance()->GetExecutableInfo().SetSSEPatternSearching(false);

	this->keyBinds.clear();
}

#define BUFFER_COUNT 8
#define BUFFER_LENGTH 32768

void M3ScriptHook::Log(const char* string, ...)
{
	static int32_t currentBuffer;
	static char* buffer = nullptr;

	if (!buffer)
	{
		buffer = new char[BUFFER_COUNT * BUFFER_LENGTH];
	}

	int32_t thisBuffer = currentBuffer;

	va_list ap;
	va_start(ap, string);
	int32_t length = vsnprintf(&buffer[thisBuffer * BUFFER_LENGTH], BUFFER_LENGTH, string, ap);
	va_end(ap);

	if (length >= BUFFER_LENGTH)
	{
		__debugbreak();
		exit(1);
	}

	buffer[(thisBuffer * BUFFER_LENGTH) + BUFFER_LENGTH - 1] = '\0';

	currentBuffer = (currentBuffer + 1) % BUFFER_COUNT;

	const char* msg =  &buffer[thisBuffer * BUFFER_LENGTH];

	std::fstream file("ScriptHook.log", std::ios::out | std::ios::app);

	auto now = std::chrono::system_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
	auto timer = std::chrono::system_clock::to_time_t(now);
	auto local_time = *std::localtime(&timer);

	file << "[" << std::put_time(&local_time, "%m/%d/%Y %I:%M:%S") << ":" << std::setfill('0') << std::setw(3) << ms.count() << " " << std::put_time(&local_time, "%p") << "] " << msg << std::endl;
	file.close();
}

void M3ScriptHook::Log(std::string message)
{
	return this->Log(message.c_str());
}

void M3ScriptHook::LogToFile(const char *fileName, const char *string, ...)
{
	static int32_t currentBuffer;
	static char* buffer = nullptr;

	if (!buffer)
	{
		buffer = new char[BUFFER_COUNT * BUFFER_LENGTH];
	}

	int32_t thisBuffer = currentBuffer;

	va_list ap;
	va_start(ap, string);
	int32_t length = vsnprintf(&buffer[thisBuffer * BUFFER_LENGTH], BUFFER_LENGTH, string, ap);
	va_end(ap);

	if (length >= BUFFER_LENGTH)
	{
		__debugbreak();
		exit(1);
	}

	buffer[(thisBuffer * BUFFER_LENGTH) + BUFFER_LENGTH - 1] = '\0';

	currentBuffer = (currentBuffer + 1) % BUFFER_COUNT;

	const char* msg = &buffer[thisBuffer * BUFFER_LENGTH];

	std::fstream file(fileName, std::ios::out | std::ios::app);
	file << msg;
	file << "\n";
	file.close();
}

void M3ScriptHook::EndThreads()
{
	this->Log(__FUNCTION__);
	this->m_bEnded = true;
	PluginSystem::instance()->StopPlugins();
	delete LuaStateManager::instance();
}

void M3ScriptHook::LoadScript(const std::string &file)
{
	this->Log(__FUNCTION__);
	auto threadState = LuaStateManager::instance()->GetState();
	this->LoadLuaFile(threadState, file);
}

void M3ScriptHook::LoadLuaFile(lua_State *L, const std::string &name)
{
	this->Log(__FUNCTION__);
	std::string file = "function dofile (filename)local f = assert(loadfile(filename)) return f() end dofile(\"";
	file.append(name);
	file.append("\")");
	this->ExecuteLua(L, file);
}

// Export
LUA_API bool ExecuteLua(lua_State *L, const std::string &lua) 
{
	M3ScriptHook::instance()->Log(__FUNCTION__);
	return M3ScriptHook::instance()->ExecuteLua(L, lua);
}

bool M3ScriptHook::ExecuteLua(lua_State *L, const std::string &lua)
{
	this->Log(std::string("Trying to execute: " + lua));

	if (!L) {
		this->Log("BadState");
		return false;
	}

	luaL_loadbuffer_(L, const_cast<char*>(lua.c_str()), lua.length(), "Mafia3ScriptHook");

	int32_t result = lua_pcall_(L, 0, LUA_MULTRET, 0);

	if (result != 0)
	{
		if (LUA_ERRSYNTAX == result)
		{
			this->Log("Error loading Lua code into buffer with (Syntax Error)");
			return false;
		}
		else if (LUA_ERRMEM == result)
		{
			this->Log("Error loading Lua code into buffer with (Memory Allocation Error)");
			return false;
		}
		else
		{
			std::stringstream ss;
			ss << "Error loading Lua code into buffer. Error ";
			ss << result;
			this->Log(ss.str());
			const char *error = lua_tostring_(L, -1);
			this->Log(error);
			return false;
		}
	}
	return true;
}

uint32_t WINAPI M3ScriptHook::mainThread(LPVOID) {
	static M3ScriptHook *instance = M3ScriptHook::instance();

	instance->Log(__FUNCTION__);

	while (!instance->HasEnded()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		std::this_thread::yield(); // Process other threads

		M3ScriptHook::instance()->ProcessKeyBinds();

		if (GetAsyncKeyState(VK_F1) & 1) {
			ScriptSystem::instance()->ReloadScripts();
		}
		/*if (GetAsyncKeyState(VK_F2) & 1) {
			instance->Shutdown();
			PluginSystem::instance()->ReloadPlugins();
		}*/
		/*
		if (GetAsyncKeyState(VK_F3) & 1) {
			instance->Shutdown();
		}

		if (GetAsyncKeyState(VK_F6) & 1) {
			instance->Shutdown();
			PluginSystem::instance()->UnloadPlugins();
		}*/

	}

	return 0;
}

void M3ScriptHook::StartThreads()
{
	this->Log(__FUNCTION__);
	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)M3ScriptHook::mainThread, 0, 0, 0);

	LuaStateManager::instance()->StartThread();
}

bool M3ScriptHook::HasEnded()
{
	return this->m_bEnded;
}

void M3ScriptHook::Shutdown()
{
	this->Log(__FUNCTION__);
	this->m_bEnded = true;
	this->EndThreads();
	FreeLibraryAndExitThread((HMODULE)GetModuleHandle("M3ScriptHook.dll"), 0);
}

void M3ScriptHook::CreateKeyBind(const char *key, const char *context)
{
	M3ScriptHook::instance()->Log("Binding key %s to function %s", key, context);
	std::unique_lock<std::recursive_mutex> lkScr(_keyBindMutex);

	bool found = false;
	uint8_t keyID = -1;
	for (int32_t i = 0; i < 0xFE && !found; ++i) {
		if (strcmp(key, BindableKeys[i]) == 0) {
			keyID = i;
			found = true;
			break;
		}
	}
	if (found) {
		this->keyBinds[keyID] = context; //.push_back(new M3KeyBind(keyID, context));
	}
	else {
		this->Log("Could not create keybind, key %s is unknown", key);
	}
}

void M3ScriptHook::DestroyKeyBind(const char* key)
{
	M3ScriptHook::instance()->Log("Unbinding key %", key);
	//std::lock_guard<std::mutex> lk{ _keyBindMutex };
	std::unique_lock<std::recursive_mutex> lkScr(_keyBindMutex);

	bool found = false;
	uint8_t keyID = -1;
	for (int32_t i = 0; i < 0xFE && !found; ++i) {
		if (strcmp(key, BindableKeys[i]) == 0) {
			keyID = i;
			found = true;
			break;
		}
	}

	if (found)
	{
		/*for (auto bind : keyBinds) //; it != keyBinds.end();)
		{
			if (bind->key == keyID) {
				keyBinds.erase(std::remove(keyBinds.begin(), keyBinds.end(), bind), keyBinds.end());
			}
		}*/
		this->keyBinds.erase(keyID);
	}
}

void M3ScriptHook::ProcessKeyBinds()
{
	//this->Log(__FUNCTION__);
	std::unique_lock<std::recursive_mutex> lkScr(_keyBindMutex);

	auto L = GetL();
	if (!L)
		return;

	//this->Log("%d", keyBinds.size());
	if (!keyBinds.size())
		return;

	/*auto it = keyBinds.begin();
	for (auto bind : keyBinds) //; it != keyBinds.end();)
	{
		if (GetAsyncKeyState(bind->key) & 1)
		{
			M3ScriptHook::instance()->ExecuteLua(L, bind->bind);
		}
		//++it;
	}*/
	for (auto it = keyBinds.begin(); it != keyBinds.end(); ++it) {
		if (GetAsyncKeyState(it->first) & 1) {
			M3ScriptHook::instance()->ExecuteLua(L, it->second);
		}
	}

}

BOOL APIENTRY DllMain(HMODULE, DWORD code, LPVOID) {
	switch (code) {
	case DLL_PROCESS_ATTACH:
		PluginSystem::instance()->LoadPlugins();
		M3ScriptHook::instance()->StartThreads();
		break;
	case DLL_PROCESS_DETACH:
		M3ScriptHook::instance()->Shutdown();
		break;
	}
	return TRUE;
}


