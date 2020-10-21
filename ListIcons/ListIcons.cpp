#include "DesktopController.h"

#include <fmt/core.h>

using namespace std;
using namespace DcUtil;

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

        fmt::print("Enumerating desktop icons:\n");
        dc.enumerateIcons(
            [](const DesktopIcon* icon)
            {
                static size_t iconCount = 1;
                printIconDetails(iconCount++, *icon);
            }
        );

        fmt::print("\nRetrieving pointers to desktop icons:\n");
        vector<unique_ptr<DesktopIcon>> icons = dc.allIcons();
        for (size_t i = 0; i < icons.size(); ++i)
            printIconDetails(i+1, *icons[i]);

        unique_ptr<DesktopIcon> recycleBin = dc.iconByName(L"Recycle Bin");
        if (recycleBin)
            fmt::print("\nFound Recycle Bin on desktop");
    }
    catch (const std::exception& e)
    {
        fmt::print("{}\n", e.what());
    }
}
