#ifdef PYBIND11_BUILD
#include <pybind11/pybind11.h>

#include "DesktopController.h"

namespace py = pybind11;
using namespace DcUtil;

void InitDesktopIcon_pybind11(py::module& m)
{
    py::class_<DesktopIcon, std::unique_ptr<DesktopIcon>>(m, "DesktopIcon")
        .def("displayName", &DesktopIcon::displayName, "Display name of icon.")
        .def("position", &DesktopIcon::position, "Get the position of icon.")
        .def("reposition", &DesktopIcon::reposition, "Set the position of an icon.");
}

#endif