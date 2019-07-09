cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(
  eh-sim
  VERSION 0.0.1
  LANGUAGES CXX
)

add_executable(
  ${PROJECT_NAME}
  src/scheme/backup_every_cycle.hpp
  src/scheme/clank.hpp
  src/scheme/data_sheet.hpp
  src/scheme/eh_model.hpp
  src/scheme/eh_scheme.hpp
  src/scheme/magical_scheme.hpp
  src/scheme/on_demand_all_backup.hpp
  src/scheme/parametric.hpp
  src/capacitor.hpp
  src/main.cpp
  src/simulate.cpp
  src/simulate.hpp
  src/stats.hpp
  src/voltage_trace.cpp
  src/voltage_trace.hpp
        src/input_source.cpp src/input_source.hpp src/capacitor.cpp src/scheme/task_based.hpp src/simul_timer.cpp src/simul_timer.h src/task_history.h src/task_history.cpp)

target_include_directories(
  ${PROJECT_NAME}
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE argagg
  PRIVATE thumbulator
)

set_target_properties(
  ${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 14
  CXX_STANDARD_REQUIRED ON
)
