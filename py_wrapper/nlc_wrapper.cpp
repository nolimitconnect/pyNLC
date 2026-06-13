#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "IFromGui.h"

namespace py = pybind11;

// 1. Trampoline class needed because IFromGui has pure virtual functions
class PyIFromGui : public IFromGui {
public:
    using IFromGui::IFromGui; // Inherit constructors

    void fromGuiAppStartup(std::string assetsDir, std::string rootDataDir, bool fromThread) override {
        PYBIND11_OVERRIDE_PURE(void, IFromGui, fromGuiAppStartup, assetsDir, rootDataDir, fromThread);
    }
    void fromGuiSetUserSpecificDir(std::string userSpecificDir, bool fromThread) override {
        PYBIND11_OVERRIDE_PURE(void, IFromGui, fromGuiSetUserSpecificDir, userSpecificDir, fromThread);
    }
    void fromGuiSetUserXferDir(std::string userDownloadDir, bool fromThread) override {
        PYBIND11_OVERRIDE_PURE(void, IFromGui, fromGuiSetUserXferDir, userDownloadDir, fromThread);
    }
    void fromGuiAppShutdown(void) override {
        PYBIND11_OVERRIDE_PURE(void, IFromGui, fromGuiAppShutdown);
    }
    bool fromGuiDeleteUser(VxGUID& onlineId) override {
        PYBIND11_OVERRIDE_PURE(bool, IFromGui, fromGuiDeleteUser, onlineId);
    }
    uint64_t fromGuiGetDiskFreeSpace(const char* dir) override {
        PYBIND11_OVERRIDE_PURE(uint64_t, IFromGui, fromGuiGetDiskFreeSpace, dir);
    }
    // Add remaining trampoline overrides for other virtual methods here...
};

// 2. Export the module to Python
PYBIND11_MODULE(nlc_engine, m) {
    m.doc() = "Python bindings for NoLimitConnect core logic libraries";

    // Register placeholder definitions for custom types used in parameters
    py::class_<VxGUID>(m, "VxGUID")
        .def(py::init<>());
        
    py::class_<VxNetIdent>(m, "VxNetIdent");

    // Register the abstract interface and its helper trampoline
    py::class_<IFromGui, PyIFromGui>(m, "IFromGui")
        .def("app_startup", &IFromGui::fromGuiAppStartup, 
             py::arg("assets_dir"), py::arg("root_data_dir"), py::arg("from_thread") = false,
             py::call_guard<py::gil_scoped_release>()) // Releases GIL for thread safety
        
        .def("set_user_dir", &IFromGui::fromGuiSetUserSpecificDir, 
             py::arg("user_dir"), py::arg("from_thread") = false)
        
        .def("set_xfer_dir", &IFromGui::fromGuiSetUserXferDir, 
             py::arg("user_download_dir"), py::arg("from_thread") = false)
        
        .def("shutdown", &IFromGui::fromGuiAppShutdown)
        
        .def("delete_user", &IFromGui::fromGuiDeleteUser, py::arg("online_id"))
        
        .def("get_free_space", &IFromGui::fromGuiGetDiskFreeSpace, py::arg("dir") = nullptr);
}
