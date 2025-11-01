set(
        gazprea_utils_src
        "${CMAKE_CURRENT_SOURCE_DIR}/ValidationUtils.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/BackendUtils.cpp"
)

target_sources(gazc PRIVATE ${gazprea_utils_src})

target_include_directories(gazc PUBLIC "${PROJECT_SOURCE_DIR}/include")