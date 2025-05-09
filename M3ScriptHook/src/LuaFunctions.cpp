// Base Application taken from Klusark (GPLv2)
// https://code.google.com/archive/p/mafia2injector/

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

#include <Export.h>
#include <Common.h>
#include <sstream>
#include <LuaFunctions.h>
#include <M3ScriptHook.h>
#include <cstdint>
#include <hooking/hooking.h>
#include <fstream>
#include <thread>

 /************************************************************************/
 /* Find pattern implementation											*/
 /************************************************************************/
uint64_t GetPointerFromPattern(const char *name, const char *pattern)
{
	auto pat = hooking::pattern(pattern).get(0).origaddr();
	return pat;
}

/************************************************************************/
/* Lua Load Buffer impl													*/
/************************************************************************/
typedef int32_t(__cdecl *luaL_loadbuffer_t)(lua_State *L, char *buff, size_t size, char *name);
luaL_loadbuffer_t		pluaL_loadbuffer = nullptr;

__declspec(dllexport) int32_t luaL_loadbuffer_(lua_State *L, char *buff, size_t size, char *name)
{
	return pluaL_loadbuffer(L, buff, size, name);
}

/************************************************************************/
/* Lua pcall implementation                                             */
/************************************************************************/
typedef int32_t(__cdecl *lua_pcall_t)(lua_State *L, int32_t nargs, int32_t nresults, int32_t errfunc);
lua_pcall_t				plua_pcall2 = nullptr;

__declspec(dllexport) int32_t lua_pcall_(lua_State *L, int32_t nargs, int32_t nresults, int32_t errfunc)
{
	return plua_pcall2(L, nargs, nresults, errfunc);
}

/************************************************************************/
/* Lua tolstring implementation                                         */
/************************************************************************/
typedef const char *	(__cdecl *lua_tostring_t) (lua_State *L, int32_t idx);
lua_tostring_t			plua_tostring = nullptr;

__declspec(dllexport) const char *lua_tostring_(lua_State *L, int32_t idx)
{
	return plua_tostring(L, idx);
}

/************************************************************************/
/* Lua isstring implementation                                         */
/************************************************************************/
typedef uint32_t(__cdecl *lua_isstring_t) (lua_State *L, int32_t idx);
lua_isstring_t			plua_isstring = nullptr;

__declspec(dllexport) uint32_t lua_isstring_(lua_State *L, int32_t idx)
{
	return plua_isstring(L, idx);
}

/************************************************************************/
/* Lua newthread implementation                                         */
/************************************************************************/
typedef	lua_State *		(__cdecl *lua_newthread_t) (lua_State *L);
lua_newthread_t		plua_newthread = nullptr;

__declspec(dllexport) lua_State *lua_newthread_(lua_State *L)
{
	return plua_newthread(L);
}

/************************************************************************/
/* Lua pushcclosure implementation                                      */
/************************************************************************/
typedef	lua_State *		(__cdecl *lua_pushcclosure_t) (lua_State *L, lua_CFunction fn, int n, int64_t a);
lua_pushcclosure_t		plua_pushcclosure = nullptr;

__declspec(dllexport) lua_State *lua_pushcclosure_(lua_State *L, lua_CFunction fn, int n, int64_t a = 0)
{
	return plua_pushcclosure(L, fn, n, a);
}

/************************************************************************/
/* Lua setglobal implementation		                                    */
/************************************************************************/
typedef	lua_State *		(__cdecl *lua_setglobal_t) (lua_State *L, const char *var);
lua_setglobal_t		  plua_setglobal = nullptr;

__declspec(dllexport) lua_State *lua_setglobal_(lua_State *L, const char *var)
{
	return plua_setglobal(L, var);
}

/************************************************************************/
/* Lua setfield implementation		                                    */
/************************************************************************/
typedef	lua_State *		(__cdecl *lua_setfield_t) (lua_State *L, int idx, const char *k);
lua_setfield_t		  plua_setfield = nullptr;

__declspec(dllexport) lua_State *lua_setglobal_(lua_State *L, int idx, const char *k)
{
	return plua_setfield(L, idx, k);
}

/************************************************************************/
/* (Havok) Lua gettop implementation			                        */
/************************************************************************/
int32_t lua_gettop_(lua_State *L)
{
	return (int32_t)((*(uintptr_t *)((uintptr_t)L + 72) - *(uintptr_t *)((uintptr_t)L + 80)) >> 4);
}

//
__declspec(dllexport) void logPointer(std::string name, uint64_t pointer)
{
	if (pointer == NULL)
	{
		std::stringstream ss;
		ss << name << "Failed to find: " << name;
		M3ScriptHook::instance()->Log(ss.str());
		return;
	}

	// Get the base address of the main module
	uint64_t baseAddress = reinterpret_cast<uint64_t>(GetModuleHandle(nullptr));

	// Calculate the offset
	uint64_t offset = pointer - baseAddress;

	// Format the log message
	std::stringstream ss;
	ss << name << " Mafia3DefinitiveEdition.exe+0x" << std::hex << std::uppercase << offset;

	// Log the message
	M3ScriptHook::instance()->Log(ss.str().c_str());
}

//
lua_State* GetL(C_ScriptGameMachine *pMainScriptMachine)
{
	if (!pMainScriptMachine) {
		pMainScriptMachine = LuaFunctions::instance()->GetMainGameScriptMachine();
	}

	auto luaPtr = *(lua_State **)((uintptr_t)pMainScriptMachine + 208);
	return luaPtr;
}

int32_t LuaFunctions::PrintToLog(lua_State *L)
{
	const char *logFile;
	const char *message;

	if (plua_isstring(L, 1)) {
		logFile = plua_tostring(L, 1);
	}

	if (plua_isstring(L, 2)) {
		message = plua_tostring(L, 2);
	}

	M3ScriptHook::instance()->LogToFile(logFile, message);
	return 0;
}

int32_t LuaFunctions::BindKey(lua_State *L)
{
	const char *key = "";
	const char *context = "";

	if (plua_isstring(L, 1)) {
		key = plua_tostring(L, 1);
	}

	if (plua_isstring(L, 2)) {
		context = plua_tostring(L, 2);
	}

	M3ScriptHook::instance()->CreateKeyBind(key, context);
	return 0;
}

int32_t LuaFunctions::UnbindKey(lua_State *L)
{
	const char *key = "";
	const char *context = "";

	if (plua_isstring(L, 1)) {
		key = plua_tostring(L, 1);
	}

	if (plua_isstring(L, 2)) {
		context = plua_tostring(L, 2);
	}

	M3ScriptHook::instance()->DestroyKeyBind(key);
	return 0;
}

int32_t LuaFunctions::DelayBuffer(lua_State *L)
{
	M3ScriptHook::instance()->Log(__FUNCTION__);
	const char *time = "";
	const char *context = "";

	if (plua_isstring(L, 1)) {
		time = plua_tostring(L, 1);
	}

	if (plua_isstring(L, 2)) {
		context = plua_tostring(L, 2);
	}

	// Pretty hacky implementation, we should consider using a Job queue instead and checking time?
	std::thread th = std::thread([L, time, context]() {
		M3ScriptHook::instance()->Log(__FUNCTION__);
		auto mtime = std::stoi(time);
		std::this_thread::sleep_for(std::chrono::milliseconds(mtime));
		M3ScriptHook::instance()->ExecuteLua(L, context);
	});
	th.detach();

	return 0;
}

int32_t LuaFunctions::FNV32a(lua_State *L)
{
	M3ScriptHook::instance()->Log(__FUNCTION__);
	const char *toHash = "";

	if (plua_isstring(L, 1)) {
		toHash = plua_tostring(L, 1);
	}

	static auto fnvHash = [](const char* str)
	{
		const size_t length = strlen(str) + 1;
		unsigned int hash = 2166136261u;
		for (size_t i = 0; i < length; ++i)
		{
			hash ^= *str++;
			hash *= 16777619u;
		}
		return hash;
	};

	return fnvHash(toHash);
}

//
LuaFunctions::LuaFunctions()
{
	M3ScriptHook::instance()->Log(__FUNCTION__);
	if (!this->LoadPointers())
	{
		M3ScriptHook::instance()->Log(__FUNCTION__ " Failed to find signatures.");
		exit(1);
	}
}

C_ScriptGameMachine *LuaFunctions::GetMainGameScriptMachine()
{
	return this->m_pMainGameScriptMachine;
}

bool LuaFunctions::IsMainScriptMachineReady()
{
	return this->m_mainScriptMachineReady;
}

bool LuaFunctions::LoadPointers()
{
	M3ScriptHook::instance()->Log(__FUNCTION__);

	uint64_t engineAssignAddress = hooking::pattern("48 89 3D ? ? ? ? 48 8B 07").get(0).origaddr();
	if (!engineAssignAddress)
	{
		return this->m_mainScriptMachineReady;
	}
	uint64_t engine = engineAssignAddress + *(int32_t *)(engineAssignAddress + 3) + 7;
	logPointer("m_pMainGameScriptMachine", engine);
	while (*(uintptr_t *)engine == 0)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	this->m_pMainGameScriptMachine = *(C_ScriptGameMachine**)engine;

	auto pCallAddr = GetPointerFromPattern("lua_pcall", "E8 ? ? ? ? 8B D8 85 C0 75 ? 4C 8B C5");
	logPointer("lua_pcall", pCallAddr);
	auto pCall = pCallAddr + *(int32_t *)(pCallAddr + 1) + 5;
	plua_pcall2 = (lua_pcall_t)pCall;
	if (!plua_pcall2) {
		return this->m_mainScriptMachineReady;
	}

	plua_tostring = (lua_tostring_t)GetPointerFromPattern("lua_tostring", "81 FA ? ? ? ? 7E ? 85 D2 7E ? 48 8B 41 ? 48 63 D2 48 83 C0 ? 48 C1 E2 ? 48 03 D0 48 3B 51 ? 73 ? 45 33 C0");
	logPointer("lua_tostring", (uintptr_t)plua_tostring);
	if (!plua_tostring)
	{
		return this->m_mainScriptMachineReady;
	}

	auto isStringAddr = GetPointerFromPattern("lua_isstring", "E8 ? ? ? ? 85 C0 74 ? BA ? ? ? ? 48 8B CF E8 ? ? ? ? 48 8B D0");
	logPointer("lua_isstring", isStringAddr);
	auto isString = isStringAddr + *(int32_t *)(isStringAddr + 1) + 5;
	plua_isstring = (lua_isstring_t)isString;
	if (!plua_isstring) {
		return this->m_mainScriptMachineReady;
	}

	//
	auto loadBufferAddr = GetPointerFromPattern("lua_loadbuffer", "E8 ? ? ? ? 8B D8 85 C0 75 ? 45 33 C9");
	if (!loadBufferAddr)
	{
		return this->m_mainScriptMachineReady;
	}
	auto loadBuffer = loadBufferAddr + *(int32_t *)(loadBufferAddr + 1) + 5;
	logPointer("lua_loadBuffer", loadBuffer);
	pluaL_loadbuffer = (luaL_loadbuffer_t)loadBuffer;

	//
	plua_newthread = (lua_newthread_t)GetPointerFromPattern("lua_newthread", "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 48 8B 79 ? 48 8B F1 44 8B B9");
	logPointer("lua_newthread", (uintptr_t)plua_newthread);
	if (!plua_newthread) {
		return this->m_mainScriptMachineReady;
	}

	//
	plua_pushcclosure = (lua_pushcclosure_t)GetPointerFromPattern("plua_pushcclosure", "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 83 EC ? 49 63 F0");
	logPointer("lua_pushcclosure", (uintptr_t)plua_pushcclosure);
	if (!plua_pushcclosure) {
		return this->m_mainScriptMachineReady;
	}

	//
	auto setFieldAddr = GetPointerFromPattern("lua_setfield", "E8 ? ? ? ? BE ? ? ? ? 81 BF");
	if (!setFieldAddr)
	{
		return this->m_mainScriptMachineReady;
	}
	auto setField = setFieldAddr + *(int32_t *)(setFieldAddr + 1) + 5;
	logPointer("lua_setfield", setField);
	plua_setfield = (lua_setfield_t)setField;

	//
	auto setGobalAddr = GetPointerFromPattern("lua_setglobal", "E8 ? ? ? ? 48 8B 54 24 ? 48 8B CE E8 ? ? ? ? 85 C0");
	if (!setGobalAddr)
	{
		return this->m_mainScriptMachineReady;
	}
	auto setGlobal = setGobalAddr + *(int32_t *)(setGobalAddr + 1) + 5;
	logPointer("lua_setglobal", setGlobal);
	plua_setglobal = (lua_setglobal_t)setGlobal;

	/*static auto pat = hooking::pattern("48 8B 05 ? ? ? ? 48 89 44 24 ? 4C 8B 4C 24").get(0).origaddr();
	auto patAddr = pat + *(int32_t*)(pat + 3) + 7;
	patAddr = *(uint64_t*)patAddr;
	if (patAddr)
	{
		logPointer("patAddr", patAddr);
		DWORD old_protect;
		VirtualProtect((PVOID)patAddr, strlen("/gui/main_menu_dev.swf"), PAGE_EXECUTE_READWRITE, (PDWORD)&old_protect);
		memcpy((void*)patAddr, "/gui/main_menu_dev.swf", strlen("/gui/main_menu_dev.swf"));
		DWORD unused;
		VirtualProtect((PVOID)patAddr, strlen("/gui/main_menu_dev.swf"), old_protect, &unused);
	}*/

	//
	this->m_mainScriptMachineReady = true;

	M3ScriptHook::instance()->Log(__FUNCTION__ " Finished");
	return m_mainScriptMachineReady;
	/*
	static auto addr = hooking::pattern("F3 0F 10 0D ? ? ? ? 4C 8D 44 24 ? F3 48 0F 2C D0").get(0).origaddr();
	static auto inc = hooking::inject_call<uintptr_t, lua_State*, int32_t>((addr + 0x62));
	inc.inject([](lua_State *L, int32_t a2) {
	const char *test;
	if (plua_isstring(L, 1))
	{
	test = plua_tostring(L, 1);
	}

	M3ScriptHook::instance()->Log("Entity: %s", test);

	return inc.call(L, a2);
	});*/
}

bool LuaFunctions::Setup()
{
	M3ScriptHook::instance()->Log(__FUNCTION__);

	static auto SetupGlobalLuaFunction = [](lua_State *L, const char *n, lua_CFunction f) {
		M3ScriptHook::instance()->Log(__FUNCTION__);
		plua_pushcclosure(L, f, 0, 0);
		plua_setglobal(L, n);
		//plua_setfield(L, -10002, (n));
	};

	auto L = GetL();
	SetupGlobalLuaFunction(L, "printToLog", LuaFunctions::PrintToLog);
	SetupGlobalLuaFunction(L, "bindKey", LuaFunctions::BindKey);
	SetupGlobalLuaFunction(L, "unbindKey", LuaFunctions::UnbindKey);
	SetupGlobalLuaFunction(L, "setTimeout", LuaFunctions::DelayBuffer);
	SetupGlobalLuaFunction(L, "fnv32", LuaFunctions::FNV32a);
	
	return true;
}