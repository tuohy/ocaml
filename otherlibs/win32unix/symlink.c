/***********************************************************************/
/*                                                                     */
/*                                OCaml                                */
/*                                                                     */
/*               David Allsopp, MetaStack Solutions Ltd.               */
/*                                                                     */
/*  Copyright 2015 MetaStack Solutions Ltd.  All rights reserved.      */
/*  This file is distributed under the terms of the GNU Library        */
/*  General Public License, with the special exception on linking      */
/*  described in file ../../LICENSE.                                   */
/*                                                                     */
/***********************************************************************/

/*
 * Windows Vista functions enabled
 */
#define _WIN32_WINNT 0x0600

#include <caml/mlvalues.h>
#include <caml/memory.h>
#include <caml/alloc.h>
#include <caml/fail.h>
#include <caml/signals.h>
#include "unixsupport.h"

CAMLprim value unix_symlink(value to_dir, value source, value dest)
{
  CAMLparam3(to_dir, source, dest);
  DWORD flags = (Bool_val(to_dir) ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0);
  BOOL result;

  caml_enter_blocking_section();
  result = CreateSymbolicLink(String_val(dest), String_val(source), flags);
  caml_leave_blocking_section();

  if (!result) {
    win32_maperr(GetLastError());
    uerror("symlink", dest);
  }

  CAMLreturn(Val_unit);
}

#define luid_eq(l, r) (l.LowPart == r.LowPart && l.HighPart == r.HighPart)

CAMLprim value unix_has_symlink(value unit)
{
  CAMLparam1(unit);
  HANDLE hProcess = GetCurrentProcess();
  BOOL result = FALSE;

  if (OpenProcessToken(hProcess, TOKEN_READ, &hProcess)) {
    LUID seCreateSymbolicLinkPrivilege;

    if (LookupPrivilegeValue(NULL,
                             SE_CREATE_SYMBOLIC_LINK_NAME,
                             &seCreateSymbolicLinkPrivilege)) {
      DWORD length;

      if (!GetTokenInformation(hProcess, TokenPrivileges, NULL, 0, &length)) {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
          TOKEN_PRIVILEGES* privileges = (TOKEN_PRIVILEGES*)malloc(length);
          if (GetTokenInformation(hProcess,
                                  TokenPrivileges,
                                  privileges,
                                  length,
                                  &length)) {
            DWORD count = privileges->PrivilegeCount;

            if (count) {
              LUID_AND_ATTRIBUTES* privs = privileges->Privileges;
              while (count-- && !(result = luid_eq(privs->Luid, seCreateSymbolicLinkPrivilege)))
                privs++;
            }
          }

          free(privileges);
        }
      }
    }

    CloseHandle(hProcess);
  }

  CAMLreturn(Val_bool(result));
}
