#include "DesktopController.h"
#include "DesktopIcon.h"

using namespace std;
using namespace DcUtil;

DesktopIcon::DesktopIcon(
    IShellFolder* shellFolderArg,
    IFolderView* folderViewArg,
    CComHeapPtr<ITEMID_CHILD> itemIdArg) 
        : shellfolder(shellFolderArg)
        , folderview(folderViewArg)
        , itemid(itemIdArg)
{
}

DesktopIcon::~DesktopIcon()
{
    if (itemid)
        itemid.Free();
}

std::wstring DesktopIcon::displayName() const
{ 
    return DesktopController::shellFolderObjNameToStrW(shellfolder, itemid);
}

Vec2<int> DesktopIcon::position() const
{
    POINT pt;
    folderview->GetItemPosition(itemid, &pt);
    return Vec2<int>(pt.x, pt.y); 
}

// Prefer DesktopController::repositionIcons (for multiple icons) over this. 
// Setting the position of icons is very expensive, but it's more efficient 
// to position multiple items at once.
void DesktopIcon::reposition(const Vec2<int>& point) const 
{
    PCITEMID_CHILD itemIdList[1] = { itemid };
    POINT pt = { point.x, point.y };
    folderview->SelectAndPositionItems(1, itemIdList, &pt, SVSI_POSITIONITEM);
}
