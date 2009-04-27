######################################################################
#
#  This is the QUI project file
#
######################################################################

TEMPLATE     = app
INCLUDEPATH += .
DEPENDPATH  += .
CONFIG      += no_keywords 
RESOURCES   += QUI.qrc
ICON         = resources/icons/qchem.png
QT          += sql



#################################
# System specific configuration #
#################################

macx {
   INCLUDEPATH += /usr/local/include/boost-1_35 
   INCLUDEPATH += /Library/Frameworks/QtSql.framework/Headers
   CONFIG += ppc debug

#  release {
#     QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.4u.sdk
#     CONFIG += ppc x86 release
#  }

}

win32 {
   INCLUDEPATH += "C:\Program Files\boost\boost_1_36_0"
   CONFIG += release
}

linux {
   CONFIG += release
}



###############
# Input Files #
###############

HEADERS += OptionDatabaseForm.h OptionEditors.h InputDialog.h Node.h \
		   QtNode.h Register.h OptionDatabase.h Conditions.h Option.h \
		   Actions.h Qui.h Job.h FileDisplay.h KeywordSection.h \ 
		   RemSection.h Preferences.h MoleculeSection.h \
		   GeometryConstraint.h OptSection.h ExternalChargesSection.h \
           LJParametersSection.h FindDialog.h Process.h
           
SOURCES += main.C OptionDatabaseForm.C Option.C OptionDatabase.C \
           OptionEditors.C Conditions.C Actions.C \
		   InputDialogLogic.C InputDialogSlots.C InitializeQChemLogic.C \
           Job.C Qui.C FileDisplay.C KeywordSection.C ReadInput.C \
           RemSection.C Preferences.C MoleculeSection.C InputDialog.C \
		   GeometryConstraint.C  OptSection.C ExternalChargesSection.C \
           LJParametersSection.C FindDialog.C Process.C InputDialogMenu.C \
           ProcessQChemKill.C getpids.C

FORMS += OptionDatabaseForm.ui OptionListEditor.ui OptionNumberEditor.ui \
         FileDisplay.ui QuiMainWindow.ui PreferencesBrowser.ui \
         GeometryConstraintDialog.ui FindDialog.ui ProcessMonitor.ui
