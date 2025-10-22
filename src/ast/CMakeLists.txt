set(
        gazprea_ast_src
        "${CMAKE_CURRENT_SOURCE_DIR}/Ast.cpp"
)

target_sources(gazc PRIVATE ${gazprea_ast_src})

target_include_directories(gazc PUBLIC "${PROJECT_SOURCE_DIR}/include")