set(
        gazprea_statements_src
        "${CMAKE_CURRENT_SOURCE_DIR}/DeclarationAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/BlockAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/AssignmentAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ReturnAst.cpp"
)

target_sources(gazc PRIVATE ${gazprea_statements_src})

target_include_directories(gazc PUBLIC "${PROJECT_SOURCE_DIR}/include")