set(
        gazprea_expressions_src
        "${CMAKE_CURRENT_SOURCE_DIR}/IntegerAst.cpp"
)

target_sources(gazc PRIVATE ${gazprea_expressions_src})

target_include_directories(gazc PUBLIC "${PROJECT_SOURCE_DIR}/include")