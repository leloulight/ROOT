############################################################################
# CMakeLists.txt file for building ROOT core/base package
############################################################################

ROOT_GLOB_HEADERS(headers1 RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/inc ${CMAKE_CURRENT_SOURCE_DIR}/inc/T*.h)
set(headers3 GuiTypes.h KeySymbols.h Buttons.h TTimeStamp.h TVirtualMutex.h 
             TVirtualPerfStats.h TVirtualX.h TParameter.h 
             TVirtualAuth.h TFileInfo.h TFileCollection.h 
             TRedirectOutputGuard.h TVirtualMonitoring.h TObjectSpy.h
             TUri.h TUrl.h TInetAddress.h TVirtualTableInterface.h
             TBase64.h)
list(REMOVE_ITEM headers1 ${headers3})

ROOT_GLOB_SOURCES(sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx)

ROOT_GENERATE_DICTIONARY(G__Base1 ${headers1} LINKDEF LinkDef1.h)
ROOT_GENERATE_DICTIONARY(G__Base2 ${headers1} LINKDEF LinkDef2.h)
ROOT_GENERATE_DICTIONARY(G__Base3 ${headers3} LINKDEF LinkDef3.h)

set(BASE1_DICTIONARY  ${CMAKE_CURRENT_BINARY_DIR}/G__Base1.cxx PARENT_SCOPE)
set(BASE2_DICTIONARY  ${CMAKE_CURRENT_BINARY_DIR}/G__Base2.cxx PARENT_SCOPE)
set(BASE3_DICTIONARY  ${CMAKE_CURRENT_BINARY_DIR}/G__Base3.cxx PARENT_SCOPE)

set(LibCore_SRCS ${LibCore_SRCS} ${sources} PARENT_SCOPE)
set(LibCore_LINKDEF ${LibCore_LINKDEF} 
                    ${CMAKE_CURRENT_SOURCE_DIR}/inc/LinkDef1.h
                    ${CMAKE_CURRENT_SOURCE_DIR}/inc/LinkDef2.h
                    ${CMAKE_CURRENT_SOURCE_DIR}/inc/LinkDef3.h
                    PARENT_SCOPE)

add_custom_target(BASE_DICTIONARY DEPENDS G__Base1.cxx
                                          G__Base2.cxx 
                                          G__Base3.cxx 
                                          ${base4dict} )
set_target_properties(BASE_DICTIONARY PROPERTIES FOLDER Dictionaries)
ROOT_INSTALL_HEADERS()

#---Extra options-----------------------------------------------------------------------

if(GCC_MAJOR EQUAL 4 AND GCC_MINOR EQUAL 1)
  set_source_files_properties(${CMAKE_SOURCE_DIR}/core/base/src/TString.cxx
                              ${CMAKE_SOURCE_DIR}/core/base/src/TContextMenu.cxx
                              COMPILE_FLAGS -Wno-strict-aliasing )
endif()

if(${ROOT_ARCHITECTURE} MATCHES linuxicc)
  set_source_files_properties( ${CMAKE_CURRENT_BINARY_DIR}/G__Base3.cxx 
                               COMPILE_FLAGS -wd191 )
endif()

