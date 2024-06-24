include(FetchContent)

########################################################################################################################
# Boost
########################################################################################################################
set(BOOST_ROOT "C:/Users/Clement/Documents/Libs/boost_1_85_0")
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(
    Boost ${BOOST_MINIMUM_VERSION}
    REQUIRED COMPONENTS
        serialization
)

if(Boost_FOUND)
    include_directories("${Boost_INCLUDE_DIRS}/")
    link_directories(${Boost_LIBRARY_DIRS})
else()
    message(FATAL_ERROR "Boost package not found." )
endif()


########################################################################################################################
# Qt
########################################################################################################################

# Try to find Qt6
find_package(
    Qt6
    COMPONENTS
        Core
        Gui
        Widgets
)

# If Qt6 was not found, fallback to Qt5
# Require minimum Qt 5.15 for versionless cmake targets. This can be relaxed down to Qt 5.6 (?) if needed by modifying
# The CMake target linking statements as demonstrated in the Qt documentation.
if (NOT Qt6_FOUND)
    find_package(
        Qt5 5.15
        REQUIRED
        COMPONENTS
            Core
            Gui
            Widgets
    )
endif()
