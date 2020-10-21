#include "DesktopController.h"

#include <fmt/core.h>

using namespace std;
using namespace DeskCtrlUtil;

void printIconDetails(size_t index, const DesktopIcon& icon)
{
    Vec2<int> pos = icon.position();
    string name = wstringToOem(icon.displayName());

    fmt::print("\t{}) {} at position {},{}\n", index, name, pos.x, pos.y);
}

int main()
{
    try
    {
        DesktopController dc;

        // Icons can be iterated over one by one.
        fmt::print("Enumerating desktop icons:\n");
        dc.enumerateIcons(
            [](const DesktopIcon* icon)
            {
                static size_t iconCount = 1;
                printIconDetails(iconCount++, *icon);
            }
        );

        // Alternatively, shared pointers to icons can be retrieved.
        fmt::print("\nRetrieving pointers to desktop icons:\n");
        vector<shared_ptr<DesktopIcon>> icons = dc.allIcons();
        for (size_t i = 0; i < icons.size(); ++i)
            printIconDetails(i+1, *icons[i]);

        // Icons can be searched for by name.
        shared_ptr<DesktopIcon> recycleBin = dc.iconByName(L"Recycle Bin");
        if (recycleBin)
            fmt::print("\nFound Recycle Bin on desktop");
    }
    catch (const std::exception& e)
    {
        fmt::print("{}\n", e.what());
    }
}
