// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#ifndef WASMEDGE_JAVA_CONSTANTS_H
#define WASMEDGE_JAVA_CONSTANTS_H

#define ORG_WASMEDGE_FUNCTIONTYPECONTEXT "org/wasmedge/FunctionTypeContext"
#define ORG_WASMEDGE_STATISTICSCONTEXT "org/wasmedge/StatisticsContext"
#define ORG_WASMEDGE_MEMORYINSTANCECONTEXT "org/wasmedge/MemoryInstanceContext"
#define ORG_WASMEDGE_GLOBALTYPECONTEXT "org/wasmedge/GlobalTypeContext"
#define ORG_WASMEDGE_EXPORTTYPECONTEXT "org/wasmedge/ExportTypeContext"
#define ORG_WASMEDGE_MEMORYTYPECONTEXT "org/wasmedge/MemoryTypeContext"
#define ORG_WASMEDGE_GLOBALINSTANCECONTEXT "org/wasmedge/GlobalInstanceContext"
#define ORG_WASMEDGE_MODULEINSTANCECONTEXT "org/wasmedge/ModuleInstanceContext"
#define ORG_WASMEDGE_WASMEDGEVM "org/wasmedge/WasmEdgeVm"
#define ORG_WASMEDGE_FUNCTIONINSTANCECONTEXT                                   \
  "org/wasmedge/FunctionInstanceContext"
#define ORG_WASMEDGE_TABLEINSTANCECONTEXT "org/wasmedge/TableInstanceContext"
#define ORG_WASMEDGE_ASYNC "org/wasmedge/Async"
#define ORG_WASMEDGE_ASTMODULECONTEXT "org/wasmedge/AstModuleContext"
#define ORG_WASMEDGE_STORECONTEXT "org/wasmedge/StoreContext"
#define ORG_WASMEDGE_TABLETYPECONTEXT "org/wasmedge/TableTypeContext"
#define ORG_WASMEDGE_IMPORTTYPECONTEXT "org/wasmedge/ImportTypeContext"
#define ORG_WASMEDGE_LIMIT "org/wasmedge/Limit"
#define LIMIT_IS_HAS_MAX "isHasMax"
#define LIMIT_GET_MAX "getMax"
#define LIMIT_GET_MIN "getMin"
#define SET_NAME "setName"
#define GET "get"

#define DEFAULT_CONSTRUCTOR "<init>"
#define SET_VALUE_METHOD "setValue"
#define GET_NAME "getName"
#define TO_STRING "toString"

/** Internal Values **/
#define POINTER "pointer"
#define POINTER_TYPE "J"

/** Java Exceptions **/
#define JAVA_LANG_NOCLASSDEFFOUNDERROR "java/lang/NoClassDefFoundError"
#define JAVA_LANG_NOSUCHMETHODERROR "java/lang/NoSuchMethodError"
#define JAVA_LANG_EXCEPTION "java/lang/Exception"
#define JAVA_LANG_RUNTIMEEXCEPTION "java/lang/RuntimeException"
#define GET_CLASS "getClass"

/** ArrayList **/
#define JAVA_UTIL_ARRAYLIST "java/util/ArrayList"
#define LIST_SIZE "size"
#define ADD_ELEMENT "add"

/** Value Types **/
#define ORG_WASMEDGE_VALUE "org/wasmedge/Value"
#define ORG_WASMEDGE_ENUMS_VALUETYPE "org/wasmedge/enums/ValueType"
#define GET_VALUE "getValue"
#define GET_TYPE "getType"
#define PARSE_TYPE "parseType"
#define VOID_VALUETYPE "()Lorg/wasmedge/enums/ValueType;"
#define INT_VALUETYPE "(I)Lorg/wasmedge/enums/ValueType;"

#define ORG_WASMEDGE_I32VALUE "org/wasmedge/I32Value"
#define ORG_WASMEDGE_I64VALUE "org/wasmedge/I64Value"
#define ORG_WASMEDGE_F32VALUE "org/wasmedge/F32Value"
#define ORG_WASMEDGE_V128VALUE "org/wasmedge/V128Value"
#define ORG_WASMEDGE_F64VALUE "org/wasmedge/F64Value"
#define ORG_WASMEDGE_EXTERNREF "org/wasmedge/ExternRef"
#define ORG_WASMEDGE_FUNCREF "org/wasmedge/FuncRef"

/** method signatures **/
#define VOID_VOID "()V"
#define VOID_CLASS "()Ljava/lang/Class;"
#define VOID_STRING "()Ljava/lang/String;"
#define VOID_LONG "()J"
#define VOID_INT "()I"
#define VOID_FLOAT "()F"
#define VOID_DOUBLE "()D"

#define INT_VOID "(I)V"
#define LONG_VOID "(J)V"
#define FLOAT_VOID "(F)V"
#define DOUBLE_VOID "(D)V"
#define STRING_VOID "(Ljava/lang/String;)V"
#define VOID_BOOL "()Z"
#define BOOLLONGLONG_VOID "(ZJJ)V"
#define ASTMODULECONTEXT_VOID "(JLorg/wasmedge/AstModuleContext;)V"
#define STRING_HOSTFUNCTION "(Ljava/lang/String;)Lorg/wasmedge/HostFunction;"
#define MEMORYINSTANCECONTEXTLIST_RESULT                                       \
  "(Lorg/wasmedge/MemoryInstanceContext;Ljava/util/List;Ljava/util/"           \
  "List;)Lorg/wasmedge/Result;"
#define OBJECT_BOOL "(Ljava/lang/Object;)Z"
#define LISTLIST_VOID "(Ljava/util/List;Ljava/util/List;)V"
#define INT_OBJECT "(I)Ljava/lang/Object;"

/** host function **/
#define GET_HOST_FUNC "getHostFunc"
#define APPLY "apply"

/** messages **/
#define ERR_CLASS_NOT_FOUND "Exception class not found"
#define ERR_EXCEPTION_THROWN_CLASS_NOT_FOUND "Exception thrown for no class def"
#define ERR_NO_SUCH_METHOD "Exception thrown for no such method"
#define ERR_FIND_CLASS "find class error"
#define ERR_GET_CLASS_NAME "get class name error"
#define ERR_POINTER_FIELD_NOT_FOUND "pointer filed not found"
#define ERR_TEMPLATE "Error occurred with message: %s."
#define ERR_GET_INT_VALUE "Error get int value"
#define ERR_CREATE_JAVA_LIST "Error when creating java list"
#define ERR_GET_NAME_FALIED "get name error"
#define ERR_RUN_FROM_FILE_TEMPLATE                                             \
  "Error running wasm from file %s, error message: %s."
#define ERR_CREATE_VALUE_TYPE_LIST_FAILED "Error when creating value type list"
#define ERR_ADD_VALUE_TYPE "Error when adding value type"
#define ERROR_CREATE_FUNCTION_TYPE_FAILED                                      \
  "Error when creating function type context"
#define ERR_SET_FUNCTION_TYPE_FAILED                                           \
  "Error when setting function type context name"
#define ERR_CREATE_STATICS_CONTEXT_FAILED "error creating stat context"

#endif
