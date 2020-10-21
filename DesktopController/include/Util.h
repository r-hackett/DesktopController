#pragma once

#include <Windows.h>
#include <string>

/** @brief A namespace for any utility-like functionality.
 */
namespace DcUtil
{
    /** @brief A vector with 2 components. 
     *
     *  Implemented as a union. 
     *  Always has a size of sizeof(T) * 2, with x in the first half and y in the last.
     *  @tparam T Data type of member variables.
     */
    template <typename T>
    union Vec2
    {
        /** Default constructor. Initialises all components to 0.
         */
        Vec2() 
        { 
            x = y = 0; 
        }

        /** Constructor.
         *
         *  @param _x Value assigned to the first component.
         *  @param _y Value assigned to the second component.
         */
        Vec2(T _x, T _y)
            : x(_x)
            , y(_y) {}

        /** Copy constructor which copies from a Vec2 with a different template type argument.
         *  Casts the components from type U to type T and assigns them to this->x and this->y. 
         *
         *  @tparam U Template type of the incoming Vec2.
         *  @param A Vec2 to cast and copy in to this.
         */
        template<class U>
        Vec2<T>(const Vec2<U>& a)
        {
            x = static_cast<T>(a.x),
            y = static_cast<T>(a.y);
        }

        /** Copy Assignment operator which copies from a Vec2 with a different template type argument.
         *  Casts the components from type U to type T and assigns them to this->x and this->y.
         *
         *  @tparam U Template type of the incoming Vec2.
         *  @param a Vec2 to cast and copy in to this.
         *  @return A reference to *this.
         */
        template<class U>
        Vec2<T>& operator=(const Vec2<U>& a)
        {
            x = static_cast<T>(a.x), 
            y = static_cast<T>(a.y);
            return *this;
        }

        /** Addition operator overload. 
         *  Adds the components of v to the components of this.
         *
         *  @param v Incoming Vec2 to add to this.
         *  @return The Vec2 result of the addition.
         */
        Vec2<T> operator+(const Vec2<T>& v)
        { 
            return Vec2<T>(x + v.x, y + v.y);
        }

        /** Subtraction operator overload.
         *  Subtracts the components of v from the components of this.
         *
         *  @param v Incoming Vec2 to subtract from this.
         *  @return The Vec2 result of the subtraction.
         */
        Vec2<T> operator-(const Vec2<T>& v)
        {
            return Vec2<T>(x - v.x, y - v.y);
        }

        /** Multiplication operator overload.
         *  Multiplies a scalar value with the components of this.
         *
         *  @param v Incoming value to multiply with this.
         *  @return The Vec2 result of the multiplication.
         */
        Vec2<T> operator*(T value)
        { 
            return Vec2<T>(x * value, y * value);
        }

        /** Addition assignment operator overload.
         *  Adds the components of v to the components of this.
         *
         *  @param v Incoming Vec2 to add to this.
         *  @return A reference to *this.
         */
        Vec2<T>& operator+=(const Vec2<T>& v)
        {
            x += v.x;
            y += v.y;
            return *this;
        }

        /** Subtraction assignment operator overload.
         *  Subtracts the components of v from the components of this.
         *
         *  @param v Incoming Vec2 to subtract from this.
         *  @return A reference to *this.
         */
        Vec2<T>& operator-=(const Vec2<T>& v)
        {
            x -= v.x;
            y -= v.y;
            return *this;
        }

        /** Multiplication assignment operator overload.
         *  Multiplies the components of v with the components of this.
         *
         *  @param v Incoming Vec2 to multiply with this.
         *  @return A reference to *this.
         */
        Vec2<T>& operator*=(const Vec2<T>& v)
        {
            x *= v.x;
            y *= v.y;
            return *this;
        }

        /** Subscript operator overload.
         *
         *  @param i Index of component to return.
         *  @return If the index is valid, returns the value at that index, else 0.
         */
        T operator[](const unsigned int i)
        {
            return (i < 2 ? data[i] : 0);
        }

        struct 
        { 
            T x;    /**< First component. */ 
            T y;    /**< Second component. */ 
        };

        T data[2];   /**< Array containing the first and second components. */ 
    };

#if 0
    /** Return a random integer in the range min-max (inclusive).
     *
     *  @param min Minimum possible value to return.
     *  @param max Maximum possible value to return.
     */
    int randomInt(int min, int max);
#endif

    /** Converts a string of wide characters to a single-byte OEM code page string. 
     * 
     * Desktop icon display names are encoded as UTF-16 Unicode. These characters aren't
     * supported easily by the Win32 console. This function approximates the Unicode characters
     * for display in the console.
     *
     *  @param s UTF-16 Unicode string.
     *  @return OEM approximation of the UTF-16 Unicode input.
     */
    std::string wstringToOem(const std::wstring& s);

    /** Find the location of the user's desktop directory.
     *
     *  @return Full path of the desktop directory as a Unicode string.
     */
    std::wstring desktopDirectory();
};