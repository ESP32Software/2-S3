#
FILE(GLOB_RECURSE SOURCES src/indication/*.cpp)
#
# Exposes components to both source and header files.
set(REQUIRES 
    led_strip
    main
)
#
# Anything that must be exposed to the sources files, but may remain hidden from the header files.
set(PRIV_REQUIRES
)

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "include"
                       REQUIRES ${REQUIRES}
                       PRIV_REQUIRES ${PRIV_REQUIRES}
                      )
