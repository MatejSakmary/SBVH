cmake_minimum_required (VERSION 3.21)
project(SBVH)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin")

add_executable(${PROJECT_NAME}
    "source/main.cpp"
    "source/application.cpp"
    "source/rendering_backend/renderer.cpp"
    "source/rendering_backend/camera.cpp"
    "source/raytracing_backend/scene.cpp"
)

find_package(assimp CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(daxa CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME}
    glm::glm
    imgui::imgui
    daxa::daxa
    assimp
    glfw
)
# define LOG_DEBUG whenever building in DEBUG mode
target_compile_definitions(${PROJECT_NAME} PRIVATE "$<$<CONFIG:Debug>:LOG_DEBUG>")

# This creates a marko define that can be used to find the daxa include folder for shader compilation.
set(DAXA_INCLUDE_DIR "$<TARGET_FILE_DIR:SBVH>/../../vcpkg_installed/x64-$<LOWER_CASE:$<PLATFORM_ID>>/include")
target_compile_definitions(${PROJECT_NAME} PRIVATE DAXA_SHADER_INCLUDE_DIR="${DAXA_INCLUDE_DIR}")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
