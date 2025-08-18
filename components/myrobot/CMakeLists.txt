include(CMakePrintHelpers)

set(srcs
        "src/Drive.cpp"
        "src/DriveMotor.cpp"
        "src/Drum.cpp"
        "src/PowerFunctions.cpp"
        "src/TaskManager.cpp"
        "src/Buttons.cpp"
        "src/LED.cpp"
        "src/OtaUpdater.cpp"
        "src/WebInterface.cpp"
        "src/rgbLED.cpp"
        "src/Adafruit_NeoPixel.cpp"
        "src/esp.c")

 set(requires
         arduino
         bluepad32
         btstack)


idf_component_register(SRCS "${srcs}"
                INCLUDE_DIRS "include"
                REQUIRES ${requires})