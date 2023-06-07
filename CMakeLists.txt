#Specify the version being used aswell as the language
cmake_minimum_required(VERSION 3.19)

#Name your project here
project(muvitome)


#set the module directory
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}")
set(CMAKE_CXX_STANDARD 20)

find_package(GLEW REQUIRED)
find_package(TIRA REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED core glfw-binding opengl3-binding)
find_package(glfw3 CONFIG REQUIRED)


#build the executable in the binary directory on MS Visual Studio
if ( MSVC )
	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")
	SET( LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
	SET( LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_SCL_SECURE_NO_WARNINGS)
endif ( MSVC )

#set the include directories
include_directories(
			${CMAKE_CURRENT_BINARY_DIR}
			${CMAKE_CURRENT_SOURCE_DIR}
			${GLFW_INCLUDE_DIRS}
			${TIRA_INCLUDE_DIRS}
			thorlabs
)

configure_file(fonts/Roboto-Medium.ttf 
				Roboto-Medium.ttf COPYONLY)

file(GLOB THORCAM_DLL RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/thorcam "thorcam/*.dll")

foreach(FILENAME ${THORCAM_DLL})
	configure_file(thorcam/${FILENAME} 
				${FILENAME} COPYONLY)
endforeach()

file(GLOB THORLABS_DLL RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/thorlabs "thorlabs/*.dll")
foreach(FILENAME ${THORLABS_DLL})
	configure_file(thorlabs/${FILENAME} 
				${FILENAME} COPYONLY)
endforeach()

#create an executable
add_executable(muvitome
				muvitome.cpp
				thorcam/tl_camera_sdk_load.c
				thorcam/tl_mono_to_color_processing_load.c
				thorlabs/Thorlabs.MotionControl.Benchtop.StepperMotor.h
				thorlabs/TLDC4100.h
				thorlabs/vpptype.h
				ImGuiFileDialog/ImGuiFileDialog.cpp
)

target_link_libraries(muvitome
				PRIVATE glfw
				${OPENGL_LIBRARIES}
				PRIVATE GLEW::GLEW
				PRIVATE imgui::imgui
				PRIVATE glm::glm
				${CMAKE_CURRENT_SOURCE_DIR}/thorlabs/Thorlabs.MotionControl.Benchtop.StepperMotor.lib
				${CMAKE_CURRENT_SOURCE_DIR}/thorlabs/TLDC4100_64.lib
)