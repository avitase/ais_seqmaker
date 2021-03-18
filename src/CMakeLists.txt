add_executable(
        seqmaker
        seqmaker.cxx
        ais.cpp
        mmsi_counter.cpp
        seq.cpp
        sequencer.cpp
        seq_counter.cpp
        seq_maker.cpp)
target_include_directories(seqmaker BEFORE PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(
        seqmaker
        PRIVATE project_options
        project_warnings)

add_executable(
        seqdiff
        seqdiff.cxx
        ais.cpp
        seq.cpp
        sequencer.cpp
        seq_diff.cpp)
target_include_directories(seqdiff BEFORE PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(
        seqdiff
        PRIVATE project_options
        project_warnings)
