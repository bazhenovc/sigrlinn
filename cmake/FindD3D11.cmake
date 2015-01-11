# Attempt to find the D3D11 libraries
# Defines:
#
#  D3D11_FOUND		  - system has D3D11
#  D3D11_INCLUDE_PATH - path to the D3D11 headers
#  D3D11_LIBRARIES	  - path to the D3D11 libraries
#  D3D11_LIB		  - d3d11.lib
#  D3DX11_LIB		  - d3dx11.lib
#  DXERR11_LIB		  - dxerr11.lib

set (D3D11_FOUND "NO")

if (WIN32)
	set (WIN8_SDK_DIR "C:/Program Files (x86)/Windows Kits/8.0")
	set (LEGACY_SDK_DIR "$ENV{DXSDK_DIR}")
	
	if (CMAKE_CL_64)
		set (ARCH x64)
	else (CMAKE_CL_64)
		set (ARCH x86)
	endif (CMAKE_CL_64)
	
	# Look for the windows 8 sdk
	find_path (D3D11_INCLUDE_PATH 
		NAMES d3d11.h
		PATHS "${WIN8_SDK_DIR}/Include/um"
		NO_DEFAULT_PATH
		DOC "Path to the windows 8 d3d11.h file"
	)
	
	if (D3D11_INCLUDE_PATH)
		find_library (D3D11_LIB 
			NAMES d3d11
			PATHS "${WIN8_SDK_DIR}/Lib/win8/um/${ARCH}"
			NO_DEFAULT_PATH
			DOC "Path to the windows 8 d3d11.lib file"
		)
		
		if (D3D11_LIB)
			set (D3D11_FOUND "YES")
			set (D3D11_LIBRARIES ${D3D11_LIB})
			mark_as_advanced (D3D11_INCLUDE_PATH D3D11_LIB)
		endif (D3D11_LIB)	
	endif (D3D11_INCLUDE_PATH)
	
	# Otherwise look for legacy installs
	if (NOT D3D11_FOUND)
		set (D3D11_INCLUDE_PATH NOTFOUND)
		
		find_path (D3D11_INCLUDE_PATH 
			NAMES d3d11.h
			PATHS "${LEGACY_SDK_DIR}/Include"
			NO_DEFAULT_PATH
			DOC "Path to the legacy d3d11.h file"
		)
		
		if (D3D11_INCLUDE_PATH)
			find_library (D3D11_LIB 
				NAMES d3d11
				PATHS "${LEGACY_SDK_DIR}/Lib/${ARCH}"
				NO_DEFAULT_PATH
				DOC "Path to the legacy d3d11.lib file"
			)

			find_library (D3DX11_LIB 
				NAMES d3dx11
				PATHS "${LEGACY_SDK_DIR}/Lib/${ARCH}"
				NO_DEFAULT_PATH
				DOC "Path to the legacy d3dx11.lib file"
			)

			find_library (DXERR11_LIB 
				NAMES dxerr dxerr11
				PATHS "${LEGACY_SDK_DIR}/Lib/${ARCH}"
				NO_DEFAULT_PATH
				DOC "Path to the legacy dxerr11x.lib file"
			)
			
			if (D3D11_LIB AND D3DX11_LIB AND DXERR11_LIB)
				set (D3D11_FOUND "YES")
				set (D3D11_LIBRARIES ${D3D11_LIB} ${D3DX11_LIB} ${DXERR11_LIB})
				mark_as_advanced (D3D11_INCLUDE_PATH D3D11_LIB D3DX11_LIB DXERR11_LIB)
			endif (D3D11_LIB AND D3DX11_LIB AND DXERR11_LIB)
		endif (D3D11_INCLUDE_PATH)
	endif (NOT D3D11_FOUND)
endif (WIN32)

if (D3D11_FOUND)
	if (NOT D3D11_FIND_QUIETLY)
		message (STATUS "D3D11 headers found at ${D3D11_INCLUDE_PATH}")
	endif (NOT D3D11_FIND_QUIETLY)
else (D3D11_FOUND)
	if (D3D11_FIND_REQUIRED)
		message (FATAL_ERROR "Could NOT find Direct3D11")
	endif (D3D11_FIND_REQUIRED)
	if (NOT D3D11_FIND_QUIETLY)
		message (STATUS "Could NOT find Direct3D11")
	endif (NOT D3D11_FIND_QUIETLY)
endif (D3D11_FOUND)
