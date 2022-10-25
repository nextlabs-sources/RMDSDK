

#ifndef _FLTMAN_LOG_HLP_HPP__
#define _FLTMAN_LOG_HLP_HPP__


#include <nudf\eh.hpp>
#include <nudf\string.hpp>
#include <nudf\debug.hpp>
#include <nudf\dbglog.hpp>
#include <nudf\shared\moddef.h>


//
//  Handy Log Macro
//

// NX::dbg::log_item le(LEVEL, NX::dbg::build_error_msg(const std::string& file_name, const std::string& function_name, int line, const std::string& category, const std::string format, ...));

#define LOGMAN(MGR, LEVEL, INFO, ...) \
    if(NULL!=MGR && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback && ((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback(LEVEL)) { \
        NX::dbg::log_item le(LEVEL, NX::dbg::build_error_msg(NXMODNAME(), INFO, __VA_ARGS__)); \
        ((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback(le.get_log_level(), le.get_message().c_str()); \
    }

#define LOGMANEX(MGR, LEVEL, INFO, ...) \
    if(NULL!=MGR && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback && ((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback(LEVEL)) { \
        NX::dbg::log_item le(LEVEL, NX::dbg::build_error_msg2(__FILE__, __LINE__, __FUNCTION__, NXMODNAME(), INFO, __VA_ARGS__)); \
        ((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback(le.get_log_level(), le.get_message().c_str()); \
    }

#define LOGDRV(MGR, LEVEL, INFO, ...) \
    if(NULL!=MGR && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback && ((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback(LEVEL)) { \
        NX::dbg::log_item le(LEVEL, NX::dbg::build_error_msg(NXDRVNAME(), INFO, __VA_ARGS__)); \
        ((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback(le.get_log_level(), le.get_message().c_str()); \
    }

#define LOGDRVEX(MGR, LEVEL, INFO, ...) \
    if(NULL!=MGR && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback && ((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback(LEVEL)) { \
        NX::dbg::log_item le(LEVEL, NX::dbg::build_error_msg2(__FILE__, __LINE__, __FUNCTION__, NXDRVNAME(), INFO, __VA_ARGS__)); \
        ((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback(le.get_log_level(), le.get_message().c_str()); \
    }

//
//  Detail Output
//

#define LOGMAN_ASSERT(MGR, INFO, ...) \
    if(NULL!=MGR && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback && ((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback(NX::dbg::LL_CRITICAL)) { \
		NX::dbg::log_item le(NX::dbg::LL_CRITICAL, NX::dbg::build_error_msg2(__FILE__, __LINE__, __FUNCTION__, NXMODNAME(), INFO, __VA_ARGS__)); \
        ((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback(le.get_log_level(), le.get_message().c_str()); \
    }

#define LOGMAN_ERROR(MGR, INFO, ...) \
    if(NULL!=MGR && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback && ((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback(NX::dbg::LL_ERROR)) { \
        NX::dbg::log_item le(NX::dbg::LL_ERROR, NX::dbg::build_error_msg(NXMODNAME(), INFO, __VA_ARGS__)); \
        ((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback(le.get_log_level(), le.get_message().c_str()); \
    }

#define LOGMAN_WARN(MGR, INFO, ...) \
    if(NULL!=MGR && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback && ((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback(NX::dbg::LL_WARNING)) { \
        NX::dbg::log_item le(NX::dbg::LL_WARNING, NX::dbg::build_error_msg(NXMODNAME(), INFO, __VA_ARGS__)); \
        ((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback(le.get_log_level(), le.get_message().c_str()); \
    }

#define LOGMAN_INFO(MGR, INFO, ...) \
    if(NULL!=MGR && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback && ((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback(NX::dbg::LL_INFO)) { \
        NX::dbg::log_item le(NX::dbg::LL_INFO, NX::dbg::build_error_msg2(NXMODNAME(), INFO, __VA_ARGS__)); \
        ((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback(le.get_log_level(), le.get_message().c_str()); \
    }

#define LOGMAN_DBG(MGR, INFO, ...) \
    if(NULL!=MGR && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback && ((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback(NX::dbg::LL_DEBUG)) { \
        NX::dbg::log_item le(NX::dbg::LL_DEBUG, NX::dbg::build_error_msg2(__FILE__, __LINE__, __FUNCTION__, NXMODNAME(), INFO, __VA_ARGS__)); \
        ((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback(le.get_log_level(), le.get_message().c_str()); \
    }

#define LOGDRV_ASSERT(MGR, INFO, ...) \
    if(NULL!=MGR && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback && ((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback(NX::dbg::LL_CRITICAL)) { \
        NX::dbg::log_item le(NX::dbg::LL_CRITICAL, NX::dbg::build_error_msg2(__FILE__, __LINE__, __FUNCTION__, NXDRVNAME(), INFO, __VA_ARGS__)); \
        ((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback(le.get_log_level(), le.get_message().c_str()); \
    }

#define LOGDRV_ERROR(MGR, INFO, ...) \
    if(NULL!=MGR && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback && ((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback(NX::dbg::LL_ERROR)) { \
        NX::dbg::log_item le(NX::dbg::LL_ERROR, NX::dbg::build_error_msg(NXDRVNAME(), INFO, __VA_ARGS__)); \
        ((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback(le.get_log_level(), le.get_message().c_str()); \
    }

#define LOGDRV_WARN(MGR, INFO, ...) \
    if(NULL!=MGR && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback && ((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback(NX::dbg::LL_WARNING)) { \
        NX::dbg::log_item le(NX::dbg::LL_WARNING, NX::dbg::build_error_msg(NXDRVNAME(), INFO, __VA_ARGS__)); \
        ((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback(le.get_log_level(), le.get_message().c_str()); \
    }

#define LOGDRV_INFO(MGR, INFO, ...) \
    if(NULL!=MGR && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback && ((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback(NX::dbg::LL_INFO)) { \
        NX::dbg::log_item le(NX::dbg::LL_INFO, NX::dbg::build_error_msg(NXDRVNAME(), INFO, __VA_ARGS__)); \
        ((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback(le.get_log_level(), le.get_message().c_str()); \
    }

#define LOGDRV_DBG(MGR, INFO, ...) \
    if(NULL!=MGR && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback && NULL!=((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback && ((PNXRMFLT_MANAGER)MGR)->DebugDumpCheckLevelCallback(NX::dbg::LL_DEBUG)) { \
        NX::dbg::log_item le(NX::dbg::LL_DEBUG, NX::dbg::build_error_msg2(__FILE__, __LINE__, __FUNCTION__, NXDRVNAME(), INFO, __VA_ARGS__)); \
        ((PNXRMFLT_MANAGER)MGR)->DebugDumpCallback(le.get_log_level(), le.get_message().c_str()); \
    }



#endif