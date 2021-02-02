Prerequisites:
1. Install NI software: LabVIEW 64 bit, Vision Development
2. Install Emergent eSDK & eCapture 2.23.10.20605 or above, to directory [EMERGENT_DIR]
3. The example project is [EMERGENT_DIR]/eSDK/Examples/EVT_VI/eCapture.lvproj
4. The sample project comes with the pre-imported Labview library EmergentCameraC.lvlib. It is recommended to import the dll again when you obtain a new version of eSDK installer to keep Labview library updated.

Build Labview library by importing C APIs from Emergent DLL:
1. Launch LabVIEW and click menu "Tools->Import->Shared Library(.dll)". See picture Import_dll.png.
2. In "Specify Create or Update mode" screen, choose "Create VIs for a shared library" and "Next". See picture Specify_Create_or_Update_mode.png.
3. In "Select Shared Library and Header File" screen. enter "[EMERGENT_DIR]\eSDK\bin\EmergentCameraC.dll" in the "Shared Library (.dll) File" edit box, enter "[EMERGENT_DIR]\eSDK\include\EmergentCameraC.h" in the "Header (.h) File" edit box, and Click "Next". See picture Select_Shared_Library_and_Header_File.png.
4. In "Configure Include Paths and Preprocessor Definitions" screen. enter "[EMERGENT_DIR]\eSDK\include" in the "Include Paths" edit box, enter "_MSC_VER;_WIN64;EMERGENTCAMERAC_EXPORTS;LABVIEW;" in the "Preprocessor Definitions" edit box, and Click "Next". See picture Configure_Include_Paths_and_Preprocessor_Definitions.png.
5. In "Select Functions to Convert" screen, keep all default settings and click "Next". See picture Select_Functions_to_Convert.png.
6. In "Configure Project Library Settings" screen. enter a Project Library Name in the "Project Library Name(.lvlib)" edit box, enter a Project Library Path [Project_Library_Path] in the "Project Library Path" edit box, and Click "Next". See picture  Configure_Project_Library_Settings.png.
7. Click "Next" in the rest screens until "Generation Progress" screen, which shows the generation progress with a progress bar. See Generation_Progress.png.
8. The generation will take a while. If no error occurs, "Finish" screen shows up. Click "Finish" to complete. See Finish.png.
9. The Labview library <name>.lvlib is generated under [Project_Library_Path]. And VI files are generated under [Project_Library_Path]\VIs, each file corresponding to to an imported C API.


Run the sample VI:
1. Open eCapture project by double clicking [eCapture_VI_DIR]\eCapture.lvproj.
2. Open the sample eCapture VI by double clicking eCapture.vi at the project tree. See eCapture_lvproj.png.
3. During the opening, system will search for the Emergent VIs which eCapture VI depends on. Choose the first one under [Project_Library_Path]. The system can locate the others from the same directory.
4. After eCapture.vi is successfully opened, it is ready to run. See eCapture_vi.png.
5. Connect one camera (only 1 camera is supported), click menu "Operate->Run".
6. eCapture runs. It shows the Dll Version, Vendor name, image size etc, and displays the images captured from the camera. User can adjust exposure and frame rate from sliders. See eCapture_vi_running.png.
7. User should click the "On/off" button the turn off eCapture.vi. Otherwise, the cameras won't be closed and resource won't be released.

Notes:
1. Only 1 camera can be connected.
2. Only handle and display 8-bit/pixel formats.
