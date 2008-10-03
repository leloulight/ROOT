// @(#)root/reflex:$Id$
// Author: Stefan Roiser 2004

// Copyright CERN, CH-1211 Geneva 23, 2004-2006, All rights reserved.
//
// Permission to use, copy, modify, and distribute this software for any
// purpose is hereby granted without fee, provided that this copyright and
// permissions notice appear in all copies and derivatives.
//
// This software is provided "as is" without express or implied warranty.

#ifndef REFLEX_BUILD
#define REFLEX_BUILD
#endif

#include "Reflex/Builder/TypeBuilder.h"

#include "Reflex/Type.h"
#include "TypeName.h"

#include "Pointer.h"
#include "Function.h"
#include "Array.h"
#include "Enum.h"
#include "Typedef.h"
#include "PointerToMember.h"
#include "ScopeName.h"
#include "Reflex/Tools.h"
#include "Reflex/EntityProperty.h"


//-------------------------------------------------------------------------------
Reflex::Type Reflex::TypeBuilder(const char * n, 
                                 unsigned int modifiers,
                                 const Catalog& catalog) {
//-------------------------------------------------------------------------------
// Construct the type information for a type.
   const Type & ret = catalog.Types().ByName(n);
   if (ret.Id()) return Type(ret, modifiers);
   else {
      Internal::TypeName* tname = new Internal::TypeName(n, 0, catalog);
      std::string sname = Tools::GetScopeName(n);
      if (! Scope::ByName(sname).Id())
         new Internal::ScopeName(sname.c_str(), 0, catalog);
      return Type(tname, modifiers);
   }
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::ConstBuilder(const Type & t) {
//-------------------------------------------------------------------------------
// Construct a const qualified type.
   unsigned int mod = kConst;
   if (t.Is(gVolatile)) mod |= kVolatile;
   return Type(t,mod);    
}

//-------------------------------------------------------------------------------
Reflex::Type Reflex::VolatileBuilder(const Type & t) {
//-------------------------------------------------------------------------------
// Construct a volatile qualified type.
   unsigned int mod = kVolatile;
   if (t.Is(gConst))    mod |= kConst;
   return Type(t,mod);    
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::PointerBuilder(const Type & t,
                                    const std::type_info & ti,
                                    const Catalog& catalog) {
//-------------------------------------------------------------------------------
// Construct a pointer type.
   std::string buf;
   const Type & ret = Type::ByName(Internal::Pointer::BuildTypeName(buf, t));
   if (ret) return ret;
   else       return (new Internal::Pointer(t, 0, ti, catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type
Reflex::PointerToMemberBuilder(const Type & t,
                               const Scope & s,
                               const std::type_info & ti,
                               const Catalog& catalog) {
//-------------------------------------------------------------------------------
// Construct a pointer type.
   std::string buf;
   const Type & ret = Type::ByName(Internal::PointerToMember::BuildTypeName(buf, t, s));
   if (ret) return ret;
   else       return (new Internal::PointerToMember(t, 0, s, ti, catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::ReferenceBuilder(const Type & t) {
//-------------------------------------------------------------------------------
// Construct a "reference qualified" type.
   unsigned int mod = kReference;
   if (t.Is(gConst))    mod |= kConst;
   if (t.Is(gVolatile)) mod |= kVolatile;
   return Type(t,mod);    
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::ArrayBuilder(const Type & t, 
                                  size_t n,
                                  const std::type_info & ti,
                                  const Catalog& catalog) {
//-------------------------------------------------------------------------------
// Construct an array type.
   const Type & ret = Type::ByName(Internal::Array::BuildTypeName(t,n));
   if (ret) return ret;
   else       return (new Internal::Array(t, 0, n, ti, catalog))->ThisType();
}

//-------------------------------------------------------------------------------
Reflex::Type Reflex::EnumTypeBuilder(const char * nam, 
                                     const char * values,
                                     const std::type_info & ti,
                                     const Catalog& catalog) {
//-------------------------------------------------------------------------------
// Construct an enum type.

   std::string nam2(nam);

   const Type & ret = Type::ByName(nam2);
   if (ret) {
      if (ret.Is(gTypedef)) nam2 += " @HIDDEN@";
      else return ret;
   }

   Internal::Enum * e = new Internal::Enum(nam2.c_str(), ti, catalog, 0);

   std::vector<std::string> valVec;
   Tools::StringSplit(valVec, values, ";");

   const Type & int_t = Type::ByName("int");
   for (std::vector<std::string>::const_iterator it = valVec.begin(); 
        it != valVec.end(); ++it) {
      std::string iname, ivalue;
      Tools::StringSplitPair(iname, ivalue, *it, "=");
      long val = atol(ivalue.c_str());
      e->AddMember(iname.c_str(), int_t, val, 0);
   }  
   return e->ThisType();
}

//-------------------------------------------------------------------------------
Reflex::Type Reflex::TypedefTypeBuilder(const char * nam, 
                                        const Type & t,
                                        const Catalog& catalog) {
//-------------------------------------------------------------------------------
// Construct a typedef type.
   Type ret = Type::ByName(nam);
   // Check for typedef AA AA;
   if (ret == t && ! t.Is(gTypedef)) 
      if (t) t.ToTypeBase()->HideName();
      else ((Internal::TypeName*)t.Id())->HideName();
   // We found the typedef type
   else if (ret) return ret;
   // Create a new typedef
   return (new Internal::Typedef(nam , t, catalog))->ThisType();        
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r,
                                         const std::vector<Type> & p,
                                         const std::type_info & ti,
                                         const Catalog& catalog) {
//-------------------------------------------------------------------------------
// Construct a function type.
   return (new Internal::Function(r, p, ti, catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r,
                                         const Catalog& catalog) {
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector< Type > v;
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0,
                                         const Catalog& catalog) {
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Catalog& catalog) {
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Catalog& catalog) {
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Catalog& catalog) {
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Catalog& catalog) {
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}
 

//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Catalog& catalog) {
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Catalog& catalog) {
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Catalog& catalog) {
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Catalog& catalog) {
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Type & t9,
                                         const Catalog& catalog) {
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}
 

//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Type & t9,
                                         const Type & t10,
                                         const Catalog& catalog) {
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Type & t9,
                                         const Type & t10,
                                         const Type & t11,
                                         const Catalog& catalog) {
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}
 

//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Type & t9,
                                         const Type & t10,
                                         const Type & t11,
                                         const Type & t12,
                                         const Catalog& catalog) {
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector< Type > v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}
 

//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Type & t9,
                                         const Type & t10,
                                         const Type & t11,
                                         const Type & t12,
                                         const Type & t13,
                                         const Catalog& catalog) {
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Type & t9,
                                         const Type & t10,
                                         const Type & t11,
                                         const Type & t12,
                                         const Type & t13,
                                         const Type & t14,
                                         const Catalog& catalog) {
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}
 

//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Type & t9,
                                         const Type & t10,
                                         const Type & t11,
                                         const Type & t12,
                                         const Type & t13,
                                         const Type & t14,
                                         const Type & t15,
                                         const Catalog& catalog) { 
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Type & t9,
                                         const Type & t10,
                                         const Type & t11,
                                         const Type & t12,
                                         const Type & t13,
                                         const Type & t14,
                                         const Type & t15,
                                         const Type & t16,
                                         const Catalog& catalog) { 
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15,
                                           t16);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Type & t9,
                                         const Type & t10,
                                         const Type & t11,
                                         const Type & t12,
                                         const Type & t13,
                                         const Type & t14,
                                         const Type & t15,
                                         const Type & t16,
                                         const Type & t17,
                                         const Catalog& catalog) { 
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15,
                                           t16, t17);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Type & t9,
                                         const Type & t10,
                                         const Type & t11,
                                         const Type & t12,
                                         const Type & t13,
                                         const Type & t14,
                                         const Type & t15,
                                         const Type & t16,
                                         const Type & t17,
                                         const Type & t18,
                                         const Catalog& catalog) { 
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15,
                                           t16, t17, t18);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Type & t9,
                                         const Type & t10,
                                         const Type & t11,
                                         const Type & t12,
                                         const Type & t13,
                                         const Type & t14,
                                         const Type & t15,
                                         const Type & t16,
                                         const Type & t17,
                                         const Type & t18,
                                         const Type & t19,
                                         const Catalog& catalog) { 
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15,
                                           t16, t17, t18, t19);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Type & t9,
                                         const Type & t10,
                                         const Type & t11,
                                         const Type & t12,
                                         const Type & t13,
                                         const Type & t14,
                                         const Type & t15,
                                         const Type & t16,
                                         const Type & t17,
                                         const Type & t18,
                                         const Type & t19,
                                         const Type & t20,
                                         const Catalog& catalog) { 
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15,
                                           t16, t17, t18, t19, t20);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Type & t9,
                                         const Type & t10,
                                         const Type & t11,
                                         const Type & t12,
                                         const Type & t13,
                                         const Type & t14,
                                         const Type & t15,
                                         const Type & t16,
                                         const Type & t17,
                                         const Type & t18,
                                         const Type & t19,
                                         const Type & t20,
                                         const Type & t21,
                                         const Catalog& catalog) { 
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15,
                                           t16, t17, t18, t19, t20, t21);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Type & t9,
                                         const Type & t10,
                                         const Type & t11,
                                         const Type & t12,
                                         const Type & t13,
                                         const Type & t14,
                                         const Type & t15,
                                         const Type & t16,
                                         const Type & t17,
                                         const Type & t18,
                                         const Type & t19,
                                         const Type & t20,
                                         const Type & t21,
                                         const Type & t22,
                                         const Catalog& catalog) { 
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15,
                                           t16, t17, t18, t19, t20, t21, t22);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Type & t9,
                                         const Type & t10,
                                         const Type & t11,
                                         const Type & t12,
                                         const Type & t13,
                                         const Type & t14,
                                         const Type & t15,
                                         const Type & t16,
                                         const Type & t17,
                                         const Type & t18,
                                         const Type & t19,
                                         const Type & t20,
                                         const Type & t21,
                                         const Type & t22,
                                         const Type & t23,
                                         const Catalog& catalog) { 
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15,
                                           t16, t17, t18, t19, t20, t21, t22, t23);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Type & t9,
                                         const Type & t10,
                                         const Type & t11,
                                         const Type & t12,
                                         const Type & t13,
                                         const Type & t14,
                                         const Type & t15,
                                         const Type & t16,
                                         const Type & t17,
                                         const Type & t18,
                                         const Type & t19,
                                         const Type & t20,
                                         const Type & t21,
                                         const Type & t22,
                                         const Type & t23,
                                         const Type & t24,
                                         const Catalog& catalog) { 
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15,
                                           t16, t17, t18, t19, t20, t21, t22, t23, t24);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Type & t9,
                                         const Type & t10,
                                         const Type & t11,
                                         const Type & t12,
                                         const Type & t13,
                                         const Type & t14,
                                         const Type & t15,
                                         const Type & t16,
                                         const Type & t17,
                                         const Type & t18,
                                         const Type & t19,
                                         const Type & t20,
                                         const Type & t21,
                                         const Type & t22,
                                         const Type & t23,
                                         const Type & t24,
                                         const Type & t25,
                                         const Catalog& catalog) { 
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15,
                                           t16, t17, t18, t19, t20, t21, t22, t23, t24, t25);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Type & t9,
                                         const Type & t10,
                                         const Type & t11,
                                         const Type & t12,
                                         const Type & t13,
                                         const Type & t14,
                                         const Type & t15,
                                         const Type & t16,
                                         const Type & t17,
                                         const Type & t18,
                                         const Type & t19,
                                         const Type & t20,
                                         const Type & t21,
                                         const Type & t22,
                                         const Type & t23,
                                         const Type & t24,
                                         const Type & t25,
                                         const Type & t26,
                                         const Catalog& catalog) { 
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15,
                                           t16, t17, t18, t19, t20, t21, t22, t23, t24, t25, t26);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Type & t9,
                                         const Type & t10,
                                         const Type & t11,
                                         const Type & t12,
                                         const Type & t13,
                                         const Type & t14,
                                         const Type & t15,
                                         const Type & t16,
                                         const Type & t17,
                                         const Type & t18,
                                         const Type & t19,
                                         const Type & t20,
                                         const Type & t21,
                                         const Type & t22,
                                         const Type & t23,
                                         const Type & t24,
                                         const Type & t25,
                                         const Type & t26,
                                         const Type & t27,
                                         const Catalog& catalog) { 
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15,
                                           t16, t17, t18, t19, t20, t21, t22, t23, t24, t25, t26, t27);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Type & t9,
                                         const Type & t10,
                                         const Type & t11,
                                         const Type & t12,
                                         const Type & t13,
                                         const Type & t14,
                                         const Type & t15,
                                         const Type & t16,
                                         const Type & t17,
                                         const Type & t18,
                                         const Type & t19,
                                         const Type & t20,
                                         const Type & t21,
                                         const Type & t22,
                                         const Type & t23,
                                         const Type & t24,
                                         const Type & t25,
                                         const Type & t26,
                                         const Type & t27,
                                         const Type & t28,
                                         const Catalog& catalog) { 
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15,
                                           t16, t17, t18, t19, t20, t21, t22, t23, t24, t25, t26, t27, t28);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Type & t9,
                                         const Type & t10,
                                         const Type & t11,
                                         const Type & t12,
                                         const Type & t13,
                                         const Type & t14,
                                         const Type & t15,
                                         const Type & t16,
                                         const Type & t17,
                                         const Type & t18,
                                         const Type & t19,
                                         const Type & t20,
                                         const Type & t21,
                                         const Type & t22,
                                         const Type & t23,
                                         const Type & t24,
                                         const Type & t25,
                                         const Type & t26,
                                         const Type & t27,
                                         const Type & t28,
                                         const Type & t29,
                                         const Catalog& catalog) { 
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15,
                                           t16, t17, t18, t19, t20, t21, t22, t23, t24, t25, t26, t27, t28, t29);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Type & t9,
                                         const Type & t10,
                                         const Type & t11,
                                         const Type & t12,
                                         const Type & t13,
                                         const Type & t14,
                                         const Type & t15,
                                         const Type & t16,
                                         const Type & t17,
                                         const Type & t18,
                                         const Type & t19,
                                         const Type & t20,
                                         const Type & t21,
                                         const Type & t22,
                                         const Type & t23,
                                         const Type & t24,
                                         const Type & t25,
                                         const Type & t26,
                                         const Type & t27,
                                         const Type & t28,
                                         const Type & t29,
                                         const Type & t30,
                                         const Catalog& catalog) { 
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15,
                                           t16, t17, t18, t19, t20, t21, t22, t23, t24, t25, t26, t27, t28, t29,
                                           t30);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}


//-------------------------------------------------------------------------------
Reflex::Type Reflex::FunctionTypeBuilder(const Type & r, 
                                         const Type & t0, 
                                         const Type & t1,
                                         const Type & t2,
                                         const Type & t3,
                                         const Type & t4,
                                         const Type & t5,
                                         const Type & t6,
                                         const Type & t7,
                                         const Type & t8,
                                         const Type & t9,
                                         const Type & t10,
                                         const Type & t11,
                                         const Type & t12,
                                         const Type & t13,
                                         const Type & t14,
                                         const Type & t15,
                                         const Type & t16,
                                         const Type & t17,
                                         const Type & t18,
                                         const Type & t19,
                                         const Type & t20,
                                         const Type & t21,
                                         const Type & t22,
                                         const Type & t23,
                                         const Type & t24,
                                         const Type & t25,
                                         const Type & t26,
                                         const Type & t27,
                                         const Type & t28,
                                         const Type & t29,
                                         const Type & t30,
                                         const Type & t31,
                                         const Catalog& catalog) { 
//-------------------------------------------------------------------------------
// Construct a function type.
   std::vector<Type> v = Tools::MakeVector(t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15,
                                           t16, t17, t18, t19, t20, t21, t22, t23, t24, t25, t26, t27, t28, t29,
                                           t30, t31);
   const Type & ret = Type::ByName(Internal::Function::BuildTypeName(r,v));
   if (ret) return ret;
   else       return (new Internal::Function(r, v, typeid(UnknownType), catalog))->ThisType();
}



