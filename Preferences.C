/*!
 *  \file Preferences.C
 *  
 *  \author Andrew Gilbert
 *  \date   March 2009
 */

#include "Preferences.h"
#include <QFileDialog>
#include <QSettings>



namespace Qui {
namespace Preferences {


static QString s_organization = "Q-Chem Inc";
static QString s_application  = "QUI";


// **********  Browser  ********* //
Browser::Browser(QWidget* parent) : QDialog(parent) {
   m_ui.setupUi(this);
   // These are the options we allow the user to edit in the Browser.  If you
   // add another option here, then you also need to update the
   // Browser::on_okButtonClicked function below.
   m_ui.lineEditRunQChem->setText(QChemRunScript());
   m_ui.lineEditAvogadro->setText(AvogadroPath());
}


void Browser::on_okButton_clicked(bool)  {
   QChemRunScript(m_ui.lineEditRunQChem->text());
   AvogadroPath(m_ui.lineEditAvogadro->text());
   NumberOfProcesses(m_ui.numberOfProcesses->value());
}


void Browser::on_browseRunQChemButton_clicked(bool) {
   setFilePath(m_ui.lineEditRunQChem);
}


void Browser::on_browseAvogadroButton_clicked(bool) {
   setFilePath(m_ui.lineEditAvogadro);
}


//! Convenience function that opens a file browser so the user can specify a
//! directory.
void Browser::setPath(QLineEdit* edit) {
   QString s(edit->text());
   QString path(QFileDialog::getExistingDirectory(this, tr("Select Path"), s));
   if (!path.isEmpty()) edit->setText(path);
}


//! Convenience function that opens a file browser so the user can specify a
//! file.
void Browser::setFilePath(QLineEdit* edit) {
   QString s(edit->text());
   QString path(QFileDialog::getOpenFileName(this, tr("Select File"), s));
   if (!path.isEmpty()) edit->setText(path);
}



// **********  Non-member Access Functions  ********* //
// The following functions give access to individual options and provide the
// interface for changing options.  When adding preferences, a new (overloaded)
// function should be created to get and set the option.  This should provide a
// default if the preference does not exist in the preference file, and also
// take care of the necessary conversion from the QVariant.


// Size of the InputDialog window
QSize MainWindowSize() {
   QVariant value(Get("MainWindowSize"));
   return value.isNull() ? QSize(960,570) : value.value<QSize>();
}

void MainWindowSize(QSize const& size) {
   Set("MainWindowSize", QVariant::fromValue(size));
}



// Size of the Output file display
QSize FileDisplayWindowSize() {
   QVariant value(Get("FileDisplayWindowSize"));
   return value.isNull() ? QSize(550,400) : value.value<QSize>();
}

void FileDisplayWindowSize(QSize const& size) {
   Set("FileDisplayWindowSize", QVariant::fromValue(size));
}



// Font for the InputDialog previewText display
QFont PreviewFont() {
   QVariant value(Get("PreviewTextFont"));
   return value.isNull() ? QFont("Courier New") : value.value<QFont>();
}

void PreviewFont(QFont const& font) {
   Set("PreviewTextFont", QVariant::fromValue(font));
}



// Font for displaying the output files.
QFont FileDisplayFont() {
   QVariant value(Get("FileDisplayFont"));
   return value.isNull() ? QFont("Courier New") : value.value<QFont>();
}

void FileDisplayFont(QFont const& font) {
   Set("FileDisplayFont", QVariant::fromValue(font));
}



// FileDialogs should use this to initialize the filename when opening and
// saving files
QString LastFileAccessed() {
   QVariant value(Get("LastSavedFile"));
   return value.isNull() ? QString() : value.value<QString>();
}

void LastFileAccessed(QString const& filePath) {
   Set("LastSavedFile", QVariant::fromValue(filePath));
}



// Path to the avogadro application
QString AvogadroPath() {
   QVariant value(Get("AvogadroPath"));
   return value.isNull() ? QString() : value.value<QString>();
}

void AvogadroPath(QString const& path) {
   Set("AvogadroPath", QVariant::fromValue(path));
}



// Path to the QChem script
QString QChemRunScript() {
   QVariant value(Get("QChemRunScript"));
   return value.isNull() ? QString() : value.value<QString>();
}

void QChemRunScript(QString const& filePath) {
   Set("QChemRunScript", QVariant::fromValue(filePath));
}



// Number of concurrent processes to run
int NumberOfProcesses() {
   QVariant value(Get("NumberOfProcesses"));
   return value.isNull() ? 1 : value.value<int>();
}

void NumberOfProcesses(int n) {
   Set("NumberOfProcesses", QVariant::fromValue(n));
}



//! Retrieves a preference setting from the global QSettings object.
//! Should not be used outside the Preferences namespace.
QVariant Get(QString const& name) {
   QSettings settings(QSettings::UserScope, s_organization, s_application);
   return settings.value(name);
}


//! Changes a preference setting in the global QSettings object.
//! Should not be used outside the Preferences namespace.
void Set(QString const& name, QVariant const& value) {
   QSettings settings(QSettings::UserScope, s_organization, s_application);
   settings.setValue(name,value);
}


} } // end namespace Qui::Preferences
