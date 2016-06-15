TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += include \
               depends/moodycamel/include \
               depends/yaml-cpp/include \
               depends/glm-0.9.7.4/include \
               depends/spdlog/include

QMAKE_CXXFLAGS_RELEASE += -O3 -msse4.1 -mssse3 -msse3 -msse2 -msse2 -DGLM_FORCE_INLINE -DSPDLOG_NO_THREAD_ID -DSPDLOG_NO_NAME
QMAKE_CXXFLAGS_DEBUG += -DSPDLOG_DEBUG_ON -DSPDLOG_TRACE_ON -DSPDLOG_NO_THREAD_ID -DSPDLOG_NO_NAME
#QMAKE_CXXFLAGS_DEBUG += -O3 -msse4.1 -mssse3 -msse3 -msse2 -msse2 -DGLM_FORCE_INLINE

# Platform-specific configuration
#################################


# Mac OS X
macx {
    QMAKE_LFLAGS += -pagezero_size 10000 -image_base 100000000
    INCLUDEPATH += /Library/Frameworks/SDL2.framework/Headers \
                   /usr/local/Cellar/glew/1.13.0/include \
                   /usr/local/include/luajit-2.0
    LIBS += -framework OpenGL \
            -F/Library/Frameworks -framework SDL2 \
            -L/usr/local/Cellar/glew/1.13.0/lib -lGLEW \
            -L/Users/dankersten/code/c++/yaml-cpp/build -lyaml-cpp \
            /usr/local/lib/libluajit-5.1.a
}

# Windows
win32 {
    !contains(QMAKE_TARGET.arch, x86_64) {
        # 32bit Windows
        error("32bit is not supported.")
    } else {
        # 64bit Windows

    }
}

# Linux
unix:!macx {

}

DISTFILES +=

SOURCES += src/core/main.cpp \
    src/util/telemetry.cpp \
    src/util/config.cpp \
    src/util/logging.cpp \
    src/window/window.cpp \
    src/graphics/shader.cpp

HEADERS += \
    include/util/telemetry.h \
    include/util/config.h \
    include/util/logging.h \
    include/window/window.h \
    include/graphics/shader.h \
    include/util/stb_image.h \
    include/graphics/tilemap.h \
    include/graphics/mesh.h
