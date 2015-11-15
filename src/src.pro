TEMPLATE = app
TARGET = openspin
DESTDIR = ../bin/

!greaterThan(QT_MAJOR_VERSION, 4): {
    error("PropellerIDE requires Qt5.2 or greater")
}
!greaterThan(QT_MINOR_VERSION, 1): {
    error("PropellerIDE requires Qt5.2 or greater")
}

CONFIG -= debug_and_release app_bundle
CONFIG += console

INCLUDEPATH += \
    base/PropellerCompiler \
    base/SpinSource \

SOURCES += \
    base/PropellerCompiler/BlockNestStackRoutines.cpp \
    base/PropellerCompiler/CompileDatBlocks.cpp \
    base/PropellerCompiler/CompileExpression.cpp \
    base/PropellerCompiler/CompileInstruction.cpp \
    base/PropellerCompiler/CompileUtilities.cpp \
    base/PropellerCompiler/DistillObjects.cpp \
    base/PropellerCompiler/Elementizer.cpp \
    base/PropellerCompiler/ErrorStrings.cpp \
    base/PropellerCompiler/ExpressionResolver.cpp \
    base/PropellerCompiler/InstructionBlockCompiler.cpp \
    base/PropellerCompiler/PropellerCompiler.cpp \
    base/PropellerCompiler/StringConstantRoutines.cpp \
    base/PropellerCompiler/SymbolEngine.cpp \
	base/PropellerCompiler/UnusedMethodUtils.cpp \
    base/PropellerCompiler/Utilities.cpp \
    base/SpinSource/flexbuf.cpp \
    base/SpinSource/objectheap.cpp \
    base/SpinSource/pathentry.cpp \
    base/SpinSource/preprocess.cpp \
    base/SpinSource/textconvert.cpp

HEADERS += \
    base/PropellerCompiler/CompileUtilities.h \
    base/PropellerCompiler/Elementizer.h \
    base/PropellerCompiler/ErrorStrings.h \
    base/PropellerCompiler/PropellerCompiler.h \
    base/PropellerCompiler/PropellerCompilerInternal.h \
    base/PropellerCompiler/SymbolEngine.h \
    base/PropellerCompiler/Utilities.h \
	base/PropellerCompiler/UnusedMethodUtils.h \
    base/SpinSource/flexbuf.h \
    base/SpinSource/objectheap.h \
    base/SpinSource/pathentry.h \
    base/SpinSource/preprocess.h \
    base/SpinSource/textconvert.h

SOURCES += \
    main.cpp \
    openspin.cpp \

HEADERS += \
    openspin.h \

