cmake_minimum_required(VERSION 3.21)
project(StickyNotes VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui Widgets DBus Qml Quick QuickControls2 Test)
# Each KDE Framework is its own CMake package (spike S3).
find_package(KF6DBusAddons REQUIRED)
find_package(KF6StatusNotifierItem REQUIRED)
qt_standard_project_setup(REQUIRES 6.5)
qt_policy(SET QTP0004 NEW)

add_library(stickynotes_core STATIC
    src/core/NoteStore.cpp src/core/NoteIndex.cpp src/core/AppLifecycle.cpp src/core/PlatformCaps.cpp)
target_include_directories(stickynotes_core PUBLIC src/core)
target_link_libraries(stickynotes_core PUBLIC Qt6::Core Qt6::Gui Qt6::DBus)

add_library(stickynotes_editor STATIC src/editor/RichTextBridge.cpp)
target_include_directories(stickynotes_editor PUBLIC src/editor)
target_link_libraries(stickynotes_editor PUBLIC stickynotes_core Qt6::Quick Qt6::Gui)

set_source_files_properties(src/qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(stickynotes src/main.cpp)
qt_add_qml_module(stickynotes
    URI StickyNotes
    VERSION 1.0
    QML_FILES
        src/qml/Main.qml
        src/qml/Theme.qml
        src/qml/PrimaryWindow.qml
        src/qml/NoteWindow.qml
        src/qml/NoteWindowManager.qml
        src/qml/ErrorBanner.qml
        src/qml/QuitPrompt.qml
        src/qml/SettingsPage.qml
        src/qml/BinView.qml
)
target_link_libraries(stickynotes PRIVATE
    stickynotes_core stickynotes_editor Qt6::Widgets Qt6::Qml Qt6::Quick Qt6::QuickControls2
    KF6::DBusAddons KF6::StatusNotifierItem)

include(GNUInstallDirs)
install(TARGETS stickynotes RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/io.github.rupeshofficedata.StickyNotes.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES packaging/io.github.rupeshofficedata.StickyNotes.service DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/services)
install(FILES packaging/io.github.rupeshofficedata.StickyNotes.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES packaging/io.github.rupeshofficedata.StickyNotes.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
