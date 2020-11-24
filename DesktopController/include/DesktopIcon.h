#pragma once

#include "Util.h"

/** @brief A class which represents an icon on the desktop.
 *
 *  This encapsulates operations which can be performed on a desktop icon.
 */
class DesktopIcon
{
public:
    /** The constructor for DesktopIcon which takes pointers to Shell interfaces.
     *  This is called internally by DesktopController. You should not (need to)
     *  construct a DesktopIcon externally. 
     */
    DesktopIcon(
        IShellFolder* shellfolder,
        IFolderView* folderview,
        CComHeapPtr<ITEMID_CHILD> pidl);

    /** Destructor.
     */
    ~DesktopIcon();

    /** Get the display name of this icon as a UTF-16 encoded Unicode string.
     * 
     *  @return The display name as a wide string. 
     */
    std::wstring displayName() const;

    /** Get the upper left cordinates of this icon.
     *
     *  @return DcUtil::Vec2 containing x and y cordinates.
     */
    DcUtil::Vec2<int> position() const;

    /** Set the upper left cordinates of this icon.
     *
     *  @param pt New x and y cordinates of this icon. 
     *  @see DesktopController::repositionIcons()
     *  @note This is an expensive function. When setting the position
     *        of multiple icons, it's more efficient to call DesktopController::repositionIcons().
     */
    void reposition(const DcUtil::Vec2<int>& pt) const;

    /** Used internally: Get the item ID used to identify this item in shell interfaces.
     */
    ITEMID_CHILD* getItemID() const { return itemid; }

    /** Copy constructor is disabled.
     */
    DesktopIcon(const DesktopIcon&) = delete;

    /** Copy assignment operator is disabled.
     */
    void operator=(const DesktopIcon&) = delete;

    /** Default constructor is disabled.
     */
    DesktopIcon() = delete;

private:
    IShellFolder* shellfolder;
    IFolderView* folderview;
    CComHeapPtr<ITEMID_CHILD> itemid;
};