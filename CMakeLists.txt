cmake_minimum_required (VERSION 3.21)
project(SBVH)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "bin")

add_executable(${PROJECT_NAME}
    "source/main.cpp"
    "source/application.cpp"
    "source/rendering_backend/renderer.cpp"
    "source/rendering_backend/camera.cpp"
    "source/raytracing_backend/raytracer.cpp"
    "source/raytracing_backend/scene.cpp"
    "source/raytracing_backend/bvh.cpp"
    "source/raytracing_backend/aabb.cpp"
    "source/external/stb_image_impl.cpp"
)

find_package(assimp CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(daxa CONFIG REQUIRED)
find_path(STB_INCLUDE_DIRS "stb_c_lexer.h")

target_include_directories(${PROJECT_NAME} PRIVATE ${STB_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE
    glm::glm
    imgui::imgui
    daxa::daxa
    assimp::assimp
    glfw
)
# Debug mode defines
target_compile_definitions(${PROJECT_NAME} PRIVATE "$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebugInfo>>:LOG_DEBUG>")
target_compile_definitions(${PROJECT_NAME} PRIVATE "$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebugInfo>>:VISUALIZE_SPATIAL_SPLITS>")

# This creates a marko define that can be used to find the daxa include folder for shader compilation.
set(DAXA_INCLUDE_DIR "$<TARGET_FILE_DIR:SBVH>/../../vcpkg_installed/x64-$<LOWER_CASE:$<PLATFORM_ID>>/include")
target_compile_definitions(${PROJECT_NAME} PRIVATE DAXA_SHADER_INCLUDE_DIR="${DAXA_INCLUDE_DIR}")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
