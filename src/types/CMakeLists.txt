set(
        gazprea_types_src
        "${CMAKE_CURRENT_SOURCE_DIR}/Type.cpp"
)

target_sources(gazc PRIVATE ${gazprea_ast_src})

target_include_directories(gazc PUBLIC "${PROJECT_SOURCE_DIR}/include")