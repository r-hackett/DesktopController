#pragma once

// Avoid std::min/std::max collision with min/max macros defined by Windows.h inclusion.
#define NOMINMAX

#include <Windows.h>
#include <shlobj.h>
#include <exdisp.h>
#include <shlwapi.h>
#include <atlbase.h>
#include <atlalloc.h>
#include <stdio.h>
#include <objbase.h>

#include <string>
#include <functional>
#include <stdexcept>
#include <memory>
#include <vector>

#include "Util.h"
#include "DesktopIcon.h"    // Do not include this from any other header.

// ViewMode always seems to be the same value (1 = FVM_ICON) regardless of the desktop settings.
// This disables support for it, for now.
#if 0
#define ENABLE_VIEW_MODE
#endif

#ifdef ENABLE_VIEW_MODE
enum class ViewMode
{
    Small,
    Medium,
    Large,
    Unknown
};
#endif

// TODO: Not sure which others are relevant to the desktop folder. 
struct FolderFlags
{
    bool autoArrange;
    bool snapToGrid;
};

class DesktopController
{
public:
    DesktopController();
    ~DesktopController();

    // Disable copying.
    DesktopController(const DesktopController&) = delete;
    void operator=(const DesktopController&) = delete;

    // Iterate over all desktop icons. The user provided callback is called with 
    // a pointer to a DesktopIcon which is destroyed when the callback returns.
    void enumerateIcons(const std::function<void(const DesktopIcon*)>& callback);

    // Retrieve a shared pointer to a DesktopIcon which matches the given name exactly.
    std::shared_ptr<DesktopIcon> iconByName(const std::wstring& name);

    // Returns a vector of shared pointers to all desktop icons present on the desktop.
    std::vector<std::shared_ptr<DesktopIcon>> allIcons();

    // Reposition icons.
    void repositionIcons(
        const std::vector<std::shared_ptr<DesktopIcon>>& icons, 
        std::vector<DeskCtrlUtil::Vec2<int>>& points);

    // Returns the display name of a specified file object as a wide string.
    static std::wstring shellFolderObjNameToStrW(IShellFolder* shellfolder, ITEMID_CHILD* pidl);

    DeskCtrlUtil::Vec2<int> desktopResolution() const;
    DeskCtrlUtil::Vec2<int> iconSpacing() const;
    DeskCtrlUtil::Vec2<int> cursorPosition() const;

#ifdef ENABLE_VIEW_MODE
    ViewMode viewMode() const;
#endif

    FolderFlags folderFlags() const;

    // Notify the system that the contents of the desktop folder has changed.
    void refresh();

private:
    static std::string errorIdToMessage(DWORD id);

    FOLDERSETTINGS folderSettings() const;

    void findDesktopShellView(CComPtr<IShellView>& pShellViewOut);
    void findDesktopFolderView(CComPtr<IShellView> pShellView, REFIID riid, void** ppv);

    static void throwHRESULTException(const std::string& function, HRESULT result);
    static void throwLastError(const std::string& function);

    static void getLastErrorWithString(DWORD& dw, std::string& message);

    CComPtr<IShellView> shellview;
    CComPtr<IFolderView> folderview;
    CComPtr<IShellFolder> shellfolder;
};
