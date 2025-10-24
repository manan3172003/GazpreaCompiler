set(
        gazprea_expressions_src
        "${CMAKE_CURRENT_SOURCE_DIR}/IntegerAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/RealAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/UnaryAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/IdentifierAst.cpp"
)

target_sources(gazc PRIVATE ${gazprea_expressions_src})

target_include_directories(gazc PUBLIC "${PROJECT_SOURCE_DIR}/include")