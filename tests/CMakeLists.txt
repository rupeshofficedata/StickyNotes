# Tests run with the offscreen platform so they need no display.
function(sticky_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE Qt6::Test stickynotes_core stickynotes_editor Qt6::Qml Qt6::Quick)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_FORCE_STDERR_LOGGING=1")
endfunction()

sticky_test(tst_notestore)
sticky_test(tst_noteindex)
sticky_test(tst_richtextbridge)
sticky_test(tst_applifecycle)

add_test(NAME single_instance
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/single_instance_test.sh $<TARGET_FILE:stickynotes>)

# QML integration test: loads the real NoteWindow.qml from src/qml through a temporary qmldir module.
sticky_test(tst_notewindow_ui)
target_compile_definitions(tst_notewindow_ui PRIVATE QML_SOURCE_DIR="${CMAKE_SOURCE_DIR}/src/qml")
