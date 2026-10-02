#define lj_sourcenet_c
#define LUA_CORE

#include "lj_obj.h"
#include "lj_err.h"
#include "lj_frame.h"
#include "lj_dispatch.h"
#include "lj_vm.h"
#include "lua.h"
#include "lauxlib.h"

static int sn_cwrap(lua_State *L, lua_CFunction f)
{
  int n = f(L);

  if (LJ_LIKELY(n >= -1))
    return n;  

  if (n == LUA_SN_ERROR)
    return lua_error(L);

  if (n == LUA_SN_ERRORMSG)
    return luaL_error(L, "%s", lua_tostring(L, -1));

  if (n == LUA_SN_ARGERROR || n == LUA_SN_TYPEERROR) {
    int narg = (int)lua_tointeger(L, -2);
    const char *s;
    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, "_SN_ERRMSG");
    s = lua_tostring(L, -1);
    lua_pop(L, 2);
    if (n == LUA_SN_ARGERROR)
      return luaL_argerror(L, narg, s);
    return luaL_typerror(L, narg, s);
  }

  if (n <= LUA_SN_YIELD(0))
    return lua_yield(L, LUA_SN_YIELD(0) - n);
  
  lj_err_throw(L, LUA_SN_RETHROW(0) - n);
  return 0;
}

LUA_API int lua_sn_init(lua_State *L)
{
  global_State *g = G(L);
  g->wrapf = (lua_CFunction)sn_cwrap;
  setbc_op(&g->bc_cfunc_ext, BC_FUNCCW);
  return 1;
}

typedef struct SNOp {
  int op;
  int idx;
  int n;
  const char *k;
  int result;
} SNOp;

static TValue *sn_opcp(lua_State *L, lua_CFunction dummy, void *ud)
{
  SNOp *o = (SNOp *)ud;
  UNUSED(dummy);
  cframe_errfunc(L->cframe) = -1; 
  switch (o->op) {
  case LUA_SN_GETTABLE: lua_gettable(L, o->idx); break;
  case LUA_SN_SETTABLE: lua_settable(L, o->idx); break;
  case LUA_SN_GETFIELD: lua_getfield(L, o->idx, o->k); break;
  case LUA_SN_SETFIELD: lua_setfield(L, o->idx, o->k); break;
  case LUA_SN_RAWSET: lua_rawset(L, o->idx); break;
  case LUA_SN_EQUAL: o->result = lua_equal(L, o->idx, o->n); break;
  case LUA_SN_LESSTHAN: o->result = lua_lessthan(L, o->idx, o->n); break;
  case LUA_SN_CONCAT: lua_concat(L, o->n); break;
  case LUA_SN_NEXT: o->result = lua_next(L, o->idx); break;
  default: break;
  }
  return NULL;
}

LUA_API int lua_sn_op(lua_State *L, int op, int idx, int n, const char *k,
		      int *result)
{
  global_State *g = G(L);
  uint8_t oldh = hook_save(g);
  SNOp o;
  int status;
  if (idx < 0 && idx > LUA_REGISTRYINDEX)
    idx = (int)(L->top - L->base) + idx + 1;
  if (n < 0 && n > LUA_REGISTRYINDEX && op != LUA_SN_CONCAT)
    n = (int)(L->top - L->base) + n + 1;
  o.op = op; o.idx = idx; o.n = n; o.k = k; o.result = 0;
  status = lj_vm_cpcall(L, NULL, &o, sn_opcp);
  if (status) hook_restore(g, oldh);
  if (result) *result = o.result;
  return status;
}

LUA_API int lua_sn_call(lua_State *L, int nargs, int nresults)
{
  global_State *g = G(L);
  uint8_t oldh = hook_save(g);
  TValue *base;
  int status;
#if LJ_FR2
  TValue *o = L->top;
  base = o - nargs;
  L->top = o+1;
  for (; o > base; o--) copyTV(L, o, o-1);
  setnilV(o);
  base = o+1;
#else
  base = L->top - nargs;
#endif
  status = lj_vm_pcall(L, base, nresults+1, -1);
  if (status) hook_restore(g, oldh);
  return status;
}
