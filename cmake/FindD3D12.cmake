# Attempt to find the D3D12 libraries
# Defines:
#
#  D3D12_FOUND        - system has D3D12
#  D3D12_INCLUDE_PATH - path to the D3D12 headers
#  D3D12_LIBRARIES    - path to the D3D12 libraries
#  D3D12_LIB          - d3d12.lib

set (D3D12_FOUND "NO")

if (WIN32)
    set (WIN10_SDK_DIR "C:/Program Files (x86)/Windows Kits/10")

    set (WIN10_VERSION_DIR "10.0.10069.0")
    
    if (CMAKE_CL_64)
        set (ARCH x64)
    else (CMAKE_CL_64)
        set (ARCH x86)
    endif (CMAKE_CL_64)
    
    # Look for the windows 8 sdk
    find_path (D3D12_INCLUDE_PATH 
        NAMES d3d12.h
        PATHS "${WIN10_SDK_DIR}/Include/${WIN10_VERSION_DIR}/um"
        NO_DEFAULT_PATH
        DOC "Path to the windows 8 d3d12.h file"
    )
    
    if (D3D12_INCLUDE_PATH)
        find_library (D3D12_LIB 
            NAMES d3d12
            PATHS "${WIN10_SDK_DIR}/Lib/${WIN10_VERSION_DIR}/um/${ARCH}"
            NO_DEFAULT_PATH
            DOC "Path to the windows 8 d3d12.lib file"
        )
        
        if (D3D12_LIB)
            set (D3D12_FOUND "YES")
            set (D3D12_LIBRARIES ${D3D12_LIB})
            mark_as_advanced (D3D12_INCLUDE_PATH D3D12_LIB)
        endif (D3D12_LIB)   
    endif (D3D12_INCLUDE_PATH)

endif (WIN32)

if (D3D12_FOUND)
    if (NOT D3D12_FIND_QUIETLY)
        message (STATUS "D3D12 headers found at ${D3D12_INCLUDE_PATH}")
    endif (NOT D3D12_FIND_QUIETLY)
else (D3D12_FOUND)
    if (D3D12_FIND_REQUIRED)
        message (FATAL_ERROR "Could NOT find Direct3D12")
    endif (D3D12_FIND_REQUIRED)
    if (NOT D3D12_FIND_QUIETLY)
        message (STATUS "Could NOT find Direct3D12")
    endif (NOT D3D12_FIND_QUIETLY)
endif (D3D12_FOUND)
