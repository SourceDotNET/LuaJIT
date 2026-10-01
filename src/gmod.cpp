extern "C"
{
	#include "gmod.h"
	#include "lua.h"
}

extern "C" void lua_init_stack_gmod(lua_State* L1, lua_State* L)
{
    if (L && L != L1)
	{
		L1->luabase = L->luabase;
		if (L->luabase)
			((ILuaBase*)L->luabase)->SetState(L);
	}
}

// RaphaelIT7: We don't use this, rather we backported lua_setoutputf from my REngine fork
/*extern "C" void GMOD_LuaPrint(const char* str, lua_State* L) // Should be how gmod does it
{
	if (!L->luabase) // except for this, gmod doesn't do this, but we do making testing jit less of a pain
	{
		printf(str);
		return;
	}

	((ILuaInterface*)L->luabase)->Msg("%s", str);
}*/

struct UserData
{
	void* data;
	unsigned char type;
};

extern "C" void GMOD_LuaCreateEmptyUserdata(lua_State* L)
{
	UserData* pData = (UserData*)lua_newuserdata(L, sizeof(UserData));
	pData->data = nullptr;
	pData->type = 7;

	// RaphaelIT7:
	// GMod would actually call this but we can just be simpler as we do not have a vtable!
	// ((ILuaBase*)L->luabase)->PushUserType(NULL, 7);
}