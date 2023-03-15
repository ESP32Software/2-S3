#
# Include libraries up here
#
#
# Use the Global Recursive action to gather up all the source files under a variable called APP_SOURCES
FILE(GLOB_RECURSE SOURCES ${PROJECT_DIR}/main/*.cpp)
#
#
# Exposes components to both source and header files.
set(MAIN_REQUIRES
    indication
    nvs_flash
)
#
# Anything that must be exposed to the sources files, but may remain hidden from the header files.
set(MAIN_PRIV_REQUIRES
)

#
# Add all directories here for libraries
set(COMPONENT_ADD_INCLUDEDIRS
    ${CMAKE_CURRENT_LIST_DIR}/include
)
#
#
idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS ${COMPONENT_ADD_INCLUDEDIRS}
                       REQUIRES ${MAIN_REQUIRES}
                       PRIV_REQUIRES ${MAIN_PRIV_REQUIRES}
)
