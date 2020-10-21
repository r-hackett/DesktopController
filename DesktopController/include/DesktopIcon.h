#pragma once

#include "Util.h"

class DesktopIcon
{
public:
    DesktopIcon(
        IShellFolder* shellfolder,
        IFolderView* folderview,
        CComHeapPtr<ITEMID_CHILD> pidl);

    ~DesktopIcon();

    // Disable copying.
    DesktopIcon(const DesktopIcon&) = delete;
    void operator=(const DesktopIcon&) = delete;

    // Get icon attributes.
    std::wstring displayName() const;
    DeskCtrlUtil::Vec2<int> position() const;

    // Modify icon position.
    void reposition(const DeskCtrlUtil::Vec2<int>& pt) const;

    ITEMID_CHILD* getItemID() const { return itemid; }

private:
    IShellFolder* shellfolder;
    IFolderView* folderview;
    CComHeapPtr<ITEMID_CHILD> itemid;
};