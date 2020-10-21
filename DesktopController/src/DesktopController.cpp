#include "DesktopController.h"
#include "DesktopIcon.h"

#include <iostream>

using namespace std;
using namespace DeskCtrlUtil;

DesktopController::DesktopController() 
{
    HRESULT result = CoInitialize(NULL);
    if (!SUCCEEDED(result))
        throwHRESULTException("CoInitialize", result);

    // Extract IShellView, IFolderView and IShellFolder from the desktop.
    findDesktopShellView(shellview);
    findDesktopFolderView(shellview, IID_PPV_ARGS(&folderview));
    result = folderview->GetFolder(IID_PPV_ARGS(&shellfolder));
    if (!SUCCEEDED(result))
        throwHRESULTException("GetFolder", result);
}

DesktopController::~DesktopController()
{
}

void DesktopController::findDesktopShellView(CComPtr<IShellView>& shellViewOut)
{
    CComPtr<IShellWindows> shellWindows;
    HRESULT result = shellWindows.CoCreateInstance(CLSID_ShellWindows);
    if (!SUCCEEDED(result))
        throwHRESULTException("CoCreateInstance", result);

    CComVariant idloc(CSIDL_DESKTOP);
    CComVariant emptyloc;
    CComPtr<IDispatch> dispatch;
    long hwnd;

    result = shellWindows->FindWindowSW(
        &idloc,
        &emptyloc,
        SWC_DESKTOP,
        &hwnd,
        SWFO_NEEDDISPATCH,
        &dispatch);
    if (!SUCCEEDED(result))
        throwHRESULTException("FindWindowSW", result);

    CComPtr<IShellBrowser> shellBrowser;
    result = CComQIPtr<IServiceProvider>(dispatch)->QueryService(
        SID_STopLevelBrowser,
        IID_PPV_ARGS(&shellBrowser));
    if (!SUCCEEDED(result))
        throwHRESULTException("QueryService", result);

    result = shellBrowser->QueryActiveShellView(&shellViewOut);
    if (!SUCCEEDED(result))
        throwHRESULTException("QueryActiveShellView", result);
}

void DesktopController::findDesktopFolderView(CComPtr<IShellView> shellViewArg, REFIID riid, void** ppv)
{
    HRESULT result = shellViewArg->QueryInterface(riid, ppv);
    if (!SUCCEEDED(result))
        throwHRESULTException("QueryInterface", result);
}

// NOTE: pybind11 doesn't recognise a reference to DesktopIcon (const DesktopIcon&) here, 
// instead it ignores the reference and attempts to copy, so a pointer will have to do.
void DesktopController::enumerateIcons(const function<void(const DesktopIcon*)>& callback)
{
    if (!callback)
        throw runtime_error("Invalid callback in enumerateIcons().");

    CComPtr<IEnumIDList> idlist;
    HRESULT result = folderview->Items(SVGIO_ALLVIEW, IID_PPV_ARGS(&idlist));
    if (!SUCCEEDED(result))
        throwHRESULTException("Items", result);

    // Note: itemid is freed by the destructor of DesktopIcon.
    for (CComHeapPtr<ITEMID_CHILD> itemid; idlist->Next(1, &itemid, nullptr) == S_OK; )
    {
        DesktopIcon icon(shellfolder, folderview, itemid);

        // Construct a DesktopIcon and pass to the caller.
        callback(&icon);
    }
}

shared_ptr<DesktopIcon> DesktopController::iconByName(const wstring& name)
{
    CComPtr<IEnumIDList> idlist;
    HRESULT result = folderview->Items(SVGIO_ALLVIEW, IID_PPV_ARGS(&idlist));
    if (!SUCCEEDED(result))
        throwHRESULTException("Items", result);

    // Note: If an icon with a matching name is found, itemid is not freed here.
    for (CComHeapPtr<ITEMID_CHILD> itemid; idlist->Next(1, &itemid, nullptr) == S_OK; itemid.Free())
    {
        wstring displayName = shellFolderObjNameToStrW(shellfolder, itemid);
        if (displayName == name)
            return make_shared<DesktopIcon>(shellfolder, folderview, itemid);
    }

    return shared_ptr<DesktopIcon>(nullptr);
}

vector<shared_ptr<DesktopIcon>> DesktopController::allIcons()
{
    CComPtr<IEnumIDList> idlist;
    HRESULT result = folderview->Items(SVGIO_ALLVIEW, IID_PPV_ARGS(&idlist));
    if (!SUCCEEDED(result))
        throwHRESULTException("Items", result);

    vector<shared_ptr<DesktopIcon>> icons;

    for (CComHeapPtr<ITEMID_CHILD> itemid; idlist->Next(1, &itemid, nullptr) == S_OK;)
        icons.push_back(make_shared<DesktopIcon>(shellfolder, folderview, itemid));

    return icons;
}

wstring DesktopController::shellFolderObjNameToStrW(IShellFolder* shellFolderArg, ITEMID_CHILD* itemid)
{
    STRRET str;
    HRESULT result = shellFolderArg->GetDisplayNameOf(itemid, SHGDN_NORMAL, &str);
    if (!SUCCEEDED(result))
        throwHRESULTException("GetDisplayNameOf", result);

    CComHeapPtr<wchar_t> name;
    result = StrRetToStr(&str, itemid, &name);
    if (!SUCCEEDED(result))
        throwHRESULTException("StrRetToStr", result);

    return wstring(name);
}

void DesktopController::repositionIcons(const vector<shared_ptr<DesktopIcon>>& icons, vector<Vec2<int>>& points)
{
    if (icons.size() != points.size())
        throw runtime_error("Argument size mismatch in DesktopController::repositionIcons");

    vector<PCITEMID_CHILD> itemidv;
    for (auto& icon : icons)
        itemidv.push_back(icon->getItemID());

    vector<POINT> pointsv;
    for (auto& pt : points)
        pointsv.push_back({ pt.x, pt.y });    // TODO: Is assuming the format of the POINT structure safe?

    HRESULT result = folderview->SelectAndPositionItems(
        static_cast<UINT>(icons.size()),
        itemidv.data(),
        pointsv.data(),
        SVSI_POSITIONITEM);
    if (!SUCCEEDED(result))
        throwHRESULTException("SelectAndPositionItems", result);
}

string DesktopController::errorIdToMessage(DWORD id)
{
    string message;
    if (id == 0)
        message = "";

    LPSTR buffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, id, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&buffer, 0, NULL);

    message = std::string(buffer, size);

    //Free the buffer.
    LocalFree(buffer);

    // Remove newlines. 
    message.erase(std::remove(message.begin(), message.end(), '\n'), message.end());

    return message;
}

void DesktopController::throwHRESULTException(const std::string& function, HRESULT result)
{
    throw runtime_error(function + " failed: " + errorIdToMessage(result));
}

void DesktopController::getLastErrorWithString(DWORD& dw, std::string& message)
{
    dw = GetLastError();
    message = errorIdToMessage(dw);
}

void DesktopController::throwLastError(const string& function)
{
    string message;
    DWORD error;
    getLastErrorWithString(error, message);
    throw runtime_error(function + " failed: (" + std::to_string(error) + ") " + message);
}

Vec2<int> DesktopController::iconSpacing() const
{
    POINT pt{ 0,0 };
    HRESULT result = folderview->GetSpacing(&pt);
    if (!SUCCEEDED(result))
        throwHRESULTException("GetSpacing", result);
    return Vec2<int>(pt.x, pt.y);
}

Vec2<int> DesktopController::desktopResolution() const
{
    RECT desktop;
    const HWND hwnd = GetDesktopWindow();

    if (!GetWindowRect(hwnd, &desktop))
        throwLastError("GetWindowRect");

    return Vec2<int>(desktop.right, desktop.bottom);
}

Vec2<int> DesktopController::cursorPosition() const
{
    POINT pt;
    if (!GetCursorPos(&pt))
        throwLastError("cursorPosition");
    return Vec2<int>(pt.x, pt.y);
}

#ifdef ENABLE_VIEW_MODE
ViewMode DesktopController::viewMode() const
{
    FOLDERSETTINGS settings = folderSettings();

    if (settings.ViewMode == FVM_SMALLICON)
        return ViewMode::Small;
    if (settings.ViewMode == FVM_ICON)
        return ViewMode::Medium;
    if (settings.ViewMode == FVM_TILE)
        return ViewMode::Large;

    return ViewMode::Unknown;
}
#endif

FolderFlags DesktopController::folderFlags() const
{
    FOLDERSETTINGS settings = folderSettings();

    FolderFlags flags;

    flags.autoArrange = (settings.fFlags & FWF_AUTOARRANGE ? true : false);
    flags.snapToGrid = (settings.fFlags & FWF_SNAPTOGRID ? true : false);

    return flags;
}

FOLDERSETTINGS DesktopController::folderSettings() const
{
    FOLDERSETTINGS settings;
    HRESULT result = shellview->GetCurrentInfo(&settings);

    settings.ViewMode = folderview->GetAutoArrange();

    if (!SUCCEEDED(result))
        throwHRESULTException("GetCurrentInfo", result);
    return settings;
}

void DesktopController::refresh()
{
    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH | SHCNF_FLUSHNOWAIT, desktopDirectory().c_str(), NULL);
}

// deskctrl pybind11 module definition.
#ifdef PYBIND11_BUILD
#include <pybind11/pybind11.h>

void InitUtil_pybind11(pybind11::module&);
void InitDesktopIcon_pybind11(pybind11::module&);
void DesktopController_pybind11(pybind11::module&);

PYBIND11_MODULE(deskctrl, m) 
{
    InitUtil_pybind11(m);
    InitDesktopIcon_pybind11(m);
    DesktopController_pybind11(m);
}
#endif
