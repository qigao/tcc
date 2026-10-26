// P1-C: real-but-SANDBOX-ONLY CAtlModuleT::RegisterServer writes/rollback.
// Uses pinned test-patched OpenATL and a new disposable HKCU key per process.
#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <windows.h>
#include <objbase.h>
#include <objidl.h>
#include <ocidl.h>
#include <cstdio>
#include <cwchar>
#include <string>
#include <atlbase.h>
#include <atlcom.h>
static_assert(sizeof(void*)==8,"native Windows x64/ARM64 COM registrar test");
#ifdef __RATL__
#error "Never mix RATL and OpenATL headers"
#endif
const CLSID P1B_CLASS_A={
    0x1ab0965f,0x76cd,0x48ae,{0xb9,0xa8,0x44,0x21,0x96,0x0a,0x35,0x82}};
extern "C" const CLSID P1B_CLASS_B;
static const CLSID UNKNOWN_CLASS={
    0x720ec938,0xc085,0x48b2,{0xa1,0xac,0x9a,0x8e,0xc7,0x33,0x69,0x7d}};
class TestModule : public ATL::CAtlModuleT<TestModule> {};
static TestModule module;
class AutoClassA : public ATL::CComObjectRootEx<ATL::CComSingleThreadModel>,
                   public IPersist {
public:
    BEGIN_COM_MAP(AutoClassA)
        COM_INTERFACE_ENTRY_IID(IID_IPersist,IPersist)
    END_COM_MAP()
    DECLARE_NOT_AGGREGATABLE(AutoClassA)
    STDMETHOD(GetClassID)(CLSID* id) override {
        if(!id)return E_POINTER;
        *id=P1B_CLASS_A;return S_OK;
    }
};
OBJECT_ENTRY_AUTO(P1B_CLASS_A,AutoClassA)

static bool classKey(REFCLSID clsid,wchar_t* path,unsigned length) {
    wchar_t guid[40]={};
    if(!::StringFromGUID2(clsid,guid,40))return false;
    unsigned n=0;const wchar_t* prefix=L"CLSID\\";
    for(unsigned i=0;prefix[i];++i)path[n++]=prefix[i];
    for(unsigned i=0;guid[i];++i) {
        if(n+1>=length)return false;
        path[n++]=guid[i];
    }
    path[n]=0;return true;
}
static bool contains(HKEY root,REFCLSID clsid) {
    wchar_t path[64]={};
    if(!classKey(clsid,path,64))return false;
    HKEY h=nullptr;
    const LSTATUS hr=::RegOpenKeyExW(root,path,0,KEY_READ,&h);
    if(h)::RegCloseKey(h);
    return hr==ERROR_SUCCESS;
}
static bool hasValue(HKEY root,REFCLSID clsid) {
    wchar_t path[84]={};
    if(!classKey(clsid,path,84))return false;
    unsigned i=::lstrlenW(path);
    const wchar_t* end=L"\\LocalServer32";
    for(unsigned j=0;end[j];++j)path[i++]=end[j];
    path[i]=0;
    HKEY h=nullptr;
    if(::RegOpenKeyExW(root,path,0,KEY_READ,&h)!=ERROR_SUCCESS)
        return false;
    wchar_t value[80]={};
    DWORD type=0,size=sizeof(value);
    const LSTATUS err=::RegQueryValueExW(h,nullptr,nullptr,&type,
        reinterpret_cast<BYTE*>(value),&size);
    ::RegCloseKey(h);
    return err==ERROR_SUCCESS && type==REG_SZ &&
        ::lstrcmpW(value,L"TEST-ONLY-NOT-AN-EXECUTABLE")==0 &&
        size==static_cast<DWORD>((::lstrlenW(value)+1)*sizeof(wchar_t));
}
static bool absent(HKEY root,REFCLSID id){return !contains(root,id);}
struct Scope {
    std::wstring path;
    HKEY root=nullptr;
    DWORD proof=0;
    bool owned=false,redirected=false,armed=false;
    bool create() {
        GUID id={};
        if(FAILED(::CoCreateGuid(&id)))return false;
        wchar_t text[40]={};
        if(!::StringFromGUID2(id,text,40))return false;
        path=L"qigao_tcc_ATL_P1C_";
        path+=text;
        proof=id.Data1 ^ 0xA1339C47u;
        if(proof==0)proof=0x13777BC5u;
        DWORD disp=0;
        LSTATUS hr=::RegCreateKeyExW(HKEY_CURRENT_USER,path.c_str(),
            0,nullptr,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,nullptr,
            &root,&disp);
        if(hr!=ERROR_SUCCESS || disp!=REG_CREATED_NEW_KEY) {
            if(root){::RegCloseKey(root);root=nullptr;}
            path.clear(); // never delete a collision
            return false;
        }
        owned=true;
        return ::RegSetValueExW(root,L"P1C_REDIRECT_PROOF",0,REG_DWORD,
            reinterpret_cast<const BYTE*>(&proof),sizeof(proof))==ERROR_SUCCESS;
    }
    bool authorize() {
        if(!owned || ::RegOverridePredefKey(HKEY_CLASSES_ROOT,root)!=ERROR_SUCCESS)
            return false;
        redirected=true;
        if(ATL::AtlSandboxRegistryArm(root,proof)!=S_OK)
            return false;
        armed=true;return true;
    }
    bool cleanup() {
        if(armed) {
            if(ATL::AtlSandboxRegistryDisarm()!=S_OK)return false;
            armed=false;
        }
        if(redirected) {
            if(::RegOverridePredefKey(HKEY_CLASSES_ROOT,nullptr)!=ERROR_SUCCESS)
                return false;
            redirected=false;
        }
        if(root){::RegCloseKey(root);root=nullptr;}
        if(!owned)return true;
        if(::RegDeleteTreeW(HKEY_CURRENT_USER,path.c_str())!=ERROR_SUCCESS)
            return false;
        HKEY stale=nullptr;
        LSTATUS hr=::RegOpenKeyExW(HKEY_CURRENT_USER,path.c_str(),0,KEY_READ,&stale);
        if(stale)::RegCloseKey(stale);
        if(hr!=ERROR_FILE_NOT_FOUND || stale)return false;
        owned=false;path.clear();return true;
    }
    ~Scope() {
        if(owned || root || redirected || armed)
            if(!cleanup())std::fputs("REGISTRAR_CLEANUP_FAILED\n",stderr);
    }
};
static int failed(int code,const char* part) {
    std::fprintf(stderr,"ATL_P1C_REAL_FAIL code=%d step=%s\n",code,part);
    return code;
}
static DWORD WINAPI foreignRegistration(void* out) {
    *static_cast<HRESULT*>(out)=module.RegisterServer();
    return 0;
}
int main() {
    HRESULT apartment=::CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    if(FAILED(apartment))return failed(1,"apartment");
    Scope scope;
    int error=0;
    if(module.RegisterServer()!=E_NOTIMPL)error=10;
    if(!error && !scope.create())error=11;
    if(!error && ATL::AtlSandboxRegistryArm(scope.root,scope.proof)!=E_ACCESSDENIED)
        error=12;
    if(!error && !scope.authorize())error=13;
    if(!error && (module.RegisterServer(TRUE)!=E_NOTIMPL ||
                   module.UnregisterServer(TRUE)!=E_NOTIMPL))error=14;
    if(!error && module.RegisterServer(FALSE,&UNKNOWN_CLASS)!=CLASS_E_CLASSNOTAVAILABLE)
        error=15;
    if(!error) {
        HRESULT foreign=S_OK;
        HANDLE worker=::CreateThread(nullptr,0,foreignRegistration,
                                     &foreign,0,nullptr);
        if(!worker)error=16;
        else {
            const DWORD wait=::WaitForSingleObject(worker,5000);
            ::CloseHandle(worker);
            if(wait!=WAIT_OBJECT_0 || foreign!=RPC_E_WRONG_THREAD)
                error=17;
        }
    }
    if(!error && (ATL::AtlSandboxRegistryFailAt(1)!=S_OK ||
        module.RegisterServer()!=E_OUTOFMEMORY ||
        !absent(scope.root,P1B_CLASS_A) || !absent(scope.root,P1B_CLASS_B)))
        error=18;
    if(!error && (ATL::AtlSandboxRegistryFailAt(-1)!=S_OK ||
        module.RegisterServer()!=S_OK ||
        !hasValue(scope.root,P1B_CLASS_A) ||
        !hasValue(scope.root,P1B_CLASS_B)))error=19;
    if(!error && (module.RegisterServer()!=HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) ||
        !hasValue(scope.root,P1B_CLASS_A) ||
        !hasValue(scope.root,P1B_CLASS_B)))error=20;
    if(!error && (module.UnregisterServer(FALSE)!=S_OK ||
        !absent(scope.root,P1B_CLASS_A) ||
        !absent(scope.root,P1B_CLASS_B) ||
        module.UnregisterServer(FALSE)!=S_OK))error=21;
    if(!error && (module.RegisterServer(FALSE,&P1B_CLASS_A)!=S_OK ||
        !hasValue(scope.root,P1B_CLASS_A) || !absent(scope.root,P1B_CLASS_B) ||
        module.UnregisterServer(FALSE,&P1B_CLASS_A)!=S_OK ||
        !absent(scope.root,P1B_CLASS_A)))error=22;
    if(!error && ATL::AtlSandboxRegistryDisarm()!=S_OK)error=23;
    else if(!error)scope.armed=false;
    if(!error && module.RegisterServer()!=E_NOTIMPL)error=24;
    const bool clean=scope.cleanup();
    ::CoUninitialize();
    if(error)return failed(error,"registrar_contract");
    if(!clean)return failed(25,"sandbox_cleanup");
    std::puts("ATL_P1C_REAL_OK guarded=1 writes=2 rollback=1 duplicate=1 targeted=1 clean=1");
    return 0;
}
