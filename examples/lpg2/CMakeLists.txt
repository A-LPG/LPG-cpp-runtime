
add_lpg_example(LpgExample)

target_sources(LpgExample PRIVATE
        LpgExample.cpp
        LPGParser.cpp
        LPGParser_top_level_ast.cpp
        )