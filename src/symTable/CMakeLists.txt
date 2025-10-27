set(
        gazprea_symtable_src
        "${CMAKE_CURRENT_SOURCE_DIR}/Scope.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Symbol.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/SymTable.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Type.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/VariableSymbol.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/BuiltInTypeSymbol.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ScopedSymbol.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/TupleTypeSymbol.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/TypealiasSymbol.cpp"
)

target_sources(gazc PRIVATE ${gazprea_symtable_src})

target_include_directories(gazc PUBLIC "${PROJECT_SOURCE_DIR}/include")