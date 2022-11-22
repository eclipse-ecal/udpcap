include(FetchContent)
FetchContent_Declare(asio
	GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
	GIT_TAG asio-1-24-0
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	)
FetchContent_GetProperties(asio)
if(NOT asio_POPULATED)
	message(STATUS "Fetching asio...")
	FetchContent_Populate(asio)
endif()
set(asio_ROOT_DIR "${asio_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/Modules/)