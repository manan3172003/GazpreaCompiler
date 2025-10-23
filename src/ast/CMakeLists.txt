set(
        gazprea_ast_src
        "${CMAKE_CURRENT_SOURCE_DIR}/Ast.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RootAst.cpp"
)

add_subdirectory(expressions)
add_subdirectory(statements)
add_subdirectory(walkers)
target_sources(gazc PRIVATE ${gazprea_ast_src})

target_include_directories(gazc PUBLIC "${PROJECT_SOURCE_DIR}/include")