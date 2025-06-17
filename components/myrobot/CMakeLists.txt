include(CMakePrintHelpers)

set(srcs
        "src/Drive.cpp"
        "src/DriveMotor.cpp"
        "src/Drum.cpp"
        "src/PowerFunctions.cpp"
        "src/TaskManager.cpp")

 set(requires
         arduino
         bluepad32
         btstack)


idf_component_register(SRCS "${srcs}"
                INCLUDE_DIRS "include"
                REQUIRES ${requires})