---------------------------------------------
Using DesktopController (deskctrl) in Python
---------------------------------------------

Python bindings are provided by pybind11. The majority 
of class and function names are the same as in the C++ 
code but there are differences between the C++ and Python 
API.

 * The DcUtil namespace is not present in the Python code. 
   Anything in that namespace is accessible directly 
   in deskctrl.
   
 * C++ types DcUtil::Vec2<int> and DcUtil::Vec2<double> 
   are binded as IntVec2 and FloatVec2 types respectively 
   in Python.
   
 * std::vector is converted to List.
 
 * Objects wrapped in a container like std::unique_ptr can
   be accessed in Python as normal objects (std::unique_ptr
   is handled transparently by pybind11).

Use "help(deskctrl)" in Python to see a list of binded classes 
and functions. You can do the same for specific functions, 
e.g. "help(deskctrl.DesktopController.enumerateIcons)"

See the samples directory for examples.
