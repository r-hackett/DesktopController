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
#include "DesktopIcon.h"

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

/** @brief Flags returned by DesktopController::folderFlags().
 */
struct FolderFlags
{
    bool autoArrange;  /**< True if the desktop has autoarrange enabled. */
    bool snapToGrid;   /**< True if the desktop has align/snap to grid enabled. */
};


/** @brief A class used to access desktop icons.
 *
 *  A C++11 based implementation for various tasks associated with the windows desktop.
 *  Access to desktop icons is achieved via Windows Shell interfaces. 
 * 
 *  Multiple monitors are not supported officially (needs testing).
 * 
 *  Errors are handled by exceptions (std::runtime_exception).
 */
class DesktopController
{
public:
    /** Default constructor. 
     *  Note: If not already set, DPI awareness is set to PROCESS_SYSTEM_DPI_AWARE on construction.
     */
    DesktopController();

    /** Destructor.
     */
    ~DesktopController();

    /** Enumerate all desktop icons. For each icon on the desktop, a DesktopIcon pointer is passed to the caller.
     * 
     *  @param callback A caller provided callable target which takes a pointer to a DesktopIcon as an argument.
     *                  The DesktopIcon object pointed to is destroyed when the callback returns.
     */
    void enumerateIcons(const std::function<void(const DesktopIcon*)>& callback);

    /** Get a unique pointer to a DesktopIcon which matches the given name exactly.
     *
     *  @param name A UTF-16 encoded Unicode string matching a desktop icon's display name.
     *  @return If the icon is found, a DesktopIcon is constructed and returned in a unique_ptr.
     *          If no icon is found, a null unique_ptr is returned.
     */
    std::unique_ptr<DesktopIcon> iconByName(const std::wstring& name);

    /** Get a vector of unique pointers to all desktop icons present on the desktop.
     *
     *  @return A vector where each element is a unique_ptr containing a DesktopIcon. Elements
     *          of the vector are guaranteed to not be null or empty.
     */
    std::vector<std::unique_ptr<DesktopIcon>> allIcons();

    /** Reposition one or more icons. Both parameters must have the same number of elements.
     *
     *  @param icons A vector of DesktopIcon pointers.
     *  @param points DcUtil::Vec2 which contains the new coordinates for each respective 
     *                DesktopIcon in the passed icons vector.
     */
    void repositionIcons(const std::vector<DesktopIcon*>& icons, std::vector<DcUtil::Vec2<int>>& points);

    /** Used internally: Returns the display name of a specified file object as a UTF-16 Unicode string.
     */
    static std::wstring shellFolderObjNameToStrW(IShellFolder* shellfolder, ITEMID_CHILD* pidl);

    /** Get the resolution of the desktop in pixels.
     *
     *  @return DcUtil::Vec2 with x and y set to the horizontal and vertical resolution of the desktop respectively.
     */
    DcUtil::Vec2<int> desktopResolution() const;

    /** Get the dimensions of desktop icons in pixels, including surrounding whitespace.
     *
     *  @return DcUtil::Vec2 containing the dimensions of desktop icons in pixels.
     */
    DcUtil::Vec2<int> iconSpacing() const;

    /** Get position of the cursor.
     *
     *  @return DcUtil::Vec2 containing the pixel coordinates of the cursor.
     */
    DcUtil::Vec2<int> cursorPosition() const;

#ifdef ENABLE_VIEW_MODE
    ViewMode viewMode() const;
#endif

    /** Get folder flags for the desktop.
     *
     *  @return FolderFlags instance.
     */
    FolderFlags folderFlags() const;

    /** Notify the system that the contents of the desktop folder has changed.
     */
    void refresh();

    /** Copy constructor is disabled.
     */
    DesktopController(const DesktopController&) = delete;

    /** Copy assignment operator is disabled.
     */
    void operator=(const DesktopController&) = delete;

private:
    // This is done programmatically here (not in the manifest) because manifests are typically
    // only applied to .exe. There may well be ways to embded a manifest in the .lib or .dll but
    // for now this works.
    void SetHighDpiAwareness();

    bool Win81OrNewer();

    std::pair<DWORD, DWORD> getWindowsVersion();

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
