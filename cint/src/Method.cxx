/* /% C++ %/ */
/***********************************************************************
 * cint (C/C++ interpreter)
 ************************************************************************
 * Source file Method.cxx
 ************************************************************************
 * Description:
 *  Extended Run Time Type Identification API
 ************************************************************************
 * Author                  Masaharu Goto 
 * Copyright(c) 1995~2005  Masaharu Goto 
 *
 * For the licensing terms see the file COPYING
 *
 ************************************************************************/

#include "Api.h"
#include "common.h"

extern "C" int G__xrefflag;

/*********************************************************************
* class G__MethodInfo
*
* 
*********************************************************************/
///////////////////////////////////////////////////////////////////////////
void Cint::G__MethodInfo::Init()
{
  handle = (long)(G__get_ifunc_ref(&G__ifunc));
  index = -1;
#ifndef G__OLDIMPLEMENTATION2194
  usingIndex = -1;
#endif
  belongingclass=(G__ClassInfo*)NULL;
}
///////////////////////////////////////////////////////////////////////////
void Cint::G__MethodInfo::Init(G__ClassInfo &a)
{
  if(a.IsValid()) {
    handle=(long)G__get_ifunc_ref(G__struct.memfunc[a.Tagnum()]);
    index = -1;
#ifndef G__OLDIMPLEMENTATION2194
    usingIndex = -1;
#endif
    belongingclass = &a;
    G__incsetup_memfunc((int)a.Tagnum());
  }
  else {
    handle=0;
    index = -1;
#ifndef G__OLDIMPLEMENTATION2194
    usingIndex = -1;
#endif
    belongingclass=(G__ClassInfo*)NULL;
  }
}
///////////////////////////////////////////////////////////////////////////
void Cint::G__MethodInfo::Init(long handlein,long indexin
	,G__ClassInfo *belongingclassin)
{
#ifndef G__OLDIMPLEMENTATION2194
  usingIndex = -1;
#endif
  if(handlein) {
    handle = handlein;
    index = indexin;
    if(belongingclassin && belongingclassin->IsValid()) 
      belongingclass = belongingclassin;
    else {
      belongingclass=(G__ClassInfo*)NULL;
    }

    /* Set return type */
    struct G__ifunc_table_internal *ifunc;
    ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
    type.type=ifunc->type[index];
    type.tagnum=ifunc->p_tagtable[index];
    type.typenum=ifunc->p_typetable[index];
    type.reftype=ifunc->reftype[index];
    type.isconst=ifunc->isconst[index];
    type.class_property=0;
  }
  else { /* initialize if handlein==0 */
    handle=0;
    index=-1;
    belongingclass=(G__ClassInfo*)NULL;
  }
}
///////////////////////////////////////////////////////////////////////////
void Cint::G__MethodInfo::Init(G__ClassInfo *belongingclassin
	,long funcpage,long indexin)
{
  struct G__ifunc_table_internal *ifunc;
  int i=0;

  if(belongingclassin->IsValid()) {
    // member function
    belongingclass = belongingclassin;
    ifunc = G__struct.memfunc[belongingclassin->Tagnum()];
  }
  else {
    // global function
    belongingclass=(G__ClassInfo*)NULL;
    ifunc = G__p_ifunc;
  }

  // reach to desired page
  for(i=0;i<funcpage&&ifunc;i++) ifunc=ifunc->next;
  G__ASSERT(ifunc->page == funcpage);

  if(ifunc) {
    handle = (long)G__get_ifunc_ref(ifunc);
    index = indexin;
    // Set return type
    type.type=ifunc->type[index];
    type.tagnum=ifunc->p_tagtable[index];
    type.typenum=ifunc->p_typetable[index];
    type.reftype=ifunc->reftype[index];
    type.isconst=ifunc->isconst[index];
    type.class_property=0;
  }
  else { /* initialize if handlein==0 */
    handle=0;
    index=-1;
    belongingclass=(G__ClassInfo*)NULL;
  }
}
///////////////////////////////////////////////////////////////////////////
const char* Cint::G__MethodInfo::Name()
{
  if(IsValid()) {
    struct G__ifunc_table_internal *ifunc;
    ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
    return(ifunc->funcname[index]);
  }
  else {
    return((char*)NULL);
  }
}
///////////////////////////////////////////////////////////////////////////
int Cint::G__MethodInfo::Hash()
{
  if(IsValid()) {
    struct G__ifunc_table_internal *ifunc;
    ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
    return(ifunc->hash[index]);
  }
  else {
    return(0);
  }
}
///////////////////////////////////////////////////////////////////////////
struct G__ifunc_table* Cint::G__MethodInfo::ifunc()
{
  if(IsValid()) {
    return (struct G__ifunc_table*)handle;
  }
  else {
    return((struct G__ifunc_table*)NULL);
  }
}
///////////////////////////////////////////////////////////////////////////
const char* Cint::G__MethodInfo::Title() 
{
  static char buf[G__INFO_TITLELEN];
  buf[0]='\0';
  if(IsValid()) {
    struct G__ifunc_table_internal *ifunc;
    ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
    G__getcomment(buf,&ifunc->comment[index],ifunc->tagnum);
    return(buf);
  }
  else {
    return((char*)NULL);
  }
}
///////////////////////////////////////////////////////////////////////////
long Cint::G__MethodInfo::Property()
{
  if(IsValid()) {
    long property=0;
    struct G__ifunc_table_internal *ifunc;
    ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
    if (ifunc->hash[index]==0) return property;
    switch(ifunc->access[index]) {
    case G__PUBLIC: property|=G__BIT_ISPUBLIC; break;
    case G__PROTECTED: property|=G__BIT_ISPROTECTED; break;
    case G__PRIVATE: property|=G__BIT_ISPRIVATE; break;
    }
    if(ifunc->isconst[index]&G__CONSTFUNC) property|=G__BIT_ISCONSTANT | G__BIT_ISMETHCONSTANT; 
    if(ifunc->isconst[index]&G__CONSTVAR) property|=G__BIT_ISCONSTANT;
    if(ifunc->isconst[index]&G__PCONSTVAR) property|=G__BIT_ISPCONSTANT;
    if(isupper(ifunc->type[index])) property|=G__BIT_ISPOINTER;
    if(ifunc->staticalloc[index]) property|=G__BIT_ISSTATIC;
    if(ifunc->isvirtual[index]) property|=G__BIT_ISVIRTUAL;
    if(ifunc->ispurevirtual[index]) property|=G__BIT_ISPUREVIRTUAL;
    if(ifunc->pentry[index]->size<0) property|=G__BIT_ISCOMPILED;
    if(ifunc->pentry[index]->bytecode) property|=G__BIT_ISBYTECODE;
    if(ifunc->isexplicit[index]) property|=G__BIT_ISEXPLICIT;
    return(property);
  }
  else {
    return(0);
  }
}
///////////////////////////////////////////////////////////////////////////
int Cint::G__MethodInfo::NArg()
{
  if(IsValid()) {
    struct G__ifunc_table_internal *ifunc;
    ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
    return(ifunc->para_nu[index]);
  }
  else {
    return(-1);
  }
}
///////////////////////////////////////////////////////////////////////////
int Cint::G__MethodInfo::NDefaultArg()
{
  if(IsValid()) {
    struct G__ifunc_table_internal *ifunc;
    ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
    if(ifunc->para_nu[index]) {
      int i,defaultnu=0;
      for(i=ifunc->para_nu[index]-1;i>=0;i--) {
	     if(ifunc->param[index][i]->pdefault) ++defaultnu;
	     else return(defaultnu);
      }
      return(defaultnu);
    }
    else {
      return(0);
    }
  }
  return(-1);
}
///////////////////////////////////////////////////////////////////////////
int Cint::G__MethodInfo::HasVarArgs()
{
  if(IsValid()) {
    struct G__ifunc_table_internal *ifunc;
    ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
    return(2==ifunc->ansi[index]?1:0);
  }
  else {
    return(-1);
  }
}
///////////////////////////////////////////////////////////////////////////
G__InterfaceMethod Cint::G__MethodInfo::InterfaceMethod()
{
  G__LockCriticalSection();
  if(IsValid()) {
    struct G__ifunc_table_internal *ifunc;
    ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
    if(
       -1==ifunc->pentry[index]->size /* this means compiled class */
       ) {
      G__UnlockCriticalSection();

      // 25-05-07
      // We have a problem here (are we ever going to stop having them ;))
      // I discovered the first time I removed the stubs from dictionaries.
      // The thing is that if we get here it has to be a compiled class
      // but if we are using the new algorithm then the InterfaceMethod
      // is 0 (because we used our registered method in funcptr)
      // so now we have to see how to deal with that situation in
      // TCint::GetInterfaceMethodWithPrototype
      //
      // This happens only in Qt (TQObject) 
      // static TMethod *GetMethod(TClass *cl, const char *method, const char *params)
      // and I have seen that the address is not really used
      // they only want to know if the method can be executed...
      // this is extreamly shady but for the moment just pass the funcptr
      // if the interface method is zero
      if((G__InterfaceMethod)ifunc->pentry[index]->p)
         return((G__InterfaceMethod)ifunc->pentry[index]->p);
      else 
         return((G__InterfaceMethod)ifunc->funcptr);
    }
    else {
      G__UnlockCriticalSection();
      return((G__InterfaceMethod)NULL);
    }
  }
  else {
    G__UnlockCriticalSection();
    return((G__InterfaceMethod)NULL);
  }
}
#ifdef G__ASM_WHOLEFUNC
///////////////////////////////////////////////////////////////////////////
struct G__bytecodefunc *Cint::G__MethodInfo::GetBytecode()
{
  if(IsValid()) {
    struct G__ifunc_table_internal *ifunc;
    ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
    int store_asm_loopcompile = G__asm_loopcompile;
    G__asm_loopcompile = 4;
    if(!ifunc->pentry[index]->bytecode &&
       -1!=ifunc->pentry[index]->size && 
       G__BYTECODE_NOTYET==ifunc->pentry[index]->bytecodestatus
       && G__asm_loopcompile>=4
       ) {
      G__compile_bytecode((struct G__ifunc_table*)handle,(int)index);
    }
    G__asm_loopcompile = store_asm_loopcompile;
    return(ifunc->pentry[index]->bytecode);
  }
  else {
    return((struct G__bytecodefunc*)NULL);
  }
}
#endif
/* #ifndef G__OLDIMPLEMENTATION1163 */
///////////////////////////////////////////////////////////////////////////
G__DataMemberInfo Cint::G__MethodInfo::GetLocalVariable()
{
  G__DataMemberInfo localvar;
  localvar.Init((long)0,(long)(-1),(G__ClassInfo*)NULL);
  if(IsValid()) {
    int store_fixedscope=G__fixedscope;
    G__xrefflag=1;
    G__fixedscope=1;
    struct G__bytecodefunc* pbc = GetBytecode();
    G__xrefflag=0;
    G__fixedscope=store_fixedscope;
    if(!pbc) {
      if(Property()&G__BIT_ISCOMPILED) {
	G__fprinterr(G__serr,"Limitation: can not get local variable information for compiled function %s\n",Name());
      }
      else {
	G__fprinterr(G__serr,"Limitation: function %s , failed to get local variable information\n",Name());
      }
      return(localvar);
    }
    localvar.Init((long)pbc->var,(long)(-1),(G__ClassInfo*)NULL);
    return(localvar);
  }
  else {
    return(localvar);
  }
}
/* #endif */
///////////////////////////////////////////////////////////////////////////
#ifdef G__TRUEP2F
void* Cint::G__MethodInfo::PointerToFunc()
{
  if(IsValid()) {
    struct G__ifunc_table_internal *ifunc;
    ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
    if(
       -1!=ifunc->pentry[index]->size && 
       G__BYTECODE_NOTYET==ifunc->pentry[index]->bytecodestatus
       && G__asm_loopcompile>=4
       ) {
      G__compile_bytecode((struct G__ifunc_table*)handle,(int)index);
    }
    if(G__BYTECODE_SUCCESS==ifunc->pentry[index]->bytecodestatus) 
      return((void*)ifunc->pentry[index]->bytecode);
      
    return(ifunc->pentry[index]->tp2f);
  }
  else {
    return((void*)NULL);
  }
}
#endif
///////////////////////////////////////////////////////////////////////////
void Cint::G__MethodInfo::SetGlobalcomp(int globalcomp)
{
  if(IsValid()) {
    struct G__ifunc_table_internal *ifunc;
    ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
    ifunc->globalcomp[index]=globalcomp;
    if(G__NOLINK==globalcomp) ifunc->access[index]=G__PRIVATE;
    else                      ifunc->access[index]=G__PUBLIC;
  }
}
///////////////////////////////////////////////////////////////////////////
int Cint::G__MethodInfo::IsValid()
{
  if(handle) {
    struct G__ifunc_table_internal *ifunc;
    ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
    if(ifunc && 0<=index&&index<ifunc->allifunc) {
      return(1);
    }
    else {
      return(0);
    }
  }
  else {
    return(0);
  }
}
///////////////////////////////////////////////////////////////////////////
int Cint::G__MethodInfo::SetFilePos(const char* fname)
{
  struct G__dictposition* dict=G__get_dictpos((char*)fname);
  if(!dict) return(0);
  handle = (long)dict->ifunc;
  index = (long)(dict->ifn-1);
  belongingclass=(G__ClassInfo*)NULL;
  return(1);
}
///////////////////////////////////////////////////////////////////////////
int Cint::G__MethodInfo::Next()
{
  if(handle) {
    struct G__ifunc_table_internal *ifunc;
    ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
    ++index;
    if(ifunc->allifunc<=index) {
      int t = ifunc->tagnum;
      ifunc=ifunc->next;
      if(ifunc) {
	ifunc->tagnum=t;
	handle=(long)G__get_ifunc_ref(ifunc);
	index = 0;
      }
      else {
	handle=0;
	index = -1;
      }
    } 
#ifndef G__OLDIMPLEMENTATION2194
    if(ifunc==0 && belongingclass==0 && 
       usingIndex<G__globalusingnamespace.basen) {
      ++usingIndex;
      index=0;
      G__incsetup_memfunc(G__globalusingnamespace.herit[usingIndex]->basetagnum);
      ifunc=G__struct.memfunc[G__globalusingnamespace.herit[usingIndex]->basetagnum];
      handle=(long)G__get_ifunc_ref(ifunc);
    }
#endif
    if(IsValid()) {
      type.type=ifunc->type[index];
      type.tagnum=ifunc->p_tagtable[index];
      type.typenum=ifunc->p_typetable[index];
      type.reftype=ifunc->reftype[index];
      type.isconst=ifunc->isconst[index];
      type.class_property=0;
      return(1);
    }
    else {
      return(0);
    }
  }
  else {
    return(0);
  }
}
///////////////////////////////////////////////////////////////////////////
const char* Cint::G__MethodInfo::FileName()
{
  if(IsValid()) {
    struct G__ifunc_table_internal *ifunc;
    ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
    if(ifunc->pentry[index]->filenum>=0) { /* 2012, keep this */
      return(G__srcfile[ifunc->pentry[index]->filenum].filename);
    }
    else {
      return("(compiled)");
    }
  }
  else {
    return((char*)NULL);
  }
}
///////////////////////////////////////////////////////////////////////////
FILE* Cint::G__MethodInfo::FilePointer()
{
  if(IsValid()) {
    struct G__ifunc_table_internal *ifunc;
    ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
    if(
       ifunc->pentry[index]->filenum>=0 && ifunc->pentry[index]->size>=0
       ) {
      return(G__srcfile[ifunc->pentry[index]->filenum].fp);
    }
    else {
      return((FILE*)NULL);
    }
  }
  else {
    return((FILE*)NULL);
  }
}
///////////////////////////////////////////////////////////////////////////
int Cint::G__MethodInfo::LineNumber()
{
  if(IsValid()) {
    struct G__ifunc_table_internal *ifunc;
    ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
    if(
       ifunc->pentry[index]->filenum>=0 && ifunc->pentry[index]->size>=0
       ) {
      return(ifunc->pentry[index]->line_number);
    }
    else {
      return(0);
    }
  }
  else {
    return(-1);
  }
}
///////////////////////////////////////////////////////////////////////////
long Cint::G__MethodInfo::FilePosition()
{ 
  // returns  'type fname(type p1,type p2)'
  //                      ^
  long invalid=0L;
  if(IsValid()) {
#ifdef G__VMS
     G__fprinterr(G__err, 
                  "Error: VMS support now broken; please complain to cint@pcroot.cern.ch if this matters to you!\n");
    //Changed so that pos can be a long.
    struct G__ifunc_table_VMS *ifunc;
    ifunc = (struct G__ifunc_table_VMS*)handle;
#else
    struct G__ifunc_table_internal *ifunc;
    ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
#endif
    if(
       ifunc->pentry[index]->filenum>=0 && ifunc->pentry[index]->size>=0
       ) {
#if defined(G__NONSCALARFPOS2)
      return((long)ifunc->pentry[index]->pos.__pos);
#elif defined(G__NONSCALARFPOS_QNX)      
      return((long)ifunc->pentry[index]->pos._Off);
#else
      return((long)ifunc->pentry[index]->pos);
#endif
    }
    else {
      return(invalid);
    }
  }
  else {
    return(invalid);
  }
}
///////////////////////////////////////////////////////////////////////////
int Cint::G__MethodInfo::Size()
{
  if(IsValid()) {
    struct G__ifunc_table_internal *ifunc;
    ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
    if(
       ifunc->pentry[index]->size>=0
       ) {
      return(ifunc->pentry[index]->size);
    }
    else {
      return(0);
    }
  }
  else {
    return(-1);
  }
}
///////////////////////////////////////////////////////////////////////////
int Cint::G__MethodInfo::IsBusy()
{
  if(IsValid()) {
    struct G__ifunc_table_internal *ifunc;
    ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
    return(ifunc->busy[index]);
  }
  else {
    return(-1);
  }
}
///////////////////////////////////////////////////////////////////////////
static char G__buf[G__LONGLINE];
char* Cint::G__MethodInfo::GetPrototype()
{
  if (!IsValid()) return 0;
  strcpy(G__buf,Type()->Name());
  strcat(G__buf," ");
  if(belongingclass && belongingclass->IsValid()) {
    strcat(G__buf,belongingclass->Name());
    strcat(G__buf,"::");
  }
  strcat(G__buf,Name());
  strcat(G__buf,"(");
  G__MethodArgInfo arg(*this);
  int flag=0;
  while(arg.Next()) {
    if(flag) strcat(G__buf,",");
    flag=1;
    strcat(G__buf,arg.Type()->Name());
    strcat(G__buf," ");
    if(arg.Name()) strcat(G__buf,arg.Name());
    if(arg.DefaultValue()) {
      strcat(G__buf,"=");
      strcat(G__buf,arg.DefaultValue());
    }
  }
  strcat(G__buf,")");
  return(G__buf);
}
///////////////////////////////////////////////////////////////////////////
char* Cint::G__MethodInfo::GetMangledName()
{
  if (!IsValid()) return 0;
  return(G__map_cpp_name(GetPrototype()));
}
///////////////////////////////////////////////////////////////////////////
extern "C" int G__DLL_direct_globalfunc(G__value *result7
					,G__CONST char *funcname
					,struct G__param *libp,int hash) ;
extern "C" void* G__FindSym(const char* filename,const char* funcname);
int Cint::G__MethodInfo::LoadDLLDirect(const char* filename,const char* funcname) 
{
  void* p2f;
  struct G__ifunc_table_internal *ifunc;
  ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
  p2f = G__FindSym(filename,funcname);
  if(p2f) {
    ifunc->pentry[index]->tp2f = p2f;
    ifunc->pentry[index]->p = (void*)G__DLL_direct_globalfunc;
    ifunc->pentry[index]->size = -1;
    //ifunc->pentry[index]->filenum = -1; /* not good */
    ifunc->pentry[index]->line_number = -1;
    return 1;
  }
  return 0;
}
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// Global function to set precompiled library linkage
///////////////////////////////////////////////////////////////////////////
int Cint::G__SetGlobalcomp(char *funcname,char *param,int globalcomp)
{
  G__ClassInfo globalscope;
  G__MethodInfo method;
  long dummy=0;
  char classname[G__LONGLINE];

  // Actually find the last :: to get the full classname, including
  // namespace and/or containing classes.
  strcpy(classname,funcname);
  char *fname = 0;
  char * tmp = classname;
  while ( (tmp = strstr(tmp,"::")) ) {
    fname = tmp;
    tmp += 2;
  }
  if(fname) {
    *fname=0;
    fname+=2;
    globalscope.Init(classname);
  }
  else {
    fname = funcname;
  }

  if(strcmp(fname,"*")==0) {
    method.Init(globalscope);
    while(method.Next()) {
      method.SetGlobalcomp(globalcomp);
    }
    return(0);
  }
  method=globalscope.GetMethod(fname,param,&dummy);

  if(method.IsValid()) {
    method.SetGlobalcomp(globalcomp);
    return(0);
  }
  else {
    G__fprinterr(G__serr,"Warning: #pragma link, function %s(%s) not found"
	    ,fname,param);
    G__printlinenum();
    return(1);
  }
}
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// Global function to set precompiled library linkage
///////////////////////////////////////////////////////////////////////////
int Cint::G__ForceBytecodecompilation(char *funcname,char *param)
{
  G__ClassInfo globalscope;
  G__MethodInfo method;
  long dummy=0;
  char classname[G__LONGLINE];

  // Actually find the last :: to get the full classname, including
  // namespace and/or containing classes.
  strcpy(classname,funcname);
  char *fname = 0;
  char * tmp = classname;
  while ( (tmp = strstr(tmp,"::")) ) {
    fname = tmp;
    tmp += 2;
  }
  if(fname) {
    *fname=0;
    fname+=2;
    globalscope.Init(classname);
  }
  else {
    fname = funcname;
  }

  method=globalscope.GetMethod(fname,param,&dummy);

  if(method.IsValid()) {
    struct G__ifunc_table *ifunc = method.ifunc();
    int ifn = method.Index();
    int stat;
    int store_asm_loopcompile = G__asm_loopcompile;
    int store_asm_loopcompile_mode = G__asm_loopcompile_mode;
    G__asm_loopcompile_mode=G__asm_loopcompile=4;
    stat = G__compile_bytecode(ifunc,ifn);
    G__asm_loopcompile=store_asm_loopcompile;
    G__asm_loopcompile_mode=store_asm_loopcompile_mode;
    if(stat) return 0;
    else return 1;
  }
  else {
    G__fprinterr(G__serr,"Warning: function %s(%s) not found"
	    ,fname,param);
    G__printlinenum();
    return(1);
  }
}

///////////////////////////////////////////////////////////////////////////
// SetVtblIndex
///////////////////////////////////////////////////////////////////////////
void Cint::G__MethodInfo::SetVtblIndex(int vtblindex) {
  if(!IsValid()) return;
  struct G__ifunc_table_internal* ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
  ifunc->vtblindex[index] = (short)vtblindex;
}

///////////////////////////////////////////////////////////////////////////
// SetIsVirtual
///////////////////////////////////////////////////////////////////////////
void Cint::G__MethodInfo::SetIsVirtual(int isvirtual) {
  if(!IsValid()) return;
  struct G__ifunc_table_internal* ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
  ifunc->isvirtual[index] = isvirtual;
}

///////////////////////////////////////////////////////////////////////////
// SetVtblBasetagnum
///////////////////////////////////////////////////////////////////////////
void Cint::G__MethodInfo::SetVtblBasetagnum(int basetagnum) {
  if(!IsValid()) return;
  struct G__ifunc_table_internal* ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
  ifunc->vtblbasetagnum[index] = (short)basetagnum;
}

///////////////////////////////////////////////////////////////////////////
// GetFriendInfo
///////////////////////////////////////////////////////////////////////////
G__friendtag*  Cint::G__MethodInfo::GetFriendInfo() { 
   if(IsValid()) {
      struct G__ifunc_table_internal* ifunc = G__get_ifunc_internal((struct G__ifunc_table*)handle);
      return(ifunc->friendtag[index]);
   }
   else return 0;
}
///////////////////////////////////////////////////////////////////////////
// GetDefiningScopeTagnum
///////////////////////////////////////////////////////////////////////////
int Cint::G__MethodInfo::GetDefiningScopeTagnum()
{
   if (IsValid()) {
      return ifunc()->tagnum;
   } 
   else return -1;
}
///////////////////////////////////////////////////////////////////////////
// SetUserParam
///////////////////////////////////////////////////////////////////////////
void Cint::G__MethodInfo::SetUserParam(void *user) 
{
   if (IsValid()) {
      struct G__ifunc_table_internal* ifunc_internal = G__get_ifunc_internal((struct G__ifunc_table*)ifunc());
      ifunc_internal->userparam[index] = user;
   }
}
///////////////////////////////////////////////////////////////////////////
// GetUserParam
///////////////////////////////////////////////////////////////////////////
void *Cint::G__MethodInfo::GetUserParam()
{
   if (IsValid()) {
      struct G__ifunc_table_internal* ifunc_internal = G__get_ifunc_internal((struct G__ifunc_table*)ifunc());
      return ifunc_internal->userparam[index];
   }
   else return 0;
}
///////////////////////////////////////////////////////////////////////////
// GetThisPointerOffset 
// Return: Return the this-pointer offset, to adjust it in case of non left-most
// multiple inheritance
///////////////////////////////////////////////////////////////////////////
long Cint::G__MethodInfo::GetThisPointerOffset()
{
   if (IsValid()) {
      struct G__ifunc_table_internal* ifunc_internal = G__get_ifunc_internal((struct G__ifunc_table*)ifunc());
      return ifunc_internal->entry[0].ptradjust;
   }
   else return 0;
}
