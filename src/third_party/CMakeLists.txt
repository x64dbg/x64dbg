set(CMAKE_UNITY_BUILD ON)

set(TITANENGINE_RESOURCES OFF CACHE BOOL "" FORCE)
add_subdirectory(TitanEngine)

set(GLEEBUG_RESOURCES OFF CACHE BOOL "" FORCE)
add_subdirectory(GleeBug)

set(DBGENG_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
add_subdirectory(DbgEng)

add_custom_target(third_party)
add_dependencies(third_party
    TitanEngine
    GleeBugTitanEngine
    GleeBugStaticEngine
    DbgEngTitanEngine
)
