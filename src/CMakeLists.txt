add_executable(SSAO-demo
        gl_env.h
        main.cpp
        shader.h
        mesh.h
        model.h)

target_link_libraries(SSAO-demo PRIVATE assimp::assimp glew_s glm stb glfw imgui)
target_include_directories(SSAO-demo PRIVATE
        ../third_party/glew/include
        ${CMAKE_CURRENT_BINARY_DIR})

target_compile_features(SSAO-demo PRIVATE cxx_std_11)

configure_file(config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)