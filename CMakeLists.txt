project(QUI)
# This is the CMake file which is used only when the QUI is compiled as a
# plugin for Avogadro.  The stand alone program should be compiled using
# qmake/make.

cmake_minimum_required(VERSION 2.6)

if(CMAKE_COMPILER_IS_GNUCXX)
  if(CMAKE_SYSTEM_NAME MATCHES Linux)
    set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--fatal-warnings -Wl,--no-undefined -lc${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--enable-new-dtags ${CMAKE_SHARED_LINKER_FLAGS}")
  endif(CMAKE_SYSTEM_NAME MATCHES Linux)
endif(CMAKE_COMPILER_IS_GNUCXX)

set(CMAKE_MODULE_PATH ${QUI_SOURCE_DIR}/cmake/modules)


# Find the Avogadro library if it is installed
find_package(Avogadro REQUIRED)
# Set up the build environment
include(${Avogadro_USE_FILE})
set(QT_USE_QTSQL true)
include(${QT_USE_FILE})
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${QT_QTSQL_INCLUDE_DIR})

find_package(Boost)
message("FindBoost::Boost_INCLUDE_DIR: ${Boost_INCLUDE_DIR}")

find_package(OpenBabel2 REQUIRED)

ADD_DEFINITIONS(${QT_DEFINITIONS})
ADD_DEFINITIONS(-DAVOGADRO)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${Boost_INCLUDE_DIR}
  ${PYTHON_INCLUDE_PATH}
  ${OPENBABEL2_INCLUDE_DIR}
)

### qchem
set(qchemextension_SRCS  
    Actions.C
    Conditions.C 
    GeometryConstraint.C
    InitializeQChemLogic.C       
    InputDialog.C
    InputDialogAvogadro.C
    InputDialogLogic.C
    InputDialogSlots.C
    InputDialogMenu.C
    Job.C
    Option.C             
    Process.C
    OptionDatabase.C
    OptSection.C
    MoleculeSection.C
    KeywordSection.C
    LJParametersSection.C
    ExternalChargesSection.C
    QtNode.C
    RemSection.C
    QChemExtension.C
    Preferences.C
    QuiAvogadro.C
    Qui.C)

set(qchemextension_MOC_HDRS
    InputDialog.h
    QChemExtension.h
    QtNode.h)

set(qchemextension_UIS
    FileDisplay.ui
    FindDialog.ui
    GeometryConstraintDialog.ui
    OptionDatabaseForm.ui
    OptionListEditor.ui
    OptionNumberEditor.ui
    InputDialog.ui
    PeriodicTable.ui
    PreferencesBrowser.ui
    ProcessMonitor.ui
    pt.ui)

#qt4_wrap_ui(qchemextension_UI_HDRS ${qchemextension_UIS})
#qt4_automoc(${qchemextension_SRCS})

avogadro_plugin(qchemextension
  "${qchemextension_SRCS}"
  "${qchemextension_UIS}")

target_link_libraries(qchemextension ${QT_QTSQL_LIBRARY})

