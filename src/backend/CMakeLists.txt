set(
        gazprea_backend_src
        "${CMAKE_CURRENT_SOURCE_DIR}/Backend.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Root.cpp"
)

target_sources(gazc PRIVATE ${gazprea_backend_src})
add_subdirectory(statements)
add_subdirectory(types)
add_subdirectory(prototypes)
add_subdirectory(expressions)
add_subdirectory(utils)

target_include_directories(gazc PUBLIC "${PROJECT_SOURCE_DIR}/include")