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
target_compile_definitions(${PROJECT_NAME} PRIVATE "$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:LOG_DEBUG>")
target_compile_definitions(${PROJECT_NAME} PRIVATE "$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:TRACK_TRAVERSE_STEP_COUNT>")
target_compile_definitions(${PROJECT_NAME} PRIVATE "VISUALIZE_SPATIAL_SPLITS")
target_compile_definitions(${PROJECT_NAME} PRIVATE "LOG_DEBUG")

# This creates a marko define that can be used to find the daxa include folder for shader compilation.
set(DAXA_INCLUDE_DIR "$<TARGET_FILE_DIR:SBVH>/../../vcpkg_installed/x64-$<LOWER_CASE:$<PLATFORM_ID>>/include")
target_compile_definitions(${PROJECT_NAME} PRIVATE DAXA_SHADER_INCLUDE_DIR="${DAXA_INCLUDE_DIR}")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

set(PACKAGE_APP true)

if(PACKAGE_APP)
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        list(APPEND RUNTIME_ARTIFACT_TARGETS glfw assimp::assimp)
    endif()

    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
    install(IMPORTED_RUNTIME_ARTIFACTS ${RUNTIME_ARTIFACT_TARGETS})
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/source/rendering_backend/shaders" DESTINATION bin)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/source/rendering_backend/shared" DESTINATION bin/shared)
    install(FILES "${DAXA_INCLUDE_DIR}/daxa/daxa.inl" "${DAXA_INCLUDE_DIR}/daxa/daxa.glsl" "${DAXA_INCLUDE_DIR}/daxa/daxa.hlsl" DESTINATION bin/shaders/daxa)
    install(FILES "${CMAKE_SOURCE_DIR}/imgui.ini" DESTINATION bin)

    set(CPACK_PACKAGE_NAME "SBVH_app")
    set(CPACK_PACKAGE_VENDOR "Matej-Sakmary")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SBVH")
    set(CPACK_PACKAGE_DESCRIPTION "SBVH")
    # set(CPACK_RESOURCE_FILE_WELCOME "${CMAKE_SOURCE_DIR}/packaging/infos/welcome.txt")
    # set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/packaging/infos/license.txt")
    # set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/packaging/infos/readme.txt")
    # set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/appicon.png")

    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        # configure_file("packaging/main.rc.in" "${CMAKE_BINARY_DIR}/main.rc")
        # target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_BINARY_DIR}/main.rc")

        # set(CPACK_GENERATOR WIX)
        # set(CPACK_WIX_UPGRADE_GUID 186207C7-9FC3-4F45-9FB1-6C515E0A93CC)
        set(CPACK_PACKAGE_EXECUTABLES ${PROJECT_NAME} "SBVH app")

        # Set the default installation directory. In this case it becomes C:/Program Files/GabeVoxelGame
        set(CPACK_PACKAGE_INSTALL_DIRECTORY "SBVH app")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # TODO: Find a better way to package, though tar.gz works for now
        # install(FILES "${CMAKE_SOURCE_DIR}/packaging/gabe_voxel_game.desktop" DESTINATION share/applications)
        # set(CPACK_BINARY_AppImage ON)
    endif()

    include(InstallRequiredSystemLibraries)
    include(CPack)
endif()
