if(CMAKE_SYSTEM_NAME MATCHES RetroBeBox)
    # LaunchAPPL launches classic Mac OS applications; nothing here applies to BeOS.
    return()
endif()

add_subdirectory(Common)

if(CMAKE_SYSTEM_NAME MATCHES Retro.*)
    add_subdirectory(Server)
else()
    add_subdirectory(Client)
endif()
