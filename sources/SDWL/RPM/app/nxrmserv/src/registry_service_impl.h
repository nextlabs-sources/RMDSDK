#ifndef __NXRMSERV_REGISTRY_SERVICE_IMPL_H__
#define __NXRMSERV_REGISTRY_SERVICE_IMPL_H__

#include <string>
#include "nudf\json.hpp"

using namespace std;

class cregistry_service_impl {
public:
  cregistry_service_impl(const wstring &strWinUserId);
  ~cregistry_service_impl();

public:
  string handle_registry_command(const NX::json_value &reg_cmd);

protected:
  string handle_get_value(const NX::json_value &reg_cmd);

  string get_value(uint32_t u32Root, const wstring &strKey,
                   const wstring &strItemName, uint32_t u32Type);

  string handle_set_value(const NX::json_value &reg_cmd);

  string set_value(uint32_t u32Root, const wstring &strKey,
                   const wstring &strItemName, uint32_t u32ItemValue);

  string set_value(uint32_t u32Root, const wstring &strKey,
                   const wstring &strItemName, uint64_t u64ItemValue);

  string set_value(uint32_t u32Root, const wstring &strKey,
                   const wstring &strItemName, const wstring &strItemValue,
                   bool bExpand);

  string handle_delete_key(const NX::json_value &reg_cmd);

  string delete_key(uint32_t u32Root, const wstring &strKey);

protected:
  wstring m_strWindowsUserId;
};


#endif