
#include <Windows.h>
#include "registry_service_impl.h"
#include "nudf\dbglog.hpp"
#include "nudf\string.hpp"
#include "nudf\winutil.hpp"
#include <nudf\conversion.hpp>


const wstring NX_STR_FUNCTION = L"function";
const wstring NX_STR_GET_VALUE = L"get_value";
const wstring NX_STR_SET_VALUE = L"set_value";
const wstring NX_STR_DELETE_KEY = L"delete_key";
const wstring NX_STR_ROOTKEY = L"rootkey";
const wstring NX_STR_SUBKEY = L"subkey";
const wstring NX_STR_ITEMNAME = L"itemname";
const wstring NX_STR_ITEMVALUE = L"itemvalue";
const wstring NX_STR_VALUETYPE = L"valuetype";

const char NX_EXCEPTION_FORMAT[] =
    "{\"code\":%d, \"message\":\"exception : (%s) \" }";

const char NX_NORMAL_FORMAT[] = "{\"code\":%d, \"message\": \"%s\" }";

cregistry_service_impl::cregistry_service_impl(const wstring &strWinUserId)
    : m_strWindowsUserId(strWinUserId) {}

cregistry_service_impl::~cregistry_service_impl() {}

string
cregistry_service_impl::handle_registry_command(const NX::json_value &reg_cmd) {
  LOGINFO(NX::string_formater(
      L"cregistry_service_impl::handle_registry_command enter"));
  string response;
  if (!reg_cmd.as_object().has_field(NX_STR_FUNCTION)) {
    response = NX::string_formater(NX_NORMAL_FORMAT, ERROR_INVALID_DATA,
                                   "Error: not find function field");
    LOGERROR(NX::string_formater(
        L"cregistry_service_impl::handle_registry_command failed: %s",
        L"not find function"));
    return move(response);
  }

  wstring strFuncName = reg_cmd.as_object().at(NX_STR_FUNCTION).as_string();
  if (0 == strFuncName.compare(NX_STR_GET_VALUE)) {
    response = handle_get_value(reg_cmd);
    return move(response);
  } else if (0 == strFuncName.compare(NX_STR_SET_VALUE)) {
    response = handle_set_value(reg_cmd);
    return move(response);
  } else if (0 == strFuncName.compare(NX_STR_DELETE_KEY)) {
    response = handle_delete_key(reg_cmd);
    return move(response);
  } else {
    // string strName = NX::conv::utf16toutf8();
    response = NX::string_formater(NX_NORMAL_FORMAT, ERROR_INVALID_DATA,
                                   "Error: not find function");
    LOGERROR(NX::string_formater(
        L"cregistry_service_impl::handle_registry_command failed: %s",
        L"not find function"));
    return move(response);
  }
}

string cregistry_service_impl::handle_get_value(const NX::json_value &reg_cmd) {
  LOGINFO(
      NX::string_formater(L"cregistry_service_impl::handle_get_value enter"));

  string response;
  if (!reg_cmd.as_object().has_field(NX_STR_FUNCTION) ||
      !reg_cmd.as_object().has_field(NX_STR_ROOTKEY) ||
      !reg_cmd.as_object().has_field(NX_STR_SUBKEY) ||
      !reg_cmd.as_object().has_field(NX_STR_ITEMNAME) ||
      !reg_cmd.as_object().has_field(NX_STR_VALUETYPE)) {
    response = NX::string_formater(NX_NORMAL_FORMAT, ERROR_INVALID_DATA,
                                   "Error: not find function field");
    LOGERROR(NX::string_formater(
        L"cregistry_service_impl::handle_get_value error: %s",
        L"input parameter check failed"));
    return move(response);
  }

  uint32_t u32Root = reg_cmd.as_object().at(NX_STR_ROOTKEY).as_uint32();
  wstring strKey = reg_cmd.as_object().at(NX_STR_SUBKEY).as_string();
  wstring strItemName = reg_cmd.as_object().at(NX_STR_ITEMNAME).as_string();
  uint32_t u32Type = reg_cmd.as_object().at(NX_STR_VALUETYPE).as_uint32();

  if ((uint32_t)HKEY_CURRENT_USER == u32Root) {
    strKey = m_strWindowsUserId + L"\\" + strKey;
    u32Root = (uint32_t)HKEY_USERS;
  }

  response = get_value(u32Root, strKey, strItemName, u32Type);
  return move(response);
}

string cregistry_service_impl::get_value(uint32_t u32Root,
                                         const wstring &strKey,
                                         const wstring &strItemName,
                                         uint32_t u32Type) {
  LOGINFO(NX::string_formater(
      L"cregistry_service_impl::get_value enter u32Root:%d key:%s itemname:%s "
      L"u32type:%d",
      u32Root, strKey.c_str(), strItemName.c_str(), u32Type));
  try {
    string response;
    NX::win::reg_key key;
    HKEY hRoot = ((HKEY)(ULONG_PTR)(u32Root)); // HKEY_LOCAL_MACHINE;
    if (key.exist(hRoot, strKey)) {
      NX::win::reg_key::reg_position pos = NX::win::reg_key::reg_position::
          reg_default; //(NX::win::reg_key::reg_position)nRegPos;
      key.open(hRoot, strKey, pos, true);

      NX::json_value v = NX::json_value::create_object();
      v[L"code"] = NX::json_value(0);
      v[L"message"] = NX::json_value(L"success");

      NX::json_value objReturn = NX::json_value::create_object();
      objReturn[NX_STR_VALUETYPE] = NX::json_value(u32Type);

      if (REG_DWORD == u32Type) {
        unsigned long ulValue = 0;
        key.read_value(strItemName, &ulValue);
        objReturn[NX_STR_ITEMVALUE] = NX::json_value((unsigned int)ulValue);
      } else if (REG_QWORD == u32Type) {
        uint64_t u64Value = 0;
        key.read_value(strItemName, &u64Value);
        objReturn[NX_STR_ITEMVALUE] = NX::json_value(u64Value);
      } else if (REG_SZ == u32Type) {
        wstring strItemValue;
        key.read_value(strItemName, strItemValue);
        objReturn[NX_STR_ITEMVALUE] = NX::json_value(strItemValue);
      }

      v[L"return"] = objReturn;
      wstring strJson = v.serialize();
      response = NX::conversion::utf16_to_utf8(strJson);
      return move(response);
    } else {
      response = NX::string_formater(NX_NORMAL_FORMAT, ERROR_INVALID_DATA,
                                     "Error: not found");
      LOGERROR(NX::string_formater(L"get_value: %s: %s %s", L"get_value",
                                   strKey.c_str(), L"open failed"));
      return move(response);
    }
  } catch (const exception &e) {
    string response =
        NX::string_formater(NX_EXCEPTION_FORMAT, ERROR_INVALID_DATA, e.what());
    DWORD dwErr = ::GetLastError();
    LOGERROR(NX::string_formater(
        "cregistry_service_impl::get_value exception : %s errcode:%d",
        response.c_str(), dwErr));
    return move(response);
  }
}

string cregistry_service_impl::handle_set_value(const NX::json_value &reg_cmd) {
  LOGINFO(
      NX::string_formater(L"cregistry_service_impl::handle_set_value enter"));

  string response;
  if (!reg_cmd.as_object().has_field(NX_STR_FUNCTION) ||
      !reg_cmd.as_object().has_field(NX_STR_ROOTKEY) ||
      !reg_cmd.as_object().has_field(NX_STR_SUBKEY) ||
      !reg_cmd.as_object().has_field(NX_STR_ITEMNAME) ||
      !reg_cmd.as_object().has_field(NX_STR_ITEMVALUE) ||
      !reg_cmd.as_object().has_field(NX_STR_VALUETYPE)) {
    response = NX::string_formater(NX_NORMAL_FORMAT, ERROR_INVALID_DATA,
                                   "Error: not find function field");
    LOGERROR(NX::string_formater(
        L"cregistry_service_impl::handle_set_value error: %s",
        L"input parameter check failed"));
    return move(response);
  }

  uint32_t u32Root = reg_cmd.as_object().at(NX_STR_ROOTKEY).as_uint32();
  wstring strKey = reg_cmd.as_object().at(NX_STR_SUBKEY).as_string();
  wstring strItemName = reg_cmd.as_object().at(NX_STR_ITEMNAME).as_string();
  uint32_t u32Type = reg_cmd.as_object().at(NX_STR_VALUETYPE).as_uint32();

  if ((uint32_t)HKEY_CURRENT_USER == u32Root) {
    strKey = m_strWindowsUserId + L"\\" + strKey;
    u32Root = (uint32_t)HKEY_USERS;
  }

  if (REG_DWORD == u32Type) {
    uint32_t u32ItemValue =
        reg_cmd.as_object().at(NX_STR_ITEMVALUE).as_uint32();
    response = set_value(u32Root, strKey, strItemName, u32ItemValue);
    return move(response);
  } else if (REG_QWORD == u32Type) {
    uint64_t u64ItemValue =
        reg_cmd.as_object().at(NX_STR_ITEMVALUE).as_uint64();
    response = set_value(u32Root, strKey, strItemName, u64ItemValue);
    return move(response);
  } else if (REG_SZ == u32Type) {
    wstring strItemValue = reg_cmd.as_object().at(NX_STR_ITEMVALUE).as_string();
    response = set_value(u32Root, strKey, strItemName, strItemValue, true);
    return move(response);
  } else if (REG_EXPAND_SZ == u32Type) {
    wstring strItemValue = reg_cmd.as_object().at(NX_STR_ITEMVALUE).as_string();
    response = set_value(u32Root, strKey, strItemName, strItemValue, false);
    return move(response);
  } else {
    response = NX::string_formater(NX_NORMAL_FORMAT, ERROR_INVALID_DATA,
                                   "Error: type not found");
    LOGERROR(NX::string_formater(
        L"handle_set_value: function: set_value, key:%s valuetype:%lu",
        L"handle_set_value", strKey.c_str(), u32Type));
    return move(response);
  }
}

string cregistry_service_impl::set_value(uint32_t u32Root,
                                         const wstring &strKey,
                                         const wstring &strItemName,
                                         uint32_t u32ItemValue) {
  LOGINFO(NX::string_formater(
      L"cregistry_service_impl::set_value enter u32Root:%d key:%s itemname:%s "
      L"itemvalue:%lu",
      u32Root, strKey.c_str(), strItemName.c_str(), u32ItemValue));

  try {
    string response;
    NX::win::reg_key key;
    HKEY hRoot = ((HKEY)(ULONG_PTR)(u32Root));

    if (!key.exist(hRoot, strKey)) {
      key.create(hRoot, strKey, NX::win::reg_key::reg_position::reg_default);
      key.set_value(strItemName, (unsigned long)u32ItemValue);
      response = NX::string_formater(NX_NORMAL_FORMAT, 0, "success");
      return move(response);
    } else {
      key.open(hRoot, strKey, NX::win::reg_key::reg_position::reg_default,
               false);
      key.set_value(strItemName, (unsigned long)u32ItemValue);
      response = NX::string_formater(NX_NORMAL_FORMAT, 0, "success");
      return move(response);
    }
  } catch (const exception &e) {
    string response =
        NX::string_formater(NX_EXCEPTION_FORMAT, ERROR_INVALID_DATA, e.what());

    DWORD dwErr = ::GetLastError();
    LOGERROR(NX::string_formater(
        "cregistry_service_impl::set_value exception : %s errcode:%d",
        response.c_str(), dwErr));
    return move(response);
  }
}

string cregistry_service_impl::set_value(uint32_t u32Root,
                                         const wstring &strKey,
                                         const wstring &strItemName,
                                         uint64_t u64ItemValue) {
  LOGINFO(NX::string_formater(
      L"cregistry_service_impl::set_value enter u32Root:%d key:%s itemname:%s "
      L"itemvalue:%llu",
      u32Root, strKey.c_str(), strItemName.c_str(), u64ItemValue));
  string response;

  try {
    NX::win::reg_key key;
    HKEY hRoot = ((HKEY)(ULONG_PTR)(u32Root));

    if (!key.exist(hRoot, strKey)) {
      key.create(hRoot, strKey, NX::win::reg_key::reg_position::reg_default);
      key.set_value(strItemName, u64ItemValue);
      response = NX::string_formater(NX_NORMAL_FORMAT, 0, "success");
      return move(response);
    } else {
      key.open(hRoot, strKey, NX::win::reg_key::reg_position::reg_default,
               false);
      key.set_value(strItemName, u64ItemValue);
      response = NX::string_formater(NX_NORMAL_FORMAT, 0, "success");
      return move(response);
    }
  } catch (const exception &e) {
    string response =
        NX::string_formater(NX_EXCEPTION_FORMAT, ERROR_INVALID_DATA, e.what());
    DWORD dwErr = ::GetLastError();
    LOGERROR(NX::string_formater(
        "cregistry_service_impl::set_value exception : %s errcode:%d",
        response.c_str(), dwErr));
    return move(response);
  }
}

string cregistry_service_impl::set_value(uint32_t u32Root,
                                         const wstring &strKey,
                                         const wstring &strItemName,
                                         const wstring &strItemValue,
                                         bool bExpandable) {
  LOGINFO(NX::string_formater(
      L"cregistry_service_impl::set_value enter u32Root:%d key:%s itemname:%s "
      L"itemvalue:%s",
      u32Root, strKey.c_str(), strItemName.c_str(), strItemValue.c_str()));
  try {
    string response;
    NX::win::reg_key key;
    HKEY hRoot = ((HKEY)(ULONG_PTR)(u32Root));

    if (!key.exist(hRoot, strKey)) {
      key.create(hRoot, strKey, NX::win::reg_key::reg_position::reg_default);
      if (!strItemName.empty() || !strItemValue.empty()) {
        key.set_value(strItemName, strItemValue, bExpandable);
      }
      response = NX::string_formater(NX_NORMAL_FORMAT, 0, "success");
      return move(response);
    } else {
      key.open(hRoot, strKey, NX::win::reg_key::reg_position::reg_default,
               false);
      if (!strItemName.empty() || !strItemValue.empty()) {
        key.set_value(strItemName, strItemValue, bExpandable);
      }
      response = NX::string_formater(NX_NORMAL_FORMAT, 0, "success");
      return move(response);
    }
  } catch (const exception &e) {
    string response =
        NX::string_formater(NX_EXCEPTION_FORMAT, ERROR_INVALID_DATA, e.what());
    DWORD dwErr = ::GetLastError();
    LOGERROR(NX::string_formater(
        "cregistry_service_impl::set_value exception : %s errcode:%d",
        response.c_str(), dwErr));
    return move(response);
  }
}

string
cregistry_service_impl::handle_delete_key(const NX::json_value &reg_cmd) {
  LOGINFO(
      NX::string_formater(L"cregistry_service_impl::handle_delete_key enter"));

  string response;
  if (!reg_cmd.as_object().has_field(NX_STR_FUNCTION) ||
      !reg_cmd.as_object().has_field(NX_STR_ROOTKEY) ||
      !reg_cmd.as_object().has_field(NX_STR_SUBKEY)) {
    response = NX::string_formater(NX_NORMAL_FORMAT, ERROR_INVALID_DATA,
                                   "Error: not find function field");
    LOGERROR(NX::string_formater(
        L"cregistry_service_impl::handle_delete_key error: %s",
        L"input parameter check failed"));
    return move(response);
  }

  uint32_t u32Root = reg_cmd.as_object().at(NX_STR_ROOTKEY).as_uint32();
  wstring strKey = reg_cmd.as_object().at(NX_STR_SUBKEY).as_string();

  if ((uint32_t)HKEY_CURRENT_USER == u32Root) {
    strKey = m_strWindowsUserId + L"\\" + strKey;
    u32Root = (uint32_t)HKEY_USERS;
  }

  response = delete_key(u32Root, strKey);
  return move(response);
}

string cregistry_service_impl::delete_key(uint32_t u32Root,
                                          const wstring &strKey) {
  LOGINFO(NX::string_formater(
      L"cregistry_service_impl::delete_key enter u32Root:%lu key:%s ", u32Root,
      strKey.c_str()));
  try {
    string response;
    NX::win::reg_key key;
    HKEY hRoot = ((HKEY)(ULONG_PTR)(u32Root));

    key.remove(hRoot, strKey);
    response = NX::string_formater(NX_NORMAL_FORMAT, 0, "success");
    return move(response);
  } catch (const exception &e) {
    string response =
        NX::string_formater(NX_EXCEPTION_FORMAT, ERROR_INVALID_DATA, e.what());
    DWORD dwErr = ::GetLastError();
    LOGERROR(NX::string_formater(
        "cregistry_service_impl::delete_key exception : %s errcode:%d",
        response.c_str(), dwErr));
    return move(response);
  }
}
