# Each test is a separate executable; all run offscreen via ctest.
function(paw_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE pawide_core Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

paw_add_test(tst_fileutils)
paw_add_test(tst_templates)
paw_add_test(tst_database)
paw_add_test(tst_fileops)
paw_add_test(tst_mainwindow)
