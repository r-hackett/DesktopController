#include "Util.h"

#include <random>
#include <memory>
#include <cstdarg>
#include <shlobj.h>

using namespace std;

namespace DeskCtrlUtil
{
    int randomInt(int min, int max)
    {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> distr(min, max);
        return distr(gen);
    }

    string wstringToOem(const wstring& s)
    {
        auto buf = make_unique<char[]>(s.length() + 1);
        CharToOemW(s.c_str(), buf.get());
        return string(buf.get());
    }

    wstring desktopDirectory()
    {
        static wchar_t path[MAX_PATH + 1];
        if (SHGetSpecialFolderPathW(HWND_DESKTOP, path, CSIDL_DESKTOP, FALSE))
            return path;
        else
            return L"ERROR";
    }
}

// Utilities binding.
#ifdef PYBIND11_BUILD
#include <pybind11/pybind11.h>

void InitUtil_pybind11(pybind11::module&);

#endif