set(NLC_VERSION 1.1.4)
set(NLC_NAME NoLimitConnect)
set(NLC_ID org.nolimitconnect.nolimitconnect)
set(PROJECT_MAINTAINER bjones.engineer@gmail.com)
set(PROJECT_WEBSITE_URL nolimitconnect.org)
set(PROJECT_VENDOR nolimitconnect.org)

# Only embed a changing timestamp for Release builds.
# Keep Debug/non-Release timestamps stable to avoid unnecessary full rebuilds.
if(DEFINED CMAKE_BUILD_TYPE AND CMAKE_BUILD_TYPE STREQUAL "Release")
	string(TIMESTAMP NLC_BUILD_TIMESTAMP "%Y-%m-%dT%H:%M+0000" UTC)
else()
	set(NLC_BUILD_TIMESTAMP "debug-static")
endif()
