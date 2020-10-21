**DesktopController** is a library for programmatically manipulating desktop icons on Windows.

## Build
Debug and Release configurations will output a static library which is referenced by the samples (see below). pybind11_debug and pybind11_release configurations output a shared library (as a .pyd file) which includes Python bindings. This can be imported as a module from Python code.

NuGet is used to retrieve the appropriate version of Python for the Python bind configurations. There's no need to download third party libraries manually.

If you want to include DesktopController in your own project, either link the compiled static library and copy the relevant header files, or alternatively copy both the headers and the source files to your project. Additionally, in your Visual Studio project under **Manifest Tool -> Input and Output -> DPI Awareness**, select **High DPI Aware**.

## Samples
**Visual Studio projects.** Build as Debug or Release.

* DesktopSnake: Play a game of snake with your desktop icons.
* ListIcons: List basic information about icons on the desktop in various ways.

**Python.**

* list_icons.py
* folder_settings.py
  
## TODO

* Documentation for both C++ library and Python bindings.
* Testing on different systems.
* Testing on different versions of Windows.
* More samples.
* Inclusion in PyPi.