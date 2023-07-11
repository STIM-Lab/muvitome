#include <iostream>
#include <fstream>
#include <filesystem>
#include <Windows.h>
#include <tira/hardware/ThorlabsCamera.h>
#include <tira/hardware/ThorlabsKinesis.h>
#include <thorlabs/TLDC4100.h>

//these are libraries that Max added to the program
#include <serialib.h> //this is not attached to vcpkg and will have to be manually updated from github if needed.
                      //handles serial/COM communication from the arduino 
#include <string>
#include <stdio.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#ifndef IMGUI_DEFINE_MATH_OPERATORS
    #define IMGUI_DEFINE_MATH_OPERATORS
#endif // IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuiFileDialog/ImGuiFileDialog.h"
#include "tira/graphics_gl.h"


//this is stuff that Max added. This is how the serialib.cpp library opens serial/COM ports. The arduino is almost always attached to COM3.
#if defined (_WIN32) || defined(_WIN64)
//for serial ports above "COM9", we must use this extended syntax of "\\.\COMx".
//also works for COM0 to COM9.
//https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea?redirectedfrom=MSDN#communications-resources
#define SERIAL_PORT "\\\\.\\COM4" //change this if using a different COM port
#endif
#if defined (__linux__) || defined(__APPLE__)
#define SERIAL_PORT "/dev/ttyACM0"
#endif
//

//stuff that max added: to open serial port
serialib serial;                                     //Initialize serial object to use with the serialib library
bool automatic_mode = false;                         //Boolean to tell the program that we want to run using the automated system I have been developing.


//also stuff that Max added: global control variables
bool light_on = 1;                                      //is the microtome indicator light on? Light off = microtome is currently cutting a slice
bool light_changed = 0;                                 // has the microtome light changed?

int g_new_section_ready = 0;                            //are we ready to take a section?

GLFWwindow* window;                                     // pointer to the GLFW window that will be created (used in GLFW calls to request properties)
const char* glsl_version = "#version 130";              // specify the version of GLSL
ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);   // specify the OpenGL color used to clear the back buffer
float ui_scale = 1.5f;                                  // scale value for the UI and UI text
bool mouse_dragging = false;
double mouse_pos[2];
float render_pos[2] = { 0.0, 0.0 };                                     // camera position (center of the screen) in millimeters
float render_fov = 10;                                                  // camera fov in millimeters
float render_fov_step = 0.1;                            // camera zoom step (as a fraction of the current fov)
int display_w, display_h;
tira::glShader camera_image_shader;
tira::glGeometry rectangle;
float alpha = 0.5;

std::vector<tira::glTexture> TextureQueue;              // texture versions for each image in the image queue


ThorlabsCamera camera;
float detector_fov[2] = {1.0f, 0.5f};                   // detector field of view (in millimeters)
float detector_overlap[2] = { 15.0f, 15.0f };                         // detector overlap (percent)
int detector_bin = 2;                                   // detector binning (n x n)
tira::ThorlabsKinesis stage;                            // stage controller
//tira::ThorlabsKinesis aperture;                         // aperture controller
float y_range[2] = { 0, 0 };                            // y min and max positions
float x_range[2] = { 0, 0 };
int camera_delay = 1;
int camera_frames = 1;                                  // number of frames to integrate in every snapshot
float white_level = 1.0f;
float rgb_gain[3] = { 1.0f, 1.0f, 1.0f };
tira::image<unsigned char> LiveImage;                   // stores the current live frame
tira::glTexture LiveTexture;                            // texture representing the current live image

bool camera_live = false;
bool track_stage = false;

unsigned int frames[2];                                 // number of frames to acquire



std::string RepositoryDirectory;                        // directory name where all imaging projects will be written
std::string SamplePrefix;                                 // automatically generated subdirectory storing project images
std::string SampleNote;
std::string SettingsFile = "default.txt";               // file used to load and save default settings
int SampleImageCount = 0;

std::vector<tira::image<unsigned char>> ImageQueue;     // stores the current mosaic as a sequence of images
enum Command {Move, Image, Cut};
std::queue<Command> CommandQueue;                     // stores the current commands for collecting a mosaic

bool homed = false;                                     // flag identifies whether or not the stage has been homed this session
bool mosaic_hovered = false;

struct MuvitomePreset {
    std::string name;
    float fov[2];
};
std::vector<MuvitomePreset> Presets;
size_t SelectedPreset = 0;

std::string vertex_shader(
    "# version 330 core\n"

    "layout(location = 0) in vec3 vertices;\n"
    "layout(location = 2) in vec2 texcoords;\n"

    "uniform mat4 MVP;\n"

    "out vec4 vertex_color;\n"
    "out vec2 vertex_texcoord;\n"

    "void main() {\n"
        "gl_Position = MVP * vec4(vertices.x, vertices.y, vertices.z, 1.0f);\n"
        "vertex_texcoord = vec2(1.0 - texcoords.x, texcoords.y);\n"
    "};\n"    
);

std::string fragment_shader(
    "# version 330 core\n"

    "layout(location = 0) out vec4 color;\n"

    "in vec4 vertex_color;\n"
    "in vec2 vertex_texcoord;\n"
    "uniform sampler2D texmap;\n"
    "uniform float alpha;\n"

    "void main() {\n"
        "color = texture(texmap, vertex_texcoord);\n"
        "color.a = alpha;\n"
    "};\n"
);

void SaveSettings() {
    std::ofstream outfile(SettingsFile);
    outfile << RepositoryDirectory << std::endl;
    outfile << stage.getVelocity(2) << std::endl;
    outfile << x_range[0] << std::endl;
    outfile << x_range[1] << std::endl;
    outfile << y_range[0] << std::endl;
    outfile << y_range[1] << std::endl;
    outfile << camera.getExposure() << std::endl;
    outfile << detector_overlap[0] << std::endl;
    outfile << detector_overlap[1] << std::endl;
    outfile << SelectedPreset << std::endl;
    outfile << detector_fov[0] << std::endl;
    outfile << detector_fov[1] << std::endl;
    outfile << detector_bin << std::endl;
    outfile << white_level << std::endl;
    outfile.close();
}
void LoadSettings() {
    std::ifstream infile(SettingsFile);
    if (!infile) {
        std::cout << "WARNING: No settings file found - using default settings" << std::endl;
        return;
    }
    char buffer[1024];
    infile.getline(buffer, 1024);
    RepositoryDirectory = buffer;
    float velocity;
    infile >> velocity;
    stage.setVelocity(1, velocity);
    stage.setVelocity(2, velocity);
    infile >> x_range[0];
    infile >> x_range[1];
    infile >> y_range[0];
    infile >> y_range[1];
    float exposure;
    infile >> exposure;
    camera.setExposure(exposure);
    infile >> detector_overlap[0];
    infile >> detector_overlap[1];
    infile >> SelectedPreset;
    infile >> detector_fov[0];
    infile >> detector_fov[1];
    infile >> detector_bin;
    infile >> white_level;
    infile.close();
}
std::string ImageFileDirectory() {
    std::stringstream ss;
    ss << RepositoryDirectory << "\\" << SamplePrefix << SampleNote;
    return ss.str();
}
std::string ImageFileName() {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(5) << SampleImageCount << ".bmp";
    return ss.str();
}
std::string TileDirectory(size_t xi, size_t yi) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(5) << xi << "_" << std::setfill('0') << std::setw(5) << yi;
    return ss.str();
}

void SaveMosaic() {
    std::filesystem::path dest_path(ImageFileDirectory());				        // create a path object for the destination directory

    if (!std::filesystem::exists(dest_path)) {				                    // if the path doesn't exist, create it
        std::error_code ec;
        std::filesystem::create_directory(dest_path, ec);
        std::cout << "Repo Directory Creation: " << ec.message() << std::endl;
    }
    for (size_t i = 0; i < ImageQueue.size(); i++) {
        size_t yi = i / frames[0];                                  // calculate the y image index
        size_t xi = i - (yi * frames[0]);                           // calculate the x image index

        std::filesystem::path tile_path(ImageFileDirectory() + "\\" + TileDirectory(xi, yi));
        if (!std::filesystem::exists(tile_path)) {				                    // if the path doesn't exist, create it
            std::error_code ec;
            std::filesystem::create_directory(tile_path, ec);
            std::cout << "Tile Directory Creation: " << ec.message() << std::endl;
        }
        ImageQueue[i].save(ImageFileDirectory() + "\\" + TileDirectory(xi, yi) + "\\" + ImageFileName());
    }
    ImageQueue.clear();
    SampleImageCount++;
}
void UpdateFrames() {
    float sx = detector_fov[0] - (detector_fov[0] * detector_overlap[0] * 0.01);
    float sy = detector_fov[1] - (detector_fov[1] * detector_overlap[1] * 0.01);

    frames[0] = ceil((x_range[1] - x_range[0]) / sx);
    frames[1] = ceil((y_range[1] - y_range[0]) / sy);
}
void NewSample() {
    time_t now = time(0);                                                      // current date/time based on current system
    tm* ltm = localtime(&now);                                                 // get a time structure

    std::stringstream ss;                                                      // create a string stream for character processing
    ss << 1900 + ltm->tm_year << "_";
    ss << std::setw(2) << std::setfill('0') << ltm->tm_mon + 1 << "_";
    ss << std::setw(2) << std::setfill('0') << ltm->tm_mday << "_";
    ss << std::setw(2) << std::setfill('0') << ltm->tm_hour << "_";
    ss << std::setw(2) << std::setfill('0') << ltm->tm_min << "_";
    ss << std::setw(1) << std::setfill('0') << ltm->tm_sec << "_";

    SamplePrefix = ss.str();                                                    // save the sample prefix

    SampleImageCount = 0;                                                       // reset the sample image counter
}
void BeginMosaic() {
    ImageQueue.clear();
    TextureQueue.clear();                                                       // clear the image and texture queues

    for (size_t xi = 0; xi < frames[0]; xi++) {
        for (size_t yi = 0; yi < frames[1]; yi++) {
            CommandQueue.push(Command::Move);
            CommandQueue.push(Command::Image);
        }
    }
}

void ProcessCommandQueue() {
    if (stage.isMoving()) return;                                                   // if the stage is moving, return
    if (CommandQueue.size() == 0) {
        if (ImageQueue.size() != 0) SaveMosaic();
        return;
    }
    if (CommandQueue.front() == Command::Move) {
        size_t yi = ImageQueue.size() / frames[0];                                  // calculate the y image index
        size_t xi = ImageQueue.size() - (yi * frames[0]);                           // calculate the x image index
        float sx = detector_fov[0] - (detector_fov[0] * detector_overlap[0] * 0.01);
        float sy = detector_fov[1] - (detector_fov[1] * detector_overlap[1] * 0.01);
        float x = x_range[0] + xi * sx;
        float y = y_range[0] + yi * sy;
        stage.MoveTo(x, y, 0.0f);
        CommandQueue.pop();
        return;
    }
    else if (CommandQueue.front() == Command::Image) {
        std::cout << "Collecting Image..." << std::endl;

        std::cout << "Acquiring frame [0]..." << std::endl;
        camera.Snap();        
        tira::image<float> If_raw = camera.getImage8();
        tira::image<float> buffer = If_raw.bin(detector_bin, detector_bin);;
        for (unsigned int frame = 1; frame < camera_frames; frame++) {

            std::cout << "Acquiring frame ["<<frame<<"]..." << std::endl;
            camera.Snap();
            If_raw = camera.getImage8();
            buffer = buffer + If_raw.bin(detector_bin, detector_bin);
        }
        float summed_components = detector_bin * detector_bin * camera_frames;
        float color_scale = 1.0f / (summed_components * white_level * 255.0f) * 255.0f;
        tira::image<unsigned char> I = (buffer * color_scale).clamp(0, 255);
        ImageQueue.push_back(I);
        tira::glTexture T(I.data(), I.width(), I.height(), 0, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
        T.SetFilter(GL_NEAREST);
        TextureQueue.push_back(T);
        
        CommandQueue.pop();
        return;
    }
    else if (CommandQueue.front() == Command::Cut) {
        //tell microtome to cut here
        serial.writeChar('c');
        CommandQueue.pop();
        return;
    }
    
}

/// <summary>
/// This function renders the user interface every frame
/// </summary>
void RenderUI() {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    

    bool DisableAll = false;
    if (CommandQueue.size() != 0) DisableAll = true;
    if(DisableAll) ImGui::BeginDisabled();

    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("MUVitome Control");                                   // Create a window called "Hello, world!" and append into it.
        // open Dialog Simple
        if (ImGui::Button("Image Repository"))
            ImGuiFileDialog::Instance()->OpenDialog("ChooseDirDlgKey", "Choose a Directory", nullptr, ".");

        // display
        if (ImGuiFileDialog::Instance()->Display("ChooseDirDlgKey"))
        {
            // action if OK
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                RepositoryDirectory = ImGuiFileDialog::Instance()->GetCurrentPath();
                NewSample();
                SaveSettings();
            }
            // close
            ImGuiFileDialog::Instance()->Close();
        }
        
        // Specify repository directories
        ImGui::Text(RepositoryDirectory.c_str());
        ImGui::Text(SamplePrefix.c_str());
        ImGui::SameLine();
        static char buffer[1024];
        if (ImGui::InputText("##SampleNote", buffer, 1024, ImGuiInputTextFlags_EnterReturnsTrue)) {
            SampleNote = buffer;
            NewSample();
        }

        {                                                       // begin stage control
            bool stage_disabled = false;
            bool camera_disabled = false;
            if (stage.isMoving()) {
                ImGui::BeginDisabled();
                stage_disabled = true;
            }
            if (ImGui::Button("Home")) {
                stage.Home(1);
                stage.Home(2);
                homed = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Already Homed")) {
                ImGui::OpenPopup("AreYouSure");
            }
            if (ImGui::BeginPopup("AreYouSure")) {
                ImGui::Checkbox("ARE YOU SURE?", &homed);
                ImGui::EndPopup();
            }

            // if the stage has not been homed, disable it
            if (!homed && !stage_disabled) {
                ImGui::BeginDisabled();
                stage_disabled = true;
            }

            //MAX ADDED, check the automatic mode checkbox if want use the automated system I've been developing
            ImGui::Checkbox("Automatic mode?", &automatic_mode);

            // Collect mosaics
            if (camera_live) camera_disabled = true;

            if (camera_disabled) ImGui::BeginDisabled();
            if (ImGui::Button("Mosaic")) {
                Sleep(camera_delay * 1000);
                BeginMosaic();
            }
            mosaic_hovered = false;
            if (ImGui::IsItemHovered())
                mosaic_hovered = true;
            
            if (camera_disabled) ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::Text("Section %d, Image %d / %d", SampleImageCount, ImageQueue.size(), frames[0] * frames[1]);
            // X minimum and max position moves
            if (ImGui::Button("<-##xmin")) {
                stage.MoveToX(x_range[0]);
            }
            ImGui::SameLine();
            if (ImGui::Button("->##xmax")) {
                stage.MoveToX(x_range[1]);
            }
            ImGui::SameLine();
            if (ImGui::SliderFloat2("[X]", x_range, 0.0f, 100.0f, "%.2fmm")) {
                if (x_range[1] < x_range[0]) x_range[0] = x_range[1];
                NewSample();
            }
            

            // Y minimum and max position moves
            if (ImGui::Button("<-##ymin")) {
                stage.MoveToY(y_range[0]);
            }
            ImGui::SameLine();
            if (ImGui::Button("->##ymax")) {
                stage.MoveToY(y_range[1]);
            }
            ImGui::SameLine();
            if (ImGui::SliderFloat2("[Y]", y_range, 0.0f, 100.0f, "%.2fmm")) {
                if (y_range[1] < y_range[0]) y_range[0] = y_range[1];
                NewSample();
            }
            

            // Center Button
            if (ImGui::Button("Center")) {
                stage.MoveToY((y_range[1] - y_range[0]) / 2.0 + y_range[0]);
                stage.MoveToX((x_range[1] - x_range[0]) / 2.0 + x_range[0]);
            }
            

            ImGui::Text("Stage Position");
            float stage_pos[2];
            stage_pos[0] = stage.getPosition(1);
            stage_pos[1] = stage.getPosition(2);
            
            if (ImGui::InputFloat("X", &stage_pos[0], 0.1, 1, "%.1f mm")) {
                stage.MoveToX(stage_pos[0]);
            }
            if (ImGui::InputFloat("Y", &stage_pos[1], 0.1, 1, "%.1f mm")) {
                stage.MoveToY(stage_pos[1]);
            }

            if (stage_disabled) ImGui::EndDisabled();
            
            UpdateFrames();
            ImGui::Text("Frames: %i x %i", frames[0], frames[1]);
        }                                                       // end stage control
        
        stage.UpdateStatus();
        
        ImGui::End();                                                           // End MUVitome control

        ImGui::Begin("Stage and Camera Settings");
        float gui_velocity = stage.getVelocity(2);                                          // get the velocity for the gui
        if (ImGui::InputFloat("velocity", &gui_velocity, 1.0f, 5.0f, "%.0f m/s")) {        // display the velocity and allow the user to change it
            stage.setVelocity(1, gui_velocity);
            stage.setVelocity(2, gui_velocity);                                             // set the velocity
        }
        float gui_exposure = camera.getExposure();
        if (ImGui::InputFloat("exposure", &gui_exposure, 10.0f, 100.0f, "%.0f (ms)")) {
            camera.setExposure(gui_exposure);
        }
        ImGui::SliderFloat("white level", &white_level, 0.0f, 1.0f);
        ImGui::SliderFloat3("RGB Gains", rgb_gain, 0.0f, 5.0f);
        ImGui::InputInt("Camera Delay", &camera_delay, 1, 2);
        if (camera_delay < 0) camera_delay = 0;

        ImGui::InputInt("Camera Frames", &camera_frames, 1, 2);
        if (camera_frames <= 0) camera_frames = 1;

        if (ImGui::BeginCombo("Objective", Presets[SelectedPreset].name.c_str())) {
            for (size_t ci = 0; ci < Presets.size(); ci++) {
                const bool is_selected = (SelectedPreset == ci);
                if (ImGui::Selectable(Presets[ci].name.c_str(), is_selected)) {
                    SelectedPreset = ci;
                    detector_fov[0] = Presets[SelectedPreset].fov[0];
                    detector_fov[1] = Presets[SelectedPreset].fov[1];
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::InputFloat2("FOV", detector_fov, "%.3f mm");
        if (detector_fov[0] < 0.001f) detector_fov[0] = 0.001f;
        if (detector_fov[1] < 0.001f) detector_fov[1] = 0.001f;
        ImGui::InputFloat2("Overlap", detector_overlap, "%.1f %%");
        ImGui::InputInt("Bins", &detector_bin, 1, 2);
        if (detector_bin <= 0) detector_bin = 1;
        ImGui::SliderFloat("Alpha", &alpha, 0.0f, 1.0f);

        if (ImGui::Button("Snap")) {
            camera.Snap();
            tira::image<unsigned char> I = camera.getImage8();
            //SaveImage(I);
            SampleImageCount++;
        }
        ImGui::SameLine();
        ImGui::Checkbox("Live Image", &camera_live);
        if (ImGui::Button("Center Display")) {
            render_pos[0] = stage.getPosition(1);
            render_pos[1] = stage.getPosition(2);
        }
        ImGui::SameLine();
        ImGui::Checkbox("Track Stage", &track_stage);
        ImGui::Text("Camera Position: %.1f, %.1f", render_pos[0], render_pos[1]);

        ImGui::End();
        if (DisableAll) ImGui::EndDisabled();


    }

    //ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);  // Render a separate window showing the FPS
    //ImGui::Text(.Log().c_str());

    ImGui::Render();                                                            // Render all windows
}

/// <summary>
/// Initialize the GUI
/// </summary>
/// <param name="window">Pointer to the GLFW window that will be used for rendering</param>
/// <param name="glsl_version">Version of GLSL that will be used</param>
void InitUI(GLFWwindow* window, const char* glsl_version) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    ImGui::GetStyle().ScaleAllSizes(ui_scale);
    ImGui::GetIO().FontGlobalScale = ui_scale;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    io.Fonts->AddFontFromFileTTF("Roboto-Medium.ttf", ui_scale * 16.0f);

}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        mouse_dragging = true;
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
        mouse_dragging = false;
}
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (mouse_dragging) {
        double distance[2] = { mouse_pos[0] - xpos , mouse_pos[1] - ypos };
        double pixelstep = render_fov / (double)display_h;
        render_pos[0] += distance[0] * pixelstep;
        render_pos[1] += distance[1] * pixelstep;
    }
    mouse_pos[0] = xpos;
    mouse_pos[1] = ypos;
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    render_fov -= yoffset * render_fov_step * render_fov;
}

/// keyboard callback function used to trigger the camera when the space bar is pressed
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {       
        if (!(stage.isMoving()) && mosaic_hovered) {
            Sleep(camera_delay * 1000);
            BeginMosaic();
        }
    }
}

/// <summary>
/// Destroys the ImGui rendering interface (usually called when the program closes)
/// </summary>
void DestroyUI() {
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void InitPresets() {
    MuvitomePreset x15;
    x15.name = "15X Cassegrain";
    x15.fov[0] = .903;
    x15.fov[1] = .474;
    Presets.push_back(x15);

    MuvitomePreset x40;
    x40.name = "40X Cassegrain";
    x40.fov[0] = .345;
    x40.fov[1] = .180;
    Presets.push_back(x40);
}

//function that Max added to open the serial port that the arduino input is passed in from
char openSerial() {
    char errorOpening = serial.openDevice(SERIAL_PORT, 9600);
    // If connection fails, return the error code otherwise, display a success message
    if (errorOpening != 1) return errorOpening;
        printf("Successful connection to %s\n", SERIAL_PORT);
}

//function that Max added to read the status of the indicator light from the arduino
void UpdateLightFromArduino() {

    char status;

    //// read a character (return 0 if there is no character)
    bool char_read = serial.readChar(&status, 1);       // get the current status from the Arduino

    if(char_read == 0) return;                          // if there is no data from the Arduino, then nothing has changed

    if (char_read == 1) {                               // if the Arduino has submitted data
        if (status == '0') {                            // if the Arduino passed a character '0', then the light is off
            if (light_on) {                             // if the light variable is currently ON
                light_on = false;                       //      turn the light variable OFF
                light_changed = true;                   //      indicate that the light has changed
            }
        }
        else {                                          // if the Arduno passes a character that ISN'T '0', then the light is on
            if (!light_on) {                            // if the light variable is currently OFF
                light_on = true;                        //      turn the light variable ON
                light_changed = true;                   //      indicate that the light has changed
            }
        }
    }
}

int main(int argc, char** argv) {

    openSerial(); //Open the serial port--do I need to assign the error code to some variable? see warning message

    // Initialize hardware
	camera.Init();
    stage.Init(0, 70);

    // Load settings
    LoadSettings();
    NewSample();                                                        // initialize parameters to start a new sample (directory names, etc.)

    // Setup window
    glfwSetErrorCallback(glfw_error_callback);                          // sets a function that is called if there is a GLFW error
    if (!glfwInit())                                                    // initialize GLFW and exit if it isn't working
        return 1;

    // GL 3.0 + GLSL 130
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);                      // ask for OpenGL 3.0 if available
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window with graphics context
    window = glfwCreateWindow(1600, 1200, "MUVitome", NULL, NULL);      // creates a GLFW window
    if (window == NULL)                                                 // if something is wrong with the window, exit
        return 1;
    glfwMakeContextCurrent(window);                                     // makes the current window active
    glfwSwapInterval(1); // Enable vsync

    GLenum err = glewInit();                                            // enable GLEW (this is a feature manager for OpenGL because Windows is dumb)
    if (GLEW_OK != err){
        /* Problem: glewInit failed, something is seriously wrong. */
        printf("Error: %s\n", glewGetErrorString(err));
    }
    printf("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));     // print the version of GLEW to make sure it's working

    //set input callbacks
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    InitUI(window, glsl_version);                                       // initialize ImGUI

    // initialize the mosaic display geometry and shader
    camera_image_shader.CreateShader(vertex_shader, fragment_shader);
    rectangle = tira::glGeometry::GenerateRectangle<float>();
    render_pos[0] = x_range[0];
    render_pos[1] = y_range[0];
    render_fov = y_range[1] - y_range[0];

    InitPresets();

    // Main loop
    while (!glfwWindowShouldClose(window)){

        

        glfwPollEvents();                                       // Poll and handle events (inputs, window resize, etc.)
        
        if (automatic_mode && CommandQueue.empty()) {           // if Automatic Mode is ON and the command queue is empty (we can start a new mosaic)

            UpdateLightFromArduino();                           // Check the Arduino and update the light status
            if (light_changed) {
                if (light_on) {                                 // if the light changed to ON
                    std::cout << "Light Just Turned ON" << std::endl;
                    BeginMosaic();
                    light_changed = false;
                    CommandQueue.push(Command::Cut);
                    
                }
                if (!light_on) {                                // if the light changed to OFF
                    // do nothing
                    std::cout << "Light Just Turned OFF" << std::endl;
                    light_changed = false;
                }
            }
        }
            
        if (camera_live) {
            camera.Snap();
            LiveImage = camera.getImage8();
            LiveTexture.AssignImage(LiveImage.data(), LiveImage.width(), LiveImage.height(), 0, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
            //tira::glTexture T(I.data(), I.width(), I.height(), 0, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
        }
        else {
            ProcessCommandQueue();
        }

        RenderUI();
        
        glfwGetFramebufferSize(window, &display_w, &display_h);

        glViewport(0, 0, display_w, display_h);                     // specifies the area of the window where OpenGL can render
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);

        glClear(GL_COLOR_BUFFER_BIT);                               // clear the Viewport using the clear color
        glBlendFunc(GL_ONE, GL_ONE);
        glEnable(GL_BLEND);

        /****************************************************/
        /*      Draw Stuff To The Viewport                  */
        /****************************************************/
        // Render the mosaic here
        float aspect_ratio = (float)display_w / (float)display_h;   // calculate the aspect ratio
        float view_height = render_fov;                             // initialize the view height (in millimeters)
        float view_width = view_height * aspect_ratio;              // calculate the view width (in millimeters) from the aspect ratio

        glm::mat4 projection = glm::ortho(-view_width / 2.0f, view_width / 2.0f, -view_height / 2.0f, view_height / 2.0f, -10.0f, 10.0f);

        if (track_stage) {
            render_pos[0] = stage.getPosition(1);
            render_pos[1] = stage.getPosition(2);
        }
        glm::mat4 view = glm::lookAt(glm::vec3(render_pos[0], render_pos[1], -1.0f), glm::vec3(render_pos[0], render_pos[1], 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        
        camera_image_shader.Bind();
        glm::vec3 pos;
        float image_spacing[2];
        float overlap_fraction[2] = { detector_overlap[0] * 0.01, detector_overlap[1] * 0.01 };
        image_spacing[0] = detector_fov[0] - detector_fov[0] * (overlap_fraction[0]);
        image_spacing[1] = detector_fov[1] - detector_fov[1] * (overlap_fraction[1]);

        // if the camera is capturing a live image, render that
        if (camera_live) {
            LiveTexture.Bind();
            glm::mat4 model = glm::mat4(1.0);
            pos[0] = stage.getPosition(1);
            pos[1] = stage.getPosition(2);
            model = glm::translate(model, glm::vec3((float)(pos[0]), (float)(pos[1]), 0.0f));
            //model = glm::translate(model, glm::vec3(0, 0, 0));
            model = glm::scale(model, glm::vec3((float)detector_fov[0], (float)detector_fov[1], 1.0));
            
            glm::mat4 mvp = projection * view * model;
            camera_image_shader.SetUniformMat4f("MVP", mvp);
            camera_image_shader.SetUniform1f("alpha", 1.0f);
            rectangle.Draw();
        }
        // otherwise render the most recent mosaic
        else {
            for (size_t t = 0; t < TextureQueue.size(); t++) {
                TextureQueue[t].Bind();
                glm::mat4 model = glm::mat4(1.0);
                //CALCULATE POSITION pos
                size_t yi = t / frames[0];                                  // calculate the y image index
                size_t xi = t - (yi * frames[0]);                           // calculate the x image index
                pos[0] = x_range[0] + xi * image_spacing[0];
                pos[1] = y_range[0] + yi * image_spacing[1];
                model = glm::translate(model, glm::vec3((float)(pos[0]), (float)(pos[1]), 0.0f));

                model = glm::scale(model, glm::vec3((float)detector_fov[0], (float)detector_fov[1], 1.0));
                glm::mat4 mvp = projection * view * model;
                camera_image_shader.SetUniformMat4f("MVP", mvp);
                camera_image_shader.SetUniform1f("alpha", alpha);
                rectangle.Draw();
            }
        }


        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());     // draw the GUI data from its buffer

        glfwSwapBuffers(window);                                    // swap the double buffer
    }


    DestroyUI();                                                    // Clear the ImGui user interface

    glfwDestroyWindow(window);                                      // Destroy the GLFW rendering window
    glfwTerminate();                                                // Terminate GLFW

    SaveSettings();

    camera.Destroy();
    stage.Destroy();
    serial.closeDevice();

    return 0;
	
}