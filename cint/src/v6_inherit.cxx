/* /% C %/ */
/***********************************************************************
 * cint (C/C++ interpreter)
 ************************************************************************
 * Source file inherit.c
 ************************************************************************
 * Description:
 *  Class inheritance 
 ************************************************************************
 * Copyright(c) 1995~1999  Masaharu Goto (MXJ02154@niftyserve.or.jp)
 *
 * Permission to use, copy, modify and distribute this software and its 
 * documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  The author makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 ************************************************************************/

#include "common.h"


/**************************************************************************
* G__inheritclass
*
*  Recursively inherit base class
*
**************************************************************************/
void G__inheritclass(to_tagnum,from_tagnum,baseaccess)
int to_tagnum,from_tagnum;
char baseaccess;
{
  int i,offset,basen;
  struct G__inheritance *to_base,*from_base;
#ifndef G__OLDIMPLEMENTATION692
  int isvirtualbase;
#endif

#ifndef G__OLDIMPLEMENTATION604
  if(-1==to_tagnum || -1==from_tagnum) return;
#endif

#ifndef G__OLDIMPLEMENTATION1327
  if(G__NOLINK==G__globalcomp && 
     G__CPPLINK==G__struct.iscpplink[from_tagnum] &&
     G__CPPLINK!=G__struct.iscpplink[to_tagnum]) {
#ifndef G__OLDIMPLEMENTATION1368
    G__fprinterr(
	   "Warning: Interpreted class %s derived from"
	    ,G__fulltagname(to_tagnum,1));
    G__fprinterr(
	   " precompiled class %s",G__fulltagname(from_tagnum,1));
#else
    G__fprinterr(
	   "Warning: precompiled class %s ",G__fulltagname(from_tagnum,1));
    G__fprinterr(
	   "inherited from interpreted class %s",G__fulltagname(to_tagnum,1));
#endif
    G__printlinenum();
    G__fprinterr("!!!There are some limitations regarding compiled/interpreted class inheritance\n");
  }
#endif

  to_base = G__struct.baseclass[to_tagnum];
  from_base = G__struct.baseclass[from_tagnum];

#ifndef G__OLDIMPLEMENTATION604
  if(!to_base || !from_base) return;
#endif

  offset = to_base->baseoffset[to_base->basen]; /* just to simplify */

  /****************************************************
  * copy virtual offset 
  ****************************************************/
  /* Bug fix for multiple inheritance, if virtual offset is already
   * set, don't overwrite.  */
  if(-1 != G__struct.virtual_offset[from_tagnum] &&
     -1 == G__struct.virtual_offset[to_tagnum]) {
#ifdef G__VIRTUALBASE
    if(to_base->property[to_base->basen]&G__ISVIRTUALBASE) {
      G__struct.virtual_offset[to_tagnum] 
	=offset+G__struct.virtual_offset[from_tagnum]+G__DOUBLEALLOC;
    }
    else {
      G__struct.virtual_offset[to_tagnum] 
	=offset+G__struct.virtual_offset[from_tagnum];
    }
#else
    G__struct.virtual_offset[to_tagnum] 
      =offset+G__struct.virtual_offset[from_tagnum];
#endif
  }

  G__struct.isabstract[to_tagnum]+=G__struct.isabstract[from_tagnum];
#ifndef G__OLDIMPLEMENTATION1441
  G__struct.funcs[to_tagnum] |= (G__struct.funcs[from_tagnum]&0xf0);
#endif

  /****************************************************
  *  copy grand base class info 
  ****************************************************/
#ifndef G__OLDIMPLEMENTATION692
  isvirtualbase = (to_base->property[to_base->basen]&G__ISVIRTUALBASE); 
#endif
  basen=to_base->basen;
  for(i=0;i<from_base->basen;i++) {
    ++basen;
    to_base->basetagnum[basen] = from_base->basetagnum[i];
    to_base->baseoffset[basen] = offset+from_base->baseoffset[i];
#ifndef G__OLDIMPLEMENTATION692
    to_base->property[basen] 
      = ((from_base->property[i]&G__ISVIRTUALBASE) | isvirtualbase);
#else
    to_base->property[basen] = from_base->property[i]&G__ISVIRTUALBASE;
#endif
    if(from_base->baseaccess[i]>=G__PRIVATE) 
      to_base->baseaccess[basen]=G__GRANDPRIVATE;
    else if(G__PRIVATE==baseaccess)
      to_base->baseaccess[basen]=G__PRIVATE;
    else if(G__PROTECTED==baseaccess&&G__PUBLIC==from_base->baseaccess[i])
      to_base->baseaccess[basen]=G__PROTECTED;
    else
      to_base->baseaccess[basen]=from_base->baseaccess[i];
  }
  to_base->basen=basen+1;

}

/**************************************************************************
* G__baseconstructorwp
*
*  Read constructor arguments and
*  Recursively call base class constructor
*
**************************************************************************/
int G__baseconstructorwp()
{
  int c;
  int n=0;
  char buf[G__ONELINE];
  struct G__baseparam baseparam;
  
  /*  X::X(int a,int b) : base1(a), base2(b) { }
   *                   ^
   */
  c=G__fignorestream(":{");
  if(':'==c) c=',';
  
  while(','==c) {
    c=G__fgetstream_newtemplate(buf,"({,"); /* case 3) */
    if('('==c) {
      baseparam.name[n]=(char*)malloc(strlen(buf)+1);
      strcpy(baseparam.name[n],buf);
#ifndef G__OLDIMPLEMENTATION438
      c=G__fgetstream_newtemplate(buf,")");
#else
      c=G__fgetstream(buf,")");
#endif
      baseparam.param[n]=(char*)malloc(strlen(buf)+1);
      strcpy(baseparam.param[n],buf);
      ++n;
      c=G__fgetstream(buf,",{");
    }
  }
  
  G__baseconstructor(n,&baseparam);
  
  while(n>0) {
    --n;
    free((void*)baseparam.param[n]);
    free((void*)baseparam.name[n]);
  }
  
  fseek(G__ifile.fp,-1,SEEK_CUR);
  if(G__dispsource) G__disp_mask=1;
  return(0);
}

#ifdef G__VIRTUALBASE
/**************************************************************************
* struct and global object for virtual base class address list
**************************************************************************/
struct G__vbaseaddrlist {
  int tagnum;
  long vbaseaddr;
  struct G__vbaseaddrlist *next;
};

static struct G__vbaseaddrlist *G__pvbaseaddrlist 
  = (struct G__vbaseaddrlist*)NULL;
static int G__toplevelinstantiation=1;

/**************************************************************************
* G__storevbaseaddrlist()
**************************************************************************/
static struct G__vbaseaddrlist* G__storevbaseaddrlist()
{
  struct G__vbaseaddrlist *temp;
  temp = G__pvbaseaddrlist;
  G__pvbaseaddrlist = (struct G__vbaseaddrlist*)NULL;
  return(temp);
}

/**************************************************************************
* G__freevbaseaddrlist()
**************************************************************************/
static void G__freevbaseaddrlist(pvbaseaddrlist)
struct G__vbaseaddrlist *pvbaseaddrlist;
{
  if(pvbaseaddrlist) {
    if(pvbaseaddrlist->next) G__freevbaseaddrlist(pvbaseaddrlist->next);
    free((void*)pvbaseaddrlist);
  }
}

/**************************************************************************
* G__restorevbaseaddrlist()
**************************************************************************/
static void G__restorevbaseaddrlist(pvbaseaddrlist)
struct G__vbaseaddrlist *pvbaseaddrlist;
{
  G__freevbaseaddrlist(G__pvbaseaddrlist);
  G__pvbaseaddrlist = pvbaseaddrlist;
}

/**************************************************************************
* G__setvbaseaddrlist()
**************************************************************************/
static void G__setvbaseaddrlist(tagnum,pobject,baseoffset)
int tagnum;
long pobject;
long baseoffset;
{
  struct G__vbaseaddrlist *pvbaseaddrlist;
  struct G__vbaseaddrlist *last=(struct G__vbaseaddrlist*)NULL;
  long vbaseosaddr;
  vbaseosaddr = pobject+baseoffset;

  pvbaseaddrlist = G__pvbaseaddrlist;
  while(pvbaseaddrlist) {
    if(pvbaseaddrlist->tagnum == tagnum) {
      /* *(long*)vbaseosaddr = pvbaseaddrlist->vbaseaddr - pobject; */
      *(long*)vbaseosaddr = pvbaseaddrlist->vbaseaddr - vbaseosaddr;
      return;
    }
    last = pvbaseaddrlist;
    pvbaseaddrlist = pvbaseaddrlist->next;
  }
  if(last) {
    last->next
      = (struct G__vbaseaddrlist*)malloc(sizeof(struct G__vbaseaddrlist));
    pvbaseaddrlist = last->next;
  }
  else {
    G__pvbaseaddrlist
      = (struct G__vbaseaddrlist*)malloc(sizeof(struct G__vbaseaddrlist));
    pvbaseaddrlist = G__pvbaseaddrlist;
  }
  pvbaseaddrlist->tagnum = tagnum;
  pvbaseaddrlist->vbaseaddr = vbaseosaddr + G__DOUBLEALLOC;
  pvbaseaddrlist->next = (struct G__vbaseaddrlist*)NULL;
  /* *(long*)vbaseosaddr = pvbaseaddrlist->vbaseaddr - pobject ; */
  *(long*)vbaseosaddr = pvbaseaddrlist->vbaseaddr - vbaseosaddr ;
}
#endif

/**************************************************************************
* G__baseconstructor
*
*  Recursively call base class constructor
*
**************************************************************************/
int G__baseconstructor(n,pbaseparam)
int n;
struct G__baseparam *pbaseparam;
{
  struct G__var_array *mem;
  struct G__inheritance *baseclass;
  int store_tagnum;
  long store_struct_offset;
  int i,j;
  char *tagname,*memname;
  int flag;
  char construct[G__ONELINE];
  int p_inc,size;
  long store_globalvarpointer;
  int donen=0;
  long addr,lval;
  double dval;
#ifdef G__VIRTUALBASE
  int store_toplevelinstantiation;
  struct G__vbaseaddrlist *store_pvbaseaddrlist=NULL;
#endif
  
  /* store current tag information */
  store_tagnum=G__tagnum;
  store_struct_offset = G__store_struct_offset;
  store_globalvarpointer = G__globalvarpointer;
  
#ifdef G__VIRTUALBASE
  if(G__toplevelinstantiation) {
    store_pvbaseaddrlist = G__storevbaseaddrlist();
  }
  store_toplevelinstantiation=G__toplevelinstantiation;
  G__toplevelinstantiation=0;
#endif
  
  /****************************************************************
   * base classes
   ****************************************************************/
#ifndef G__OLDIMPLEMENTATION604
  if(-1==store_tagnum) return(0);
#endif
  baseclass=G__struct.baseclass[store_tagnum];
#ifndef G__OLDIMPLEMENTATION604
  if(!baseclass) return(0);
#endif
  for(i=0;i<baseclass->basen;i++) {
    if(baseclass->property[i]&G__ISDIRECTINHERIT) {
      G__tagnum = baseclass->basetagnum[i];
#ifdef G__VIRTUALBASE
      if(baseclass->property[i]&G__ISVIRTUALBASE) {
	long vbaseosaddr;
	vbaseosaddr = store_struct_offset+baseclass->baseoffset[i];
	G__setvbaseaddrlist(G__tagnum,store_struct_offset
			    ,baseclass->baseoffset[i]);
	/*
	if(baseclass->baseoffset[i]+G__DOUBLEALLOC==(*(long*)vbaseosaddr)) {
	  G__store_struct_offset=store_struct_offset+(*(long*)vbaseosaddr);
	}
	*/
	if(G__DOUBLEALLOC==(*(long*)vbaseosaddr)) {
	  G__store_struct_offset=vbaseosaddr+(*(long*)vbaseosaddr);
	}
	else {
	  G__store_struct_offset=vbaseosaddr+G__DOUBLEALLOC;
	  if(-1 != G__struct.virtual_offset[G__tagnum]) {
	    *(long*)(G__store_struct_offset+G__struct.virtual_offset[G__tagnum])
	      = store_tagnum;
	  }
	  continue;
	}
      }
      else {
	G__store_struct_offset=store_struct_offset+baseclass->baseoffset[i];
      }
#else
      G__store_struct_offset=store_struct_offset+baseclass->baseoffset[i];
#endif

      /* search for constructor argument */ 
      flag=0;
      tagname = G__struct.name[G__tagnum];
      if(donen<n) {
	for(j=0;j<n;j++) {
	  if(strcmp(pbaseparam->name[j],tagname)==0) {
	    flag=1;
	    ++donen;
	    break;
	  }
	}
      }
      if(flag) 
	sprintf(construct,"%s(%s)" ,tagname,pbaseparam->param[j]);
      else 
	sprintf(construct,"%s()",tagname);
      
      
      if(G__dispsource) {
	G__fprinterr("\n!!!Calling base class constructor %s",construct);
      }
      j=0;
      if(G__CPPLINK==G__struct.iscpplink[G__tagnum]) { /* C++ compiled class */
	G__globalvarpointer=G__store_struct_offset;
      }
      else G__globalvarpointer=G__PVOID;
      G__getfunction(construct,&j ,G__TRYCONSTRUCTOR);
      /* Set virtual_offset to every base classes for possible multiple
       * inheritance. */
      if(-1 != G__struct.virtual_offset[G__tagnum]) {
	*(long*)(G__store_struct_offset+G__struct.virtual_offset[G__tagnum])
	  = store_tagnum;
      }
    } /* end of if ISDIRECTINHERIT */
#ifndef G__OLDIMPLEMENTATION524
    else { /* !ISDIREDCTINHERIT , bug fix for multiple inheritance */
      if(0==(baseclass->property[i]&G__ISVIRTUALBASE)) {
	G__tagnum = baseclass->basetagnum[i];
	if(-1 != G__struct.virtual_offset[G__tagnum]) {
	  G__store_struct_offset=store_struct_offset+baseclass->baseoffset[i];
	  *(long*)(G__store_struct_offset+G__struct.virtual_offset[G__tagnum])
	    = store_tagnum;
	}
      }
    }
#endif
  }
  G__globalvarpointer = store_globalvarpointer;

#ifdef G__VIRTUALBASE
  if(store_toplevelinstantiation) {
    G__restorevbaseaddrlist(store_pvbaseaddrlist);
  }
  G__toplevelinstantiation=1;
#endif
  
  /****************************************************************
   * class members
   ****************************************************************/
  G__incsetup_memvar(store_tagnum);
  mem=G__struct.memvar[store_tagnum];

  while(mem) {
    for(i=0;i<mem->allvar;i++) {
      if('u'==mem->type[i] && 
#ifndef G__NEWINHERIT
	 0==mem->isinherit[i] &&
#endif
	 'e'!=G__struct.type[mem->p_tagtable[i]] &&
	 G__LOCALSTATIC!=mem->statictype[i]) {
	
	G__tagnum=mem->p_tagtable[i];
	G__store_struct_offset = store_struct_offset+mem->p[i];
	
	flag=0;
	memname=mem->varnamebuf[i];
	if(donen<n) {
	  for(j=0;j<n;j++) {
	    if(strcmp(pbaseparam->name[j] ,memname)==0) {
	      flag=1;
	      ++donen;
	      break;
	    }
	  }
	}
	if(flag) {
	  if(G__PARAREFERENCE==mem->reftype[i]) {
#ifndef G__OLDIMPLEMENTATION945
	    if(G__NOLINK!=G__globalcomp) 
#endif
	      {
	    if('\0'==pbaseparam->param[j][0]) {
	      G__fprinterr("Error: No initializer for reference %s "
		      ,memname);
	      G__genericerror((char*)NULL);
	    }
	    else {
	      G__genericerror("Limitation: initialization of reference member not implemented");
	    }
	      }
	    continue;
	  }
	  sprintf(construct,"%s(%s)" ,G__struct.name[G__tagnum]
		  ,pbaseparam->param[j]);
	}
	else {
	  sprintf(construct,"%s()" ,G__struct.name[G__tagnum]);
	  if(G__PARAREFERENCE==mem->reftype[i]) {
#ifndef G__OLDIMPLEMENTATION945
	    if(G__NOLINK!=G__globalcomp) 
#endif
	      {
	    G__fprinterr("Error: No initializer for reference %s "
		    ,memname);
	    G__genericerror((char*)NULL);
	      }
	    continue;
	  }
	}
	if(G__dispsource) {
	  G__fprinterr("\n!!!Calling class member constructor %s",construct);
	}
	p_inc = mem->varlabel[i][1];
	size = G__struct.size[G__tagnum];
	j=0;
	do {
	  if(G__CPPLINK==G__struct.iscpplink[G__tagnum]) { /* C++ compiled */
	    G__globalvarpointer=G__store_struct_offset;
	  }
	  else G__globalvarpointer = G__PVOID;
	  G__getfunction(construct,&j ,G__TRYCONSTRUCTOR);
	  G__store_struct_offset += size;
	  --p_inc;
	} while(p_inc>=0 && j) ;
      } /* if('u') */

      else if(donen<n && G__LOCALSTATIC!=mem->statictype[i]) {
	flag=0;
	memname=mem->varnamebuf[i];
	for(j=0;j<n;j++) {
	  if(strcmp(pbaseparam->name[j] ,memname)==0) {
	    flag=1;
	    ++donen;
	    break;
	  }
	}
	if(flag) {
	  if(G__PARAREFERENCE==mem->reftype[i]) {
#ifndef G__OLDIMPLEMENTATION945
	    if(G__NOLINK!=G__globalcomp) 
#endif
	      {
	    if('\0'==pbaseparam->param[j][0]) {
	      G__fprinterr("Error: No initializer for reference %s "
		      ,memname);
	      G__genericerror((char*)NULL);
	    }
	    else {
	      G__genericerror("Limitation: initialization of reference member not implemented");
	    }
	      }
	    continue;
	  }
	  else {
	    addr = store_struct_offset+mem->p[i];
	    if(isupper(mem->type[i])) {
	      lval = G__int(G__getexpr(pbaseparam->param[j]));
	      *(long*)addr = lval;
	    }
	    else {
	      switch(mem->type[i]) {
	      case 'b':
		lval = G__int(G__getexpr(pbaseparam->param[j]));
		*(unsigned char*)addr = lval;
		break;
	      case 'c':
		lval = G__int(G__getexpr(pbaseparam->param[j]));
		*(char*)addr = lval;
		break;
	      case 'r':
		lval = G__int(G__getexpr(pbaseparam->param[j]));
		*(unsigned short*)addr = lval;
		break;
	      case 's':
		lval = G__int(G__getexpr(pbaseparam->param[j]));
		*(short*)addr = lval;
		break;
	      case 'h':
		lval = G__int(G__getexpr(pbaseparam->param[j]));
		*(unsigned int*)addr = lval;
		break;
	      case 'i':
		lval = G__int(G__getexpr(pbaseparam->param[j]));
		*(int*)addr = lval;
		break;
	      case 'k':
		lval = G__int(G__getexpr(pbaseparam->param[j]));
		*(unsigned long*)addr = lval;
		break;
	      case 'l':
		lval = G__int(G__getexpr(pbaseparam->param[j]));
		*(long*)addr = lval;
		break;
	      case 'f':
		dval = G__double(G__getexpr(pbaseparam->param[j]));
		*(float*)addr = dval;
		break;
	      case 'd':
		dval = G__double(G__getexpr(pbaseparam->param[j]));
		*(double*)addr = dval;
		break;
	      default:
		G__genericerror("Error: Illegal type in member initialization");
		break;
	      }
	    } /* if(isupper) else */
	  } /* if(reftype) else */
	} /* if(flag) */
      } /* else if(!LOCALSTATIC) */

    } /* for(i) */
    mem=mem->next;
  } /* while(mem) */

  G__globalvarpointer = store_globalvarpointer;
#ifdef G__VIRTUALBASE
  G__toplevelinstantiation=store_toplevelinstantiation;
#endif
  
  /* restore derived tagnum */
  G__tagnum = store_tagnum;
  G__store_struct_offset = store_struct_offset;
  
  /* assign virtual_identity if contains virtual
   * function.  */
  if(-1 != G__struct.virtual_offset[G__tagnum]
#ifndef G__OLDIMPLEMENTATION1164
     /* && 0==G__no_exec_compile  << this one is correct */
     && 0==G__xrefflag
#endif
     ) {
    *(long*)(G__store_struct_offset+G__struct.virtual_offset[G__tagnum])
      = G__tagnum;
  }
  return(0);
}

/**************************************************************************
* G__basedestructor
*
*  Recursively call base class destructor
*
**************************************************************************/
int G__basedestructor()
{
  struct G__var_array *mem;
  struct G__inheritance *baseclass;
  int store_tagnum;
  long store_struct_offset;
  int i,j;
  char destruct[G__ONELINE];
  long store_globalvarpointer;
#ifndef G__OLDIMPLEMENTATION1158
  int store_addstros=0;
#endif

  /* store current tag information */
  store_tagnum=G__tagnum;
  store_struct_offset = G__store_struct_offset;
  store_globalvarpointer = G__globalvarpointer;
  
  /****************************************************************
   * class members
   ****************************************************************/
  G__incsetup_memvar(store_tagnum);
  mem=G__struct.memvar[store_tagnum];
  G__basedestructrc(mem);

  /****************************************************************
   * base classes
   ****************************************************************/
  baseclass=G__struct.baseclass[store_tagnum];
  for(i=baseclass->basen-1;i>=0;i--) {
    if(baseclass->property[i]&G__ISDIRECTINHERIT) {
      G__tagnum = baseclass->basetagnum[i];
#ifdef G__VIRTUALBASE
      if(baseclass->property[i]&G__ISVIRTUALBASE) {
	long vbaseosaddr;
	vbaseosaddr = store_struct_offset+baseclass->baseoffset[i];
	/*
	if(baseclass->baseoffset[i]+G__DOUBLEALLOC==(*(long*)vbaseosaddr)) {
	  G__store_struct_offset=store_struct_offset+(*(long*)vbaseosaddr);
	}
	*/
	if(G__DOUBLEALLOC==(*(long*)vbaseosaddr)) {
	  G__store_struct_offset=vbaseosaddr+(*(long*)vbaseosaddr);
#ifndef G__OLDIMPLEMENTATION1158
	  if(G__asm_noverflow) {
	    store_addstros=baseclass->baseoffset[i]+(*(long*)vbaseosaddr);
	  }
#endif
	}
	else {
	  continue;
	}
      }
      else {
	G__store_struct_offset=store_struct_offset+baseclass->baseoffset[i];
#ifndef G__OLDIMPLEMENTATION1158
	if(G__asm_noverflow) {
	  store_addstros=baseclass->baseoffset[i];
	}
#endif
      }
#else
      G__store_struct_offset=store_struct_offset+baseclass->baseoffset[i];
#endif
#ifndef G__OLDIMPLEMENTATION1158
      if(G__asm_noverflow) G__gen_addstros(store_addstros);
#endif
      /* avoid recursive and infinite virtual destructor call 
       * let the base class object pretend like its own class object */
      if(-1!=G__struct.virtual_offset[G__tagnum]) 
	*(long*)(G__store_struct_offset+G__struct.virtual_offset[G__tagnum])
	  = G__tagnum;
      sprintf(destruct,"~%s()",G__struct.name[G__tagnum]);
      if(G__dispsource) 
	G__fprinterr("\n!!!Calling base class destructor %s",destruct);
      j=0;
      if(G__CPPLINK==G__struct.iscpplink[G__tagnum]) {
	G__globalvarpointer = G__store_struct_offset;
      }
      else G__globalvarpointer = G__PVOID;
      G__getfunction(destruct,&j ,G__TRYDESTRUCTOR);
#ifndef G__OLDIMPLEMENTATION1158
      if(G__asm_noverflow) G__gen_addstros(-store_addstros);
#endif
    }
  }
  G__globalvarpointer = store_globalvarpointer;

  /* finish up */
  G__tagnum = store_tagnum;
  G__store_struct_offset = store_struct_offset;
  return(0);
}

/**************************************************************************
* G__basedestructrc
*
*  calling desructors for member objects
**************************************************************************/
int G__basedestructrc(mem)
struct G__var_array *mem;
{
  /* int store_tagnum; */
  long store_struct_offset;
  int i,j;
  char destruct[G__ONELINE];
  int p_inc,size;
  long store_globalvarpointer;
  long address;

#ifndef G__OLDIMPLEMENTATION604
  if(!mem) return(1);
#endif

  store_globalvarpointer = G__globalvarpointer;
  
  if(mem->next) {
    G__basedestructrc(mem->next);
  }

  /* store current tag information */
  /* store_tagnum=G__tagnum; */
  store_struct_offset = G__store_struct_offset;
  
  for(i=mem->allvar-1;i>=0;i--) {
    if('u'==mem->type[i] && 
#ifndef G__NEWINHERIT
       0==mem->isinherit[i] &&
#endif
       'e'!=G__struct.type[mem->p_tagtable[i]] &&
       G__LOCALSTATIC!=mem->statictype[i]) {
      
      G__tagnum=mem->p_tagtable[i];
      G__store_struct_offset=store_struct_offset+mem->p[i];
      sprintf(destruct,"~%s()",G__struct.name[G__tagnum]);
      p_inc = mem->varlabel[i][1];
      size = G__struct.size[G__tagnum];
#ifndef G__OLDIMPLEMENTATION1158
      if(G__asm_noverflow) {
	if(0==p_inc) G__gen_addstros(mem->p[i]);
	else         G__gen_addstros(mem->p[i]+size*p_inc);
      }
#endif

      j=0;
      G__store_struct_offset += size*p_inc;
      do {
	if(G__CPPLINK==G__struct.iscpplink[G__tagnum]) { /* C++ compiled */
	  G__globalvarpointer=G__store_struct_offset;
	}
	else G__globalvarpointer = G__PVOID;
	/* avoid recursive and infinite virtual destructor call */
	if(-1!=G__struct.virtual_offset[G__tagnum]) 
	  *(long*)(G__store_struct_offset+G__struct.virtual_offset[G__tagnum])
	    = G__tagnum;
	if(G__dispsource) {
	  G__fprinterr("\n!!!Calling class member destructor %s" ,destruct);
	}
	G__getfunction(destruct,&j,G__TRYDESTRUCTOR);
	G__store_struct_offset -= size;
#ifndef G__OLDIMPLEMENTATION1158
	if(p_inc && G__asm_noverflow) G__gen_addstros(-size);
#endif
	--p_inc;
      } while(p_inc>=0 && j) ;
      G__globalvarpointer = G__PVOID;
#ifndef G__OLDIMPLEMENTATION1158
      if(G__asm_noverflow) G__gen_addstros(-mem->p[i]);
#endif
    }
    else if(G__security&G__SECURE_GARBAGECOLLECTION && 
#ifndef G__OLDIMPLEMENTATION545
	    (!G__no_exec_compile) &&
#endif
	    isupper(mem->type[i])) {
      j=mem->varlabel[i][1]+1;
      do {
	--j;
	address = G__store_struct_offset+mem->p[i]+G__LONGALLOC*j;
	if(*(long*)address) {
	  G__del_refcount((void*)(*((long*)address)) ,(void**)address);
	}
      } while(j);
    }
  }
  G__globalvarpointer = store_globalvarpointer;
#ifndef G__OLDIMPLEMENTATION1264
  G__store_struct_offset = store_struct_offset;
#endif
  return(0);
}


/**************************************************************************
* G__ispublicbase()
*
* check if derivedtagnum is derived from basetagnum. 
* If public base or reference from member function return offset
* else return -1
* Used in standard pointer conversion
**************************************************************************/
int G__ispublicbase(basetagnum,derivedtagnum
#ifdef G__VIRTUALBASE
		    ,pobject
#endif
		    )
int basetagnum,derivedtagnum;
#ifdef G__VIRTUALBASE
long pobject;
#endif
{
  struct G__inheritance *derived;
  int i,n;

  if(0>derivedtagnum) return(-1);
  if(basetagnum==derivedtagnum) return(0);
  derived = G__struct.baseclass[derivedtagnum];
  n = derived->basen;

  for(i=0;i<n;i++) {
    if(basetagnum == derived->basetagnum[i]) {
      if(derived->baseaccess[i]==G__PUBLIC ||
	 (G__exec_memberfunc && G__tagnum==derivedtagnum &&
	  G__GRANDPRIVATE!=derived->baseaccess[i])) {
#ifdef G__VIRTUALBASE
	if(derived->property[i]&G__ISVIRTUALBASE) {
	  return(G__getvirtualbaseoffset(pobject,derivedtagnum,derived,i));
	}
	else {
	  return(derived->baseoffset[i]);
	}
#else
	return(derived->baseoffset[i]);
#endif
      }
    }
  }

  return(-1);
}

/**************************************************************************
* G__isanybase()
*
* check if derivedtagnum is derived from basetagnum. If true return offset
* to the base object. If faulse, return -1.
* Used in cast operatotion
**************************************************************************/
int G__isanybase(basetagnum,derivedtagnum
#ifdef G__VIRTUALBASE
		    ,pobject
#endif
		 )
int basetagnum,derivedtagnum;
#ifdef G__VIRTUALBASE
long pobject;
#endif
{
  struct G__inheritance *derived;
  int i,n;

#ifndef G__OLDIMPLEMENTATION1060
  if (0 > derivedtagnum) {
    for (i = 0; i < G__globalusingnamespace.basen; i++) {
      if (G__globalusingnamespace.basetagnum[i] == basetagnum)
        return 0;
    }
    return -1;
  }
#else
  if(0>derivedtagnum) return(-1);
#endif
  if(basetagnum==derivedtagnum) return(0);
  derived = G__struct.baseclass[derivedtagnum];
  n = derived->basen;

  for(i=0;i<n;i++) {
    if(basetagnum == derived->basetagnum[i]) {
#ifdef G__VIRTUALBASE
      if(derived->property[i]&G__ISVIRTUALBASE) {
	return(G__getvirtualbaseoffset(pobject,derivedtagnum,derived,i));
      }
      else {
	return(derived->baseoffset[i]);
      }
#else
      return(derived->baseoffset[i]);
#endif
    }
  }

  return(-1);
}


/**************************************************************************
* G__find_virtualoffset()
*
*  Used in G__interpret_func to subtract offset for calling virtual function
*
**************************************************************************/
int G__find_virtualoffset(virtualtag)
int virtualtag;
{
  int i;
  struct G__inheritance *baseclass;
  
  if(0>virtualtag) return(0);
  baseclass = G__struct.baseclass[virtualtag];
  for(i=0;i<baseclass->basen;i++) {
    if(G__tagnum==baseclass->basetagnum[i]) {
#ifndef G__OLDIMPLEMENTATION658
      if(baseclass->property[i]&G__ISVIRTUALBASE) {
	return(baseclass->baseoffset[i]+G__DOUBLEALLOC);
      }
      else {
	return(baseclass->baseoffset[i]);
      }
#else
      return(baseclass->baseoffset[i]);
#endif
    }
  }
  return(0);
}

#ifdef G__VIRTUALBASE
/**************************************************************************
* G__getvirtualbaseoffset()
**************************************************************************/
long G__getvirtualbaseoffset(pobject,tagnum,baseclass,basen)
long pobject;
int tagnum;
struct G__inheritance *baseclass;
int basen;
{
  long (*f) G__P((long));
#ifndef G__OLDIMPLEMENTATION652
  if(!pobject || G__no_exec_compile
#ifndef G__OLDIMPLEMENTATION1140
     || -1==pobject || 1==pobject
#endif
     ) {
    G__abortbytecode();
    return(0);
  }
#else
  if(!pobject || G__no_exec_compile) return(0);
#endif
  if(G__CPPLINK==G__struct.iscpplink[tagnum]) {
    f = (long (*) G__P((long)))(baseclass->baseoffset[basen]);
    return((*f)(pobject));
  }
  else {
    /* return((*(long*)(pobject+baseclass->baseoffset[basen]))); */
    return(baseclass->baseoffset[basen]
	   +(*(long*)(pobject+baseclass->baseoffset[basen])));
  }
}
#endif

#ifndef G__OLDIMPLEMENTATION697
/***********************************************************************
* G__publicinheritance()
***********************************************************************/
int G__publicinheritance(val1,val2)
G__value *val1;
G__value *val2;
{
  long lresult;
  if('U'==val1->type && 'U'==val2->type) {
    if(-1!=(lresult=G__ispublicbase(val1->tagnum,val2->tagnum,val2->obj.i))) {
      val2->tagnum = val1->tagnum;
      val2->obj.i += lresult;
      return(lresult);
    }
    else if(-1!=(lresult=G__ispublicbase(val2->tagnum,val1->tagnum
					 ,val1->obj.i))) {
      val1->tagnum = val2->tagnum;
      val1->obj.i += lresult;
      return(-lresult);
    }
  }
  return 0;
}
#endif

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
