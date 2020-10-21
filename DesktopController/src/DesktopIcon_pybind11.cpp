#ifdef PYBIND11_BUILD
#include <pybind11/pybind11.h>

#include "DesktopController.h"

namespace py = pybind11;
using namespace DeskCtrlUtil;

void InitDesktopIcon_pybind11(py::module& m)
{
    py::class_<DesktopIcon, std::shared_ptr<DesktopIcon>>(m, "DesktopIcon")
        .def("displayName", &DesktopIcon::displayName, "Display name of icon.")
        .def("position", &DesktopIcon::position, "Position of icon.");
}

#endif