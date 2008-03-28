/* /% C %/ */
/***********************************************************************
 * cint (C/C++ interpreter)
 ************************************************************************
 * Source file new.c
 ************************************************************************
 * Description:
 *  new delete
 ************************************************************************
 * Copyright(c) 1995~2003  Masaharu Goto 
 *
 * For the licensing terms see the file COPYING
 *
 ************************************************************************/

#include "common.h"
#include "Dict.h"

using namespace Cint::Internal;

static void G__lock_noop() {}
static void(*G__AllocMutexLock)()   = G__lock_noop;
static void(*G__AllocMutexUnLock)() = G__lock_noop;

extern "C" void G__exec_alloc_lock()   { G__AllocMutexLock(); }
extern "C" void G__exec_alloc_unlock() { G__AllocMutexUnLock(); }

extern "C" void G__set_alloclockfunc(void(*foo)())
{ G__AllocMutexLock = foo; }
extern "C" void G__set_allocunlockfunc(void(*foo)())
{ G__AllocMutexUnLock = foo; }

/****************************************************************
* G__value G__new_operator()
* 
* Called by:
*   G__getpower()
*
*      V
*  new type
*  new type[10]
*  new type(53)
*  new (arena)type
*
****************************************************************/
G__value Cint::Internal::G__new_operator(char *expression)
{
   char arena[G__ONELINE];
   char *memarena = 0;
   int arenaflag = 0;
   char construct[G__LONGLINE];
   char *type;
   char *basictype;
   char *initializer;
   char *arrayindex;
   int p = 0;
   size_t pinc;
   int size;
   int known;
   char *pointer = 0;
   char *store_struct_offset; /* used to be int */
   ::Reflex::Scope store_tagnum;
   ::Reflex::Type store_typenum;
   int var_type = 0;
   G__value result;
   int reftype = G__PARANORMAL;
   int typelen;
   int ispointer = 0;
   int tagnum;
   ::Reflex::Type typenum;
   int ld_flag = 0 ;

   G__CHECK(G__SECURE_MALLOC, 1, return(G__null));

   /******************************************************
    * get arena which is ignored due to limitation, however
    ******************************************************/
   if (expression[0] == '(') {
      G__getstream(expression + 1, &p, arena, ")");
      ++p;
      memarena = (char*)G__int(G__getexpr(arena));
      arenaflag = 1;
#ifdef G__ASM
      if (G__asm_noverflow) {
         G__asm_inst[G__asm_cp] = G__SETGVP;
         G__asm_inst[G__asm_cp+1] = 0;
         G__inc_cp_asm(2, 0);
#ifdef G__ASM_DBG
         if (G__asm_dbg) G__fprinterr(G__serr, "%3x: SETGVP 0\n", G__asm_cp);
#endif
         // --
      }
#endif
      // --
   }
   else {
      arena[0] = '\0';
   }

   /******************************************************
    * get initializer,arrayindex,type,pinc and size
    ******************************************************/
   type = expression + p;
   initializer = strchr(type, '(');
   arrayindex = strchr(type, '[');
   /* initializer and arrayindex are exclusive */
   if (initializer != NULL && arrayindex != NULL) {
      if (initializer < arrayindex) {
         arrayindex = NULL;
      }
      else {
         initializer = NULL;
      }
   }
   if (initializer) {
      *initializer = 0;
   }

   basictype = type;
   {
      unsigned int len = strlen(type);
      unsigned int nest = 0;
      for (unsigned int ind = len - 1; ind > 0; --ind) {
         switch (type[ind]) {
            case '<':
               --nest;
               break;
            case '>':
               ++nest;
               break;
            case ':':
               if (nest == 0 && type[ind-1] == ':') {
                  basictype = &(type[ind+1]);
                  ind = 1;
                  break;
               }
         }
      }
   }
   if (initializer) {
      *initializer = '(';
   }
   typenum = G__find_typedef(type);
   if (typenum) {
      tagnum = G__get_tagnum(typenum);
   }
   else {
      tagnum = -1;
   }
   if (arrayindex) {
      pinc = G__getarrayindex(arrayindex);
      *arrayindex = '\0';
      if (-1 == tagnum)  tagnum = G__defined_tagname(basictype, 1);
      if (-1 != tagnum) sprintf(construct, "%s()", G__struct.name[tagnum]);
      else sprintf(construct, "%s()", basictype);
      if (G__asm_wholefunction) G__abortbytecode();
   }
   else {
      // --
#ifdef G__ASM_IFUNC
      if (G__asm_noverflow) {
#ifdef G__ASM_DBG
         if (G__asm_dbg) G__fprinterr(G__serr, "%3x,%3x: LD 1 from %x  %s:%d\n", G__asm_cp, G__asm_dt, G__asm_dt, __FILE__, __LINE__);
#endif
         G__asm_inst[G__asm_cp] = G__LD;
         G__asm_inst[G__asm_cp+1] = G__asm_dt;
         G__asm_stack[G__asm_dt].obj.i = 1;
         G__value_typenum(G__asm_stack[G__asm_dt]) = G__get_from_type('i', 0);
         ld_flag = 1 ;
      }
#endif
      if (initializer) {
         pinc = 1;
         if (-1 == tagnum) {
            *initializer = 0;
            tagnum = G__defined_tagname(basictype, 2);
            *initializer = '(';
         }
         if (-1 != tagnum) sprintf(construct, "%s%s"
                                      , G__struct.name[tagnum], initializer);
         else strcpy(construct, basictype);
         *initializer = '\0';
      }
      else {
         pinc = 1;
         if (-1 != tagnum) sprintf(construct, "%s()", G__struct.name[tagnum]);
         else sprintf(construct, "%s()", basictype);
      }
   }

   size = G__Lsizeof(type);
   if (size == -1) {
      G__fprinterr(G__serr, "Error: type %s not defined FILE:%s LINE:%d\n"
                   , type, G__ifile.name, G__ifile.line_number);
      return(G__null);
   }

   /******************************************************
    * Store member function executing environment
    ******************************************************/
   store_struct_offset = G__store_struct_offset;
   store_tagnum = G__tagnum;
   store_typenum = G__typenum;
   result.ref = 0;

   /******************************************************
    * pointer type idendification
    ******************************************************/
   typelen = strlen(type);
   while ('*' == type[typelen-1]) {
      if (0 == ispointer) ispointer = 1;
      else {
         switch (reftype) {
            case G__PARANORMAL:
               reftype = G__PARAP2P;
               break;
            case G__PARAREFERENCE:
               break;
            default:
               ++reftype;
               break;
         }
      }
      type[--typelen] = '\0';
   }

   /******************************************************
    * identify type
    ******************************************************/
   G__typenum = G__find_typedef(type);
   if (G__typenum) {
      G__tagnum = G__typenum.RawType();
      var_type = G__get_type(G__typenum);
   }
   else {
      G__tagnum = G__Dict::GetDict().GetScope(G__defined_tagname(type, 1)); /* no error message */
      if (!G__tagnum.IsTopScope()) {
         var_type = 'u';
      }
      else {
         if (strcmp(type, "int") == 0) var_type = 'i';
         else if (strcmp(type, "char") == 0) var_type = 'c';
         else if (strcmp(type, "short") == 0) var_type = 's';
         else if (strcmp(type, "long") == 0) var_type = 'l';
         else if (strcmp(type, "float") == 0) var_type = 'f';
         else if (strcmp(type, "double") == 0) var_type = 'd';
         else if (strcmp(type, "void") == 0) var_type = 'y';
         else if (strcmp(type, "FILE") == 0) var_type = 'e';
         else if (strcmp(type, "unsignedint") == 0) var_type = 'h';
         else if (strcmp(type, "unsignedchar") == 0) var_type = 'b';
         else if (strcmp(type, "unsignedshort") == 0) var_type = 'r';
         else if (strcmp(type, "unsignedlong") == 0) var_type = 'l';
         else if (strcmp(type, "size_t") == 0) var_type = 'l';
         else if (strcmp(type, "time_t") == 0) var_type = 'l';
         else if (strcmp(type, "bool") == 0) var_type = 'g';
         else if (strcmp(type, "longlong") == 0) var_type = 'n';
         else if (strcmp(type, "unsignedlonglong") == 0) var_type = 'm';
         else if (strcmp(type, "longdouble") == 0) var_type = 'q';
      }
   }
   if (ispointer) {
      var_type = toupper(var_type);
   }

#ifdef G__ASM
   if (G__asm_noverflow) {
      if (ld_flag) {
         if (G__tagnum.IsTopScope() || G__CPPLINK != G__struct.iscpplink[G__get_tagnum(G__tagnum)]
               || ispointer || isupper(var_type)) {
            /* increment for LD 1, otherwise, cancel LD 1 */
            G__inc_cp_asm(2, 1);
         }
         else {
#ifdef G__ASM_DBG
            if (G__asm_dbg) G__fprinterr(G__serr, "Cancel LD 1\n");
#endif
         }
      }
#ifdef G__ASM_DBG
      if (G__asm_dbg) G__fprinterr(G__serr, "%3x: SETMEMFUNCENV\n", G__asm_cp);
#endif
      G__asm_inst[G__asm_cp] = G__SETMEMFUNCENV;
      G__inc_cp_asm(1, 0);
   }
#endif

   /******************************************************
    * allocate memory if this is a class object and
    * not pre-compiled.
    ******************************************************/
   if (
      G__tagnum.IsTopScope() ||
      (G__struct.iscpplink[G__get_tagnum(G__tagnum)] != G__CPPLINK) ||
      ispointer ||
      isupper(var_type)
   ) {
      if (G__no_exec_compile) {
         pointer = (char*)pinc;
      }
      else {
         if (arenaflag) {
            pointer = memarena;
         }
         else {
            // --
#ifdef G__ROOT
            pointer = (char*)G__new_interpreted_object(size * pinc);
#else
            pointer = new char[(size_t)(size*pinc)];
#endif
            // --
         }
      }
      if (pointer == (long)0 && 0 == G__no_exec_compile) {
         G__fprinterr(G__serr, "Error: memory allocation for %s %s size=%d pinc=%d FILE:%s LINE:%d\n"
                      , type, expression, size, pinc, G__ifile.name, G__ifile.line_number);
         G__tagnum = store_tagnum;
         G__typenum = store_typenum;
         return(G__null);
      }
      G__store_struct_offset = pointer;
#ifdef G__ASM_IFUNC
#ifdef G__ASM
      if (G__asm_noverflow) {
         // --
#ifdef G__ASM_DBG
         if (G__asm_dbg) G__fprinterr(G__serr, "%3x: NEWALLOC %d %d\n", G__asm_cp, size, pinc);
#endif
         G__asm_inst[G__asm_cp] = G__NEWALLOC;
         if (memarena) {
            G__asm_inst[G__asm_cp+1] = 0;
         }
         else {
            G__asm_inst[G__asm_cp+1] = size; /* pinc is in stack */
         }
         G__asm_inst[G__asm_cp+2] = (('u' == var_type) && arrayindex) ? 1 : 0;
         G__inc_cp_asm(3, 0);
#ifdef G__ASM_DBG
         if (G__asm_dbg) G__fprinterr(G__serr, "%3x: SET_NEWALLOC\n", G__asm_cp);
#endif
         G__asm_inst[G__asm_cp] = G__SET_NEWALLOC;
         ::Reflex::Type &type = *(reinterpret_cast<Reflex::Type*>(&G__asm_inst[G__asm_cp+1]));
         switch (var_type) {
            case 'u':
            case 'U':
               type = Reflex::PointerBuilder(G__tagnum);
               break;
            default:
               type = G__get_from_type(toupper(var_type),1);
         }
         G__inc_cp_asm(3, 0);
      }
#endif
#endif
      // --
   }

   /******************************************************
    * call constructor if struct, class
    ******************************************************/
   if (var_type == 'u') {
      if (G__struct.isabstract[G__get_tagnum(G__tagnum)]) {
         G__fprinterr(G__serr, "Error: abstract class object '%s' is created", G__struct.name[G__get_tagnum(G__tagnum)]);
         G__genericerror((char*)NULL);
         G__display_purevirtualfunc(G__get_tagnum(G__tagnum));
      }
      if (G__dispsource) {
         G__fprinterr(G__serr, "\n!!!Calling constructor 0x%lx.%s for new %s"
                      , G__store_struct_offset, type, type);
      }

      if (G__CPPLINK == G__struct.iscpplink[G__get_tagnum(G__tagnum)]) {
         /* This is a pre-compiled class */
         char *store_globalvarpointer = G__globalvarpointer;
         if (memarena) G__globalvarpointer = memarena;
         else G__globalvarpointer = G__PVOID;
         if (arrayindex) {
            G__cpp_aryconstruct = pinc;
#ifdef G__ASM
            if (G__asm_noverflow) {
#ifdef G__ASM_DBG
               if (G__asm_dbg) G__fprinterr(G__serr, "%3x: SETARYINDEX\n" , G__asm_cp);
#endif
               G__asm_inst[G__asm_cp] = G__SETARYINDEX;
               G__asm_inst[G__asm_cp+1] = 1;
               G__inc_cp_asm(2, 0);
            }
#endif
         }
         result = G__getfunction(construct, &known, G__CALLCONSTRUCTOR);
#ifdef G__ASM
         if (arrayindex && G__asm_noverflow) {
#ifdef G__ASM_DBG
            if (G__asm_dbg) G__fprinterr(G__serr, "%3x: RESETARYINDEX\n" , G__asm_cp);
#endif
            G__asm_inst[G__asm_cp] = G__RESETARYINDEX;
            G__asm_inst[G__asm_cp+1] = 1;
            G__inc_cp_asm(2, 0);
         }
#endif
         G__value_typenum(result) = ::Reflex::PointerBuilder(G__value_typenum(result));
         //REMOVED: result.isconst = G__VARIABLE;
         result.ref = 0;
         G__cpp_aryconstruct = 0;
         G__store_struct_offset = store_struct_offset;
         G__tagnum = store_tagnum;
         G__typenum = store_typenum;
         G__globalvarpointer = store_globalvarpointer;
#ifdef G__ASM
         if (memarena && G__asm_noverflow) {
            G__asm_inst[G__asm_cp] = G__SETGVP;
            G__asm_inst[G__asm_cp+1] = -1;
            G__inc_cp_asm(2, 0);
#ifdef G__ASM_DBG
            if (G__asm_dbg) G__fprinterr(G__serr, "%3x: SETGVP -1\n", G__asm_cp);
#endif
         }
#endif

#ifdef G__ASM
         if (G__asm_noverflow) {
#ifdef G__ASM_DBG
            if (G__asm_dbg) G__fprinterr(G__serr, "%3x: RECMEMFUNCENV\n", G__asm_cp);
#endif
            G__asm_inst[G__asm_cp] = G__RECMEMFUNCENV;
            G__inc_cp_asm(1, 0);
         }
#endif

#ifdef G__SECURITY
         if (G__security&G__SECURE_GARBAGECOLLECTION) {
            if (!G__no_exec_compile && 0 == memarena) {
               G__add_alloctable((void*)result.obj.i, G__get_type(result), G__get_tagnum(G__value_typenum(result)));
#ifdef G__ASM
               if (G__asm_noverflow) {
#ifdef G__ASM_DBG
                  if (G__asm_dbg) G__fprinterr(G__serr, "%3x: ADDALLOCTABLE\n", G__asm_cp);
#endif
                  G__asm_inst[G__asm_cp] = G__ADDALLOCTABLE;
                  G__inc_cp_asm(1, 0);
               }
#endif
            }
         }
#endif
         return(result);
      }
      else {
         // -- This is an interpreted class.
         if (
            arrayindex &&
            !G__no_exec_compile &&
            (G__struct.type[G__get_tagnum(G__tagnum)] != 'e')
         ) {
            G__alloc_newarraylist(pointer, pinc);
         }
         G__var_type = 'p';
         for (size_t i = 0;i < pinc;i++) {
            G__abortbytecode(); /* Disable bytecode */
            if (G__no_exec_compile) break;
            G__getfunction(construct, &known, G__TRYCONSTRUCTOR);
            result.ref = 0;
            if (known == 0) {
               if (initializer) {
                  G__value buf;
                  char *bp;
                  char *ep;
                  bp = strchr(construct, '(');
                  ep = strrchr(construct, ')');
                  G__ASSERT(bp && ep) ;
                  *ep = 0;
                  *bp = 0;
                  ++bp;
                  {
                     int cx, nx = 0;
                     char tmpx[G__ONELINE];
                     cx = G__getstream(bp, &nx, tmpx, "),");
                     if (',' == cx) {
                        *ep = ')';
                        *(bp - 1) = '(';
                        /* only to display error message */
                        G__getfunction(construct, &known, G__CALLCONSTRUCTOR);
                        break;
                     }
                  }
                  /* construct = "TYPE" , bp = "ARG" */
                  buf = G__getexpr(bp);
                  /* G__ASSERT(-1!=buf.tagnum); */
                  G__abortbytecode(); /* Disable bytecode */
                  if (G__value_typenum(buf).RawType().IsClass() && 0 == G__no_exec_compile) {
                     if (G__value_typenum(buf).RawType() != G__tagnum) {
                        G__fprinterr(G__serr
                                     , "Error: Illegal initialization of %s("
                                     , G__tagnum.Name(::Reflex::SCOPED).c_str());
                        G__fprinterr(G__serr, "%s)", G__value_typenum(buf).Name(::Reflex::SCOPED).c_str());
                        G__genericerror((char*)NULL);
                        return(G__null);
                     }
                     memcpy((void*)G__store_struct_offset, (void*)buf.obj.i
                            , G__value_typenum(buf).RawType().SizeOf());
                  }
               }
               break;
            }
            G__store_struct_offset += size;
            /* WARNING: FOLLOWING PART MUST BE REDESIGNED TO SUPPORT WHOLE
             * FUNCTION COMPILATION */
#ifdef G__ASM_IFUNC
#ifdef G__ASM
            G__abortbytecode(); /* Disable bytecode */
            if (G__asm_noverflow) {
               if (pinc > 1) {
#ifdef G__ASM_DBG
                  if (G__asm_dbg) G__fprinterr(G__serr, "%3x: ADDSTROS %d\n", G__asm_cp, size);
#endif
                  G__asm_inst[G__asm_cp] = G__ADDSTROS;
                  G__asm_inst[G__asm_cp+1] = size;
                  G__inc_cp_asm(2, 0);
               }
            }
#endif /* G__ASM */
#endif /* G__ASM_IFUNC */
            // --
         }
#ifdef G__ASM
         if (memarena && G__asm_noverflow) {
            G__asm_inst[G__asm_cp] = G__SETGVP;
            G__asm_inst[G__asm_cp+1] = -1;
            G__inc_cp_asm(2, 0);
#ifdef G__ASM_DBG
            if (G__asm_dbg) G__fprinterr(G__serr, "%3x: SETGVP -1'\n", G__asm_cp);
#endif
         }
#endif
         // --
      }
   }
   else if (initializer) {
      /* construct = "TYPE(ARG)" */
      struct G__param para;
      ::Reflex::Type typenum;
      int hash;
      char *bp;
      char *ep;
      int store_var_type;
      bp = strchr(construct, '(');
      ep = strrchr(construct, ')');
      *ep = 0;
      *bp = 0;
      ++bp;
      /* construct = "TYPE" , bp = "ARG" */
      typenum = G__find_typedef(construct);
      if (typenum) {
         strcpy(construct, G__type2string(G__get_type(typenum)
                                          , G__get_tagnum(typenum) , -1
                                          , G__get_reftype(typenum) , 0));
      }
      hash = strlen(construct);
      store_var_type = G__var_type;
      G__var_type = 'p';
      para.para[0] = G__getexpr(bp); /* generates LD or LD_VAR etc... */
      G__var_type = store_var_type;
      if (!G__no_exec_compile) result.ref = (long)pointer;
      else                    result.ref = 0;
      /* following call generates CAST instruction */
      if (var_type == 'U' && pointer) {
         if (0 == G__no_exec_compile) *(long*)pointer = para.para[0].obj.i;
      }
      else
         G__explicit_fundamental_typeconv(construct, hash, &para, &result);
#ifdef G__ASM
      if (G__asm_noverflow) {
         // --
#ifdef G__ASM_DBG
         if (G__asm_dbg) G__fprinterr(G__serr, "%3x: LETNEWVAL\n", G__asm_cp);
#endif
         G__asm_inst[G__asm_cp] = G__LETNEWVAL;
         G__inc_cp_asm(1, 0);
      }
#endif /* ASM */
#ifdef G__ASM
      if (memarena && G__asm_noverflow) {
         G__asm_inst[G__asm_cp] = G__SETGVP;
         G__asm_inst[G__asm_cp+1] = -1;
         G__inc_cp_asm(2, 0);
#ifdef G__ASM_DBG
         if (G__asm_dbg) G__fprinterr(G__serr, "%3x: SETGVP -1''\n", G__asm_cp);
#endif
         // --
      }
#endif
      // --
   }

   if (isupper(var_type)) {
      G__letint(&result, var_type, (long)pointer);
      G__value_typenum(result) = ::Reflex::PointerBuilder(G__modify_type(G__value_typenum(result), 0, reftype, 0, 0, 0));
   }
   else {
      G__letint(&result, toupper(var_type), (long)pointer);
      G__value_typenum(result) = G__modify_type(G__value_typenum(result), 0, reftype, 0, 0, 0);
   }
   if (G__tagnum && !G__tagnum.IsTopScope()) {
      G__value_typenum(result) = G__replace_rawtype(G__value_typenum(result), G__tagnum);
   }
   G__store_struct_offset = store_struct_offset;
   G__tagnum = store_tagnum;
   G__typenum = store_typenum;
#ifdef G__ASM
   if (G__asm_noverflow) {
      // --
#ifdef G__ASM_DBG
      if (G__asm_dbg) G__fprinterr(G__serr, "%3x: RECMEMFUNCENV\n", G__asm_cp);
#endif
      G__asm_inst[G__asm_cp] = G__RECMEMFUNCENV;
      G__inc_cp_asm(1, 0);
   }
#endif

#ifdef G__SECURITY
   if (G__security&G__SECURE_GARBAGECOLLECTION) {
      if (!G__no_exec_compile && 0 == memarena)
         G__add_alloctable((void*)result.obj.i, G__get_type(result), G__get_tagnum(G__value_typenum(result)));
#ifdef G__ASM
      if (G__asm_noverflow) {
         // --
#ifdef G__ASM_DBG
         if (G__asm_dbg) G__fprinterr(G__serr, "%3x: ADDALLOCTABLE\n", G__asm_cp);
#endif
         G__asm_inst[G__asm_cp] = G__ADDALLOCTABLE;
         G__inc_cp_asm(1, 0);
      }
#endif
      // --
   }
#endif
   return result;
}

/****************************************************************
* G__getarrayindex()
* 
* Called by:
*   G__new_operator()
*
*  [x][y][z]     get x*y*z
*
****************************************************************/
int Cint::Internal::G__getarrayindex(char* indexlist)
{
   char index[G__ONELINE];
   int store_var_type = G__var_type;
   G__var_type = 'p';
   int p = 1;
   int c = G__getstream(indexlist, &p, index, "]");
   int p_inc = 1;
   p_inc *= G__int(G__getexpr(index));
   while (*(indexlist + p) == '[') {
      ++p;
      c = G__getstream(indexlist, &p, index, "]");
      p_inc *= G__int(G__getexpr(index));
#ifdef G__ASM_IFUNC
#ifdef G__ASM
      if (G__asm_noverflow) {
         // -- We are generating bytecode.
#ifdef G__ASM_DBG
         if (G__asm_dbg) {
            G__fprinterr(G__serr, "%3x,%3x: OP2 '*'  %s:%d\n", G__asm_cp, G__asm_dt, __FILE__, __LINE__);
         }
#endif // G__ASM_DBG
         G__asm_inst[G__asm_cp] = G__OP2;
         G__asm_inst[G__asm_cp+1] = (long) '*';
         G__inc_cp_asm(2, 0);
      }
#endif // G__ASM
#endif // G__ASM_IFUNC
      // --
   }
   G__ASSERT(c == ']');
   G__var_type = store_var_type;
   return p_inc;
}

/****************************************************************
* void G__delete_operator()
* 
* Called by:
*   G__exec_statement(&brace_level);
*   G__exec_statement(&brace_level);
*
*        V
* delete 
*
****************************************************************/
void Cint::Internal::G__delete_operator(char *expression, int isarray)
{
   char *store_struct_offset; /* used to be int */
   ::Reflex::Scope store_tagnum;
   ::Reflex::Type store_typenum;
   int done;
   char destruct[G__ONELINE];
   G__value buf;
   int pinc, i, size;
   int cpplink = 0;
   int zeroflag = 0;

   buf = G__getitem(expression);
   if (!G__value_typenum(buf).FinalType().IsPointer()) {
      G__fprinterr(G__serr, "Error: %s cannot delete", expression);
      G__genericerror((char*)NULL);
      return;
   }
   else if (0 == buf.obj.i && 0 == G__no_exec_compile && G__ASM_FUNC_NOP == G__asm_wholefunction) {
      zeroflag = 1;
      G__no_exec_compile = 1;
      buf.obj.d = 0;
      buf.obj.i = 1;
   }

   G__CHECK(G__SECURE_MALLOC, 1, return);

#ifdef G__SECURITY
   if (G__security & G__SECURE_GARBAGECOLLECTION) {
      if (!G__no_exec_compile) {
         G__del_alloctable((void*)buf.obj.i);
      }
#ifdef G__ASM
      if (G__asm_noverflow) {
         // --
#ifdef G__ASM_DBG
         if (G__asm_dbg) G__fprinterr(G__serr, "%3x: DELALLOCTABLE\n", G__asm_cp);
#endif
         G__asm_inst[G__asm_cp] = G__DELALLOCTABLE;
         G__inc_cp_asm(1, 0);
      }
#endif
      // --
   }
#endif

   /*********************************************************
    * Call destructor if struct or class
    *********************************************************/
   if (
      (G__get_type(G__value_typenum(buf)) == 'U') && // pointer to class, struct, union
      (G__get_reftype(G__value_typenum(buf)) ==  G__PARANORMAL) // and, not a reference or multi-level pointer
   ) {
      store_struct_offset = G__store_struct_offset;
      store_typenum = G__typenum;
      store_tagnum = G__tagnum;

      G__store_struct_offset = (char*)buf.obj.i;
      G__typenum = G__value_typenum(buf);
      G__set_G__tagnum(buf);

      sprintf(destruct, "~%s()", G__struct.name[G__get_tagnum(G__tagnum)]);
      if (G__dispsource) {
         G__fprinterr(G__serr, "\n!!!Calling destructor 0x%lx.%s for %s"
                      , G__store_struct_offset , destruct , expression);
      }
      done = 0;

      if (0 == G__no_exec_compile && G__PVOID != G__struct.virtual_offset[G__get_tagnum(G__tagnum)] &&
            G__tagnum !=
            G__Dict::GetDict().GetScope(*(long*)(G__store_struct_offset + (size_t)G__struct.virtual_offset[G__get_tagnum(G__tagnum)]))) {
         int virtualtag =
            *(long*)(G__store_struct_offset + (size_t)G__struct.virtual_offset[G__get_tagnum(G__tagnum)]);
         buf.obj.i -= G__find_virtualoffset(virtualtag);
      }

      /*****************************************************
       * Push and set G__store_struct_offset
       *****************************************************/
#ifdef G__ASM_IFUNC
#ifdef G__ASM
      if (G__asm_noverflow) {
         G__asm_inst[G__asm_cp] = G__PUSHSTROS;
         G__asm_inst[G__asm_cp+1] = G__SETSTROS;
         G__inc_cp_asm(2, 0);
#ifdef G__ASM_DBG
         if (G__asm_dbg) {
            G__fprinterr(G__serr, "%3x: PUSHSTROS\n", G__asm_cp - 2);
            G__fprinterr(G__serr, "%3x: SETSTROS\n", G__asm_cp - 1);
         }
#endif
         if (isarray) {
            G__asm_inst[G__asm_cp] = G__GETARYINDEX;
#ifdef G__ASM_DBG
            if (G__asm_dbg) G__fprinterr(G__serr, "%3x: GETARYINDEX\n", G__asm_cp - 2);
#endif
            G__inc_cp_asm(1, 0);
         }
      }
#endif /* G__ASM */
#endif /* G__ASM_IFUNC */

      /*****************************************************
       * Call destructor
       *****************************************************/
      if (G__CPPLINK == G__struct.iscpplink[G__get_tagnum(G__tagnum)]) {
         /* pre-compiled class */
         if (isarray) G__cpp_aryconstruct = 1;

#ifndef G__ASM_IFUNC
#ifdef G__ASM
         if (G__asm_noverflow) {
            G__asm_inst[G__asm_cp] = G__PUSHSTROS;
            G__asm_inst[G__asm_cp+1] = G__SETSTROS;
            G__inc_cp_asm(2, 0);
#ifdef G__ASM_DBG
            if (G__asm_dbg) {
               G__fprinterr(G__serr, "%3x: PUSHSTROS\n", G__asm_cp - 2);
               G__fprinterr(G__serr, "%3x: SETSTROS\n", G__asm_cp - 1);
            }
#endif
         }
#endif /* G__ASM */
#endif /* !G__ASM_IFUNC */

         G__getfunction(destruct, &done, G__TRYDESTRUCTOR);
         /* Precompiled destructor must always exist here */

#ifndef G__ASM_IFUNC
#ifdef G__ASM
         if (G__asm_noverflow) {
#ifdef G__ASM_DBG
            if (G__asm_dbg) G__fprinterr(G__serr, "%3x: POPSTROS\n", G__asm_cp);
#endif
            G__asm_inst[G__asm_cp] = G__POPSTROS;
            G__inc_cp_asm(1, 0);
         }
#endif /* G__ASM */
#endif /* !G__ASM_IFUNC */

         G__cpp_aryconstruct = 0;
         cpplink = 1;
      }
      else {
         /* interpreted class */
         /* WARNING: FOLLOWING PART MUST BE REDESIGNED TO SUPPORT WHOLE
          * FUNCTION COMPILATION */
         if (isarray) {
            if (!G__no_exec_compile)
               pinc = G__free_newarraylist(G__store_struct_offset);
            else pinc = 1;
            size = G__struct.size[G__get_tagnum(G__tagnum)];
            for (i = pinc - 1;i >= 0;--i) {
               G__store_struct_offset = (char*)(buf.obj.i + size * i);
               G__getfunction(destruct, &done , G__TRYDESTRUCTOR);
#ifdef G__ASM_IFUNC
#ifdef G__ASM
               if (0 == done) break;
               G__abortbytecode(); /* Disable bytecode */
               if (G__asm_noverflow) {
#ifdef G__ASM_DBG
                  if (G__asm_dbg) G__fprinterr(G__serr, "%3x: ADDSTROS %d\n", G__asm_cp, size);
#endif
                  G__asm_inst[G__asm_cp] = G__ADDSTROS;
                  G__asm_inst[G__asm_cp+1] = (long)size;
                  G__inc_cp_asm(2, 0);
               }
#endif /* G__ASM */
#endif /* !G__ASM_IFUNC */
            }
         }
         else {
            G__getfunction(destruct, &done, G__TRYDESTRUCTOR);
         }
      }

#ifdef G__ASM
#ifdef G__SECURITY
      if (G__security&G__SECURE_GARBAGECOLLECTION && G__asm_noverflow && 0 == done) {
#ifdef G__ASM_DBG
         if (G__asm_dbg) G__fprinterr(G__serr, "%3x: BASEDESTRUCT\n", G__asm_cp);
#endif
         G__asm_inst[G__asm_cp] = G__BASEDESTRUCT;
         G__asm_inst[G__asm_cp+1] = G__get_tagnum(G__tagnum);
         G__asm_inst[G__asm_cp+2] = isarray;
         G__inc_cp_asm(3, 0);
      }
#endif
#endif

      /*****************************************************
       * Push and set G__store_struct_offset
       *****************************************************/
#ifdef G__ASM_IFUNC
#ifdef G__ASM
      if (G__asm_noverflow) {
         if (isarray) {
            G__asm_inst[G__asm_cp] = G__RESETARYINDEX;
            G__asm_inst[G__asm_cp+1] = 0;
#ifdef G__ASM_DBG
            if (G__asm_dbg) G__fprinterr(G__serr, "%3x: RESETARYINDEX\n", G__asm_cp - 2);
#endif
            G__inc_cp_asm(2, 0);
         }
         if (G__CPPLINK != G__struct.iscpplink[G__get_tagnum(G__tagnum)]) {
            /* if interpreted class, free memory */
#ifdef G__ASM_DBG
            if (G__asm_dbg) G__fprinterr(G__serr, "%3x: DELETEFREE\n", G__asm_cp);
#endif
            G__asm_inst[G__asm_cp] = G__DELETEFREE;
            G__asm_inst[G__asm_cp+1] = isarray ? 1 : 0;
            G__inc_cp_asm(2, 0);
         }
#ifdef G__ASM_DBG
         if (G__asm_dbg) G__fprinterr(G__serr, "%3x: POPSTROS\n", G__asm_cp + 1);
#endif
         G__asm_inst[G__asm_cp] = G__POPSTROS;
         G__inc_cp_asm(1, 0);
      }
#endif /* G__ASM */
#endif /* G__ASM_IFUNC */

      /*****************************************************
       * Push and set G__store_struct_offset
       *****************************************************/
      G__store_struct_offset = store_struct_offset;
      G__typenum = store_typenum;
      G__tagnum = store_tagnum;

   }
   else if (G__asm_noverflow) {
      G__asm_inst[G__asm_cp] = G__PUSHSTROS;
      G__asm_inst[G__asm_cp+1] = G__SETSTROS;
      G__inc_cp_asm(2, 0);
#ifdef G__ASM_DBG
      if (G__asm_dbg) {
         G__fprinterr(G__serr, "%3x: PUSHSTROS\n", G__asm_cp - 2);
         G__fprinterr(G__serr, "%3x: SETSTROS\n", G__asm_cp - 1);
      }
#endif
#ifdef G__ASM_DBG
      if (G__asm_dbg) G__fprinterr(G__serr, "%3x: DELETEFREE\n", G__asm_cp);
#endif
      G__asm_inst[G__asm_cp] = G__DELETEFREE;
      G__asm_inst[G__asm_cp+1] = 0;
      G__inc_cp_asm(2, 0);
#ifdef G__ASM_DBG
      if (G__asm_dbg) G__fprinterr(G__serr, "%3x: POPSTROS\n", G__asm_cp);
#endif
      G__asm_inst[G__asm_cp] = G__POPSTROS;
      G__inc_cp_asm(1, 0);
   }

   /*****************************************************
    * free memory if interpreted object
    *****************************************************/
   if (G__NOLINK == cpplink && !G__no_exec_compile) {
#ifdef G__ROOT
      G__delete_interpreted_object((void*)buf.obj.i);
#else
      delete[](char*) buf.obj.i;
#endif
   }

   /* #ifdef G__ROOT */
   /*****************************************************
    * assign NULL for deleted pointer variable
    *****************************************************/
   if (buf.ref && 0 == G__no_exec && 0 == G__no_exec_compile) *(long*)buf.ref = 0;
   /* #endif G__ROOT */

   if (zeroflag) {
      G__no_exec_compile = 0;
      buf.obj.i = 0;
   }
}

/****************************************************************
* G__alloc_newarraylist()
****************************************************************/
int Cint::Internal::G__alloc_newarraylist(void* point, int pinc)
{
   struct G__newarylist *newary;

#ifdef G__MEMTEST
   fprintf(G__memhist, "G__alloc_newarraylist(%lx,%d)\n", point, pinc);
#endif

   /****************************************************
    * Find out end of list
    ****************************************************/
   newary = &G__newarray;
   while (newary->next) newary = newary->next;


   /****************************************************
    * create next list
    ****************************************************/
   newary->next = (struct G__newarylist *)malloc(sizeof(struct G__newarylist));
   /****************************************************
    * store information
    ****************************************************/
   newary = newary->next;
   newary->point = (long)point;
   newary->pinc = pinc;
   newary->next = (struct G__newarylist *)NULL;
   return(0);
}

/****************************************************************
* G__free_newarraylist()
****************************************************************/
int Cint::Internal::G__free_newarraylist(void *point)
{
   struct G__newarylist *newary, *prev;
   int pinc, flag = 0;

#ifdef G__MEMTEST
   fprintf(G__memhist, "G__free_newarraylist(%lx)\n", point);
#endif

   /****************************************************
    * Search point
    ****************************************************/
   prev = &G__newarray;
   newary = G__newarray.next;
   while (newary) {
      if ((void*)newary->point == point) {
         flag = 1;
         break;
      }
      prev = newary;
      newary = newary->next;
   }

   if (flag == 0) {
      G__fprinterr(G__serr, "Error: delete[] on wrong object 0x%lx FILE:%s LINE:%d\n"
                   , point, G__ifile.name, G__ifile.line_number);
      return(0);
   }

   /******************************************************
    * get malloc size information
    ******************************************************/
   pinc = newary->pinc;

   /******************************************************
    * delete newarraylist
    ******************************************************/
   prev->next = newary->next;
   free((void*)newary);

   /* return result */
   return(pinc);
}

/**************************************************************************
* G__handle_delete
*
* Parsing of 'delete obj' 'delete obj[]'
**************************************************************************/
int Cint::Internal::G__handle_delete(int *piout ,char *statement)
{
  int c;
  c=G__fgetstream(statement ,"[;");
  *piout=0;
  if('['==c) {
    if('\0'==statement[0]) {
      c=G__fgetstream(statement ,"]");
      c=G__fgetstream(statement ,";");
      *piout=1;
    }
    else {
      strcpy(statement+strlen(statement),"[");
      c=G__fgetstream(statement+strlen(statement),"]");
      strcpy(statement+strlen(statement),"]");
      c=G__fgetstream(statement+strlen(statement),";");
    }
  }
  return(0);
}

/*
 * Local Variables:
 * c-tab-always-indent:nil
 * c-indent-level:2
 * c-continued-statement-offset:2
 * c-brace-offset:-2
 * c-brace-imaginary-offset:0
 * c-argdecl-indent:0
 * c-label-offset:-2
 * compile-command:"make -k"
 * End:
 */
