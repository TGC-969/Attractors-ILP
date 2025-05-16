cmake_minimum_required(VERSION 3.29)
project(AILP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(include)
include_directories(/Library/gurobi1201/macos_universal2/include)
include_directories(/Users/charan/homebrew/Cellar/symengine/0.14.0_2/include)
include_directories(/Users/charan/homebrew/Cellar/gmp/6.3.0/include)
include_directories(/Users/charan/homebrew/Cellar/flint/3.2.2/include)
include_directories(/Users/charan/homebrew/Cellar/mpfr/4.2.2/include)
include_directories(/Users/charan/homebrew/Cellar/libmpc/1.3.1/include)

set(SOURCES
    main.cpp
    src/BooleanExprEval.cpp
        src/node.cpp
        include/expressionparser.h
        src/expressionparser.cpp
        include/ILPModelBuilder.h
        src/ILPModelBuilder.cpp
        include/BooleanNetwork.h
        src/BooleanNetwork.cpp
        include/SolutionObjects.h
        src/SolutionObjects.cpp
        src/reachability.cpp
        src/IncludingSolutions.cpp
)

add_executable(ailp ${SOURCES})

target_link_libraries(ailp /Library/gurobi1201/macos_universal2/lib/libgurobi_c++.a
        /Library/gurobi1201/macos_universal2/lib/libgurobi120.dylib
        pthread
        /Users/charan/homebrew/Cellar/symengine/0.14.0_2/lib/libsymengine.0.14.0.dylib
        /Users/charan/homebrew/Cellar/gmp/6.3.0/lib/libgmp.10.dylib
        /Users/charan/homebrew/Cellar/flint/3.2.2/lib/libflint.20.0.dylib
        /Users/charan/homebrew/Cellar/mpfr/4.2.2/lib/libmpfr.6.dylib
        /Users/charan/homebrew/Cellar/libmpc/1.3.1/lib/libmpc.3.dylib
)

set_target_properties(ailp PROPERTIES
        CXX_STANDARD 17
        CXX_EXTENSIONS OFF
)