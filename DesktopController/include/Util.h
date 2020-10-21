#pragma once

#include <Windows.h>
#include <string>

namespace DeskCtrlUtil
{
    // A vector with 2 components.
    // Always has a size of sizeof(T) * 2, with x in the first half and y in the last.
    template <typename T>
    union Vec2
    {
        Vec2() 
        { 
            x = y = 0; 
        }

        Vec2(T _x, T _y)
            : x(_x)
            , y(_y) {}

        template<class U>
        Vec2<T>(const Vec2<U>& a)
        {
            x = static_cast<T>(a.x),
            y = static_cast<T>(a.y);
        }

        template<class U>
        Vec2<T>& operator=(const Vec2<U>& a)
        {
            x = static_cast<T>(a.x), 
            y = static_cast<T>(a.y);
            return *this;
        }

        Vec2 operator+(const Vec2& v) 
        { 
            return Vec2(x + v.x, y + v.y); 
        }

        Vec2 operator*(T value) 
        { 
            return Vec2(x * value, y * value); 
        }

        Vec2& operator+=(const Vec2& v)
        {
            x += v.x;
            y += v.y;
            return *this;
        }

        struct { T x, y; };

        T data[2];
    };

    // Return a random integer in the range min-max (inclusive).
    int randomInt(int min, int max);
 
    // Desktop icon display names may contain unicode characters which aren't
    // supported easily by the console. This converts a string of wide characters
    // to an OEM code page.
    std::string wstringToOem(const std::wstring& s);

    std::wstring desktopDirectory();
};