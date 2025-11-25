set(
        gazprea_utils_src
        "${CMAKE_CURRENT_SOURCE_DIR}/ArrayUtils.cpp"
)

target_sources(gazc PRIVATE ${gazprea_utils_src})

target_include_directories(gazc PUBLIC "${PROJECT_SOURCE_DIR}/include")
