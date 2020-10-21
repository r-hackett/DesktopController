#ifdef PYBIND11_BUILD
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/functional.h>

#include "DesktopController.h"

namespace py = pybind11;

void DesktopController_pybind11(py::module& m)
{
#ifdef ENABLE_VIEW_MODE
    py::enum_<ViewMode>(m, "ViewMode")  // Strongly typed enum.
        .value("Small", ViewMode::Small)
        .value("Medium", ViewMode::Medium)
        .value("Large", ViewMode::Large);
#endif

    py::class_<FolderFlags>(m, "FolderFlags")
        .def(py::init<>())
        .def_readonly("autoArrange", &FolderFlags::autoArrange)
        .def_readonly("snapToGrid", &FolderFlags::snapToGrid);

    py::class_<DesktopController>(m, "DesktopController")
        .def(py::init<>())
#ifdef ENABLE_VIEW_MODE
        .def("viewMode", &DesktopController::viewMode, "Get the current view mode.")
#endif
        .def("enumerateIcons", &DesktopController::enumerateIcons, "Iterate over all desktop icons.")
        .def("iconByName", &DesktopController::iconByName, "Get a desktop icon which matches the given name exactly.")
        .def("allIcons", &DesktopController::allIcons, "Get all desktop icons present on the desktop.")
        .def("folderFlags", &DesktopController::folderFlags, "Get the current desktop folder flags.")
        .def("cursorPosition", &DesktopController::cursorPosition, "Get the current position of the cursor.")
        .def("refresh", &DesktopController::refresh, "Notify the system that the contents of the desktop folder has changed.");
}

#endif