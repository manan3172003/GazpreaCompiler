set(
        gazprea_statements_src
        "${CMAKE_CURRENT_SOURCE_DIR}/DeclarationAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/BlockAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/AssignmentAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ReturnAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/OutputAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/InputAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/LoopAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/TypealiasAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/BreakAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ContinueAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ConditionalAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/IdentifierLeftAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/TupleAssignAst.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ProcedureCallAst.cpp"
)

target_sources(gazc PRIVATE ${gazprea_statements_src})

target_include_directories(gazc PUBLIC "${PROJECT_SOURCE_DIR}/include")