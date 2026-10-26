/* WTL8 full-header compatibility probe for llvm-mingw x64 and ARM64.
 * Required distinction: a passing Winx CWindowImpl callback test does not
 * imply every WTL control, dialog or frame header parses on modern Clang.
 * No product GUI is linked by this test.
 */
#if !defined(_WIN64)
#error "Win64 WTL8 header coverage must target x64 or ARM64"
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef WINVER
#define WINVER 0x0601
#endif
#ifndef _WIN32_IE
#define _WIN32_IE 0x0601
#endif
#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlgdi.h>
#include <atlmisc.h>
#include <atlcrack.h>

static_assert(sizeof(WTL::CMessageLoop) >= sizeof(void*), "WTL loop layout");
static_assert(sizeof(WTL::CEdit) >= sizeof(void*), "WTL edit layout");
static_assert(sizeof(WTL::CListViewCtrl) >= sizeof(void*), "WTL list control");
static_assert(sizeof(WTL::CTreeViewCtrl) >= sizeof(void*), "WTL tree control");
static_assert(sizeof(void*) == 8u, "Native 64-bit WTL must not truncate HWND");
int poc_wtl64_headers_compile_only(void) { return 1; }
