#include "service_utils.hpp"

SC_HANDLE service_utils::open_sc_manager()
{
    return OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
}

SC_HANDLE service_utils::create_service(const std::string_view driver_path)
{
    SC_HANDLE sc_manager_handle = service_utils::open_sc_manager();

    CHECK_SC_MANAGER_HANDLE(sc_manager_handle, (SC_HANDLE)INVALID_HANDLE_VALUE);

    SC_HANDLE mhyprot_service_handle = CreateService(
        sc_manager_handle,
        SERVICE_NAME,
        DISPLAY_NAME,
        SERVICE_START | SERVICE_STOP | DELETE,
        SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE,
        XorStr(L"C:\\Windows\\System32\\drivers\\vmbusraid.sys").c_str(), nullptr, nullptr, nullptr, nullptr, nullptr
    );

    if (!CHECK_HANDLE(mhyprot_service_handle))
    {
        const auto last_error = GetLastError();

        if (last_error == ERROR_SERVICE_EXISTS)
        {
            return OpenService(
                sc_manager_handle,
                SERVICE_NAME,
                SERVICE_START | SERVICE_STOP | DELETE
            );
        }

        CloseServiceHandle(sc_manager_handle);
        return (SC_HANDLE)(INVALID_HANDLE_VALUE);
    }

    CloseServiceHandle(sc_manager_handle);

    return mhyprot_service_handle;
}

bool service_utils::delete_service(SC_HANDLE service_handle, bool close_on_fail, bool close_on_success)
{
    SC_HANDLE sc_manager_handle = open_sc_manager();

    CHECK_SC_MANAGER_HANDLE(sc_manager_handle, false);

    if (!DeleteService(service_handle))
    {
        const auto last_error = GetLastError();

        if (last_error == ERROR_SERVICE_MARKED_FOR_DELETE)
        {
            CloseServiceHandle(sc_manager_handle);
            return true;
        }

        CloseServiceHandle(sc_manager_handle);
        if (close_on_fail) CloseServiceHandle(service_handle);
        return false;
    }

    CloseServiceHandle(sc_manager_handle);
    if (close_on_success) CloseServiceHandle(service_handle);

    return true;
}

bool service_utils::start_service(SC_HANDLE service_handle)
{
    return StartService(service_handle, 0, nullptr);
}

bool service_utils::stop_service(SC_HANDLE service_handle)
{
    SC_HANDLE sc_manager_handle = open_sc_manager();

    CHECK_SC_MANAGER_HANDLE(sc_manager_handle, false);

    SERVICE_STATUS service_status;

    if (!ControlService(service_handle, SERVICE_CONTROL_STOP, &service_status))
    {
        CloseServiceHandle(sc_manager_handle);
        return false;
    }

    CloseServiceHandle(sc_manager_handle);

    return true;
}
