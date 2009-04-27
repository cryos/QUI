# This is the CMake file which is used only when the QUI is compiled as a
# plugin for Avogadro.  The stand alone program should be compiled using
# qmake/make.

set(QT_USE_QTOPENGL true)

SET(QT_USE_QTSQL true)

include(${QT_USE_FILE})

find_package(Boost)
message("FindBoost::Boost_INCLUDE_DIR: ${Boost_INCLUDE_DIR}")


ADD_DEFINITIONS(${QT_DEFINITIONS})
ADD_DEFINITIONS(-DQT_PLUGIN)
ADD_DEFINITIONS(-DQT_SHARED)
ADD_DEFINITIONS(-DAVOGADRO)

set(DESTINATION_DIR ${LIB_INSTALL_DIR}/avogadro/extensions)

include_directories(
    ../
	${CMAKE_CURRENT_BINARY_DIR}
	${OPENBABEL2_INCLUDE_DIR}
	${EIGEN_INCLUDE_DIR}
	${AVOGADRO_INCLUDE_DIR}
	${Boost_INCLUDE_DIR}
	${PYTHON_INCLUDE_PATH}
)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/..)

set(LINK_LIBS "avogadro-lib")

### qchem
set(qchemextension_SRCS  
    Actions.C
    Conditions.C 
    InitializeQChemLogic.C       
    InputDialog.C
    InputDialogAvogadro.C
    InputDialogLogic.C
    InputDialogSlots.C
    Job.C
    Option.C             
    OptionDatabase.C
    OutputDisplay.C
    QChemExtension.C
    QuiAvogadro.C
    Qui.C)

set(qchemextension_MOC_HDRS
    InputDialog.h
    OutputDisplay.h
    QChemExtension.h
    QtNode.h)

set(qchemextension_UIS
    OutputDisplay.ui
    InputDialog.ui)



qt4_wrap_ui(qchemextension_UI_HDRS ${qchemextension_UIS})
qt4_wrap_cpp(qchemextension_MOC_SRCS ${qchemextension_MOC_HDRS}) 
#${qchemextension_SRCS})

add_library(qchemextension MODULE ${qchemextension_SRCS} ${qchemextension_MOC_SRCS}
	${qchemextension_UIS_H})
target_link_libraries(qchemextension ${LINK_LIBS} ${QT_LIBRARIES})
install(TARGETS qchemextension DESTINATION ${DESTINATION_DIR})
