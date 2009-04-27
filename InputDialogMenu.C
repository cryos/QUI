/*!
 *  \file InputDialogMenu.C 
 *  
 *  \brief Contains slots and functions assocuated with the menu actions.  The
 *  separation of these functions is simply to keep the InputDialog.C filesize
 *  down.
 *  
 *  \author Andrew Gilbert
 *  \date   March 2008
 */

#include "InputDialog.h"
#include "Preferences.h"
#include "Process.h"
#include "Job.h"
#include "Qui.h"
#include <QMenuBar>
#include <QClipboard>
#include <QFileDialog>
#include <QFontDialog>
#include <QMessageBox>

#include <QtDebug>

namespace Qui {

void InputDialog::initializeMenus() {
   QMenuBar* menubar(menuBar());
   QAction* action;
   QString name;
   QMenu* menu;

   menubar->clear();

   // File
   menu = menubar->addMenu(tr("File"));

   // File -> Open
   name = "Open";
   action = menu->addAction(name);
   connect(action, SIGNAL(triggered()), this, SLOT(menuOpen()));
   action->setShortcut(QKeySequence::Open);

   // File -> Save
   name = "Save";
   action = menu->addAction(name);
   connect(action, SIGNAL(triggered()), this, SLOT(menuSave()));
   action->setShortcut(QKeySequence::Save);

   // File -> Save As
   name = "Save As";
   action = menu->addAction(name);
   connect(action, SIGNAL(triggered()), this, SLOT(menuSaveAs()));
   action->setShortcut(Qt::SHIFT + Qt::CTRL + Qt::Key_S );

   // File -> Quit
   name = "Quit";
   action = menu->addAction(name);
   connect(action, SIGNAL(triggered()), this, SLOT(menuClose()));
   action->setShortcut(QKeySequence::mnemonic("&Quit"));


   // Edit
   menu = menubar->addMenu(tr("Edit"));

   // Edit -> Copy
   name = "Copy";
   action = menu->addAction(name);
   connect(action, SIGNAL(triggered()), this, SLOT(menuCopy()));
   action->setShortcut(QKeySequence::Copy);

   // Edit -> Paste
   name = "Paste";
   action = menu->addAction(name);
   action->setShortcut(QKeySequence::Paste);

   // Edit -> Paste XYZ
   name = "Paste XYZ";
   action = menu->addAction(name);
   connect(action, SIGNAL(triggered()), this, SLOT(menuPasteXYZFromClipboard()));
   action->setShortcut(Qt::SHIFT + Qt::CTRL + Qt::Key_V);
   m_menuActions[name] = action;

   // Edit -> Select All
   name = "Select All";
   action = menu->addAction(name);
   action->setShortcut(QKeySequence::SelectAll);

   // Edit -> Undo
   name = "Undo";
   action = menu->addAction(name);
   connect(action, SIGNAL(triggered()), this, SLOT(menuUndo()));
   action->setShortcut(Qt::CTRL + Qt::Key_Z);

#ifndef Q_WS_MAC
   menu->addSeparator();
#endif

   // Edit -> Preferences
   name = "Preferences";
   action = menu->addAction(name);
   connect(action, SIGNAL(triggered()), this, SLOT(menuEditPreferences()));


    // Job
   menu = menubar->addMenu(tr("Job"));

   // Job -> New Job
   name = "New";
   action = menu->addAction(name);
   connect(action, SIGNAL(triggered()), this, SLOT(menuNew()));
   action->setShortcut(QKeySequence::New);

   // Job -> New Job
   name = "Append Job";
   action = menu->addAction(name);
   connect(action, SIGNAL(triggered()), this, SLOT(menuAppendJob()));
   action->setShortcut(QKeySequence::New);

   // Job -> Reset
   name = "Reset";
   action = menu->addAction(name);
   connect(action, SIGNAL(triggered()), this, SLOT(menuResetJob()));
   m_menuActions[name] = action;

   // Job -> Build Molecule
   name = "Build Molecule";
   action = menu->addAction(name);
   connect(action, SIGNAL(triggered()), this, SLOT(menuBuildMolecule()));
   action->setShortcut(Qt::CTRL + Qt::Key_B);
   m_menuActions[name] = action;

// We can only edit constraints from the geometry panel, but this might be
// useful later on.
/*
   // Job -> Edit
   QMenu* subMenu;
   subMenu = menu->addMenu(tr("Edit"));

   // Job -> Edit -> Geometry Constraints
   name = "Geometry Constraints";
   action = subMenu->addAction(name);
   connect(action, SIGNAL(triggered()), this, SLOT(editConstraints()));
//TEMPORARY
   action->setShortcut(Qt::CTRL + Qt::Key_G);
   m_menuActions[name] = action;
*/

   // Job -> Submit
   name = "Submit";
   action = menu->addAction(name);
   connect(action, SIGNAL(triggered()), this, SLOT(menuSubmit()));
   action->setShortcut(Qt::CTRL + Qt::Key_R );

   menu->addSeparator();

   // Job -> Process Monitor
   name = "Process Monitor";
   action = menu->addAction(name);
   connect(action, SIGNAL(triggered()), this, SLOT(menuProcessMonitor()));
   action->setShortcut(Qt::CTRL + Qt::Key_P );


   // Font
   menu = menubar->addMenu(tr("Font"));

   // Font -> Set Font
   name = "Set Font";
   action = menu->addAction(name);
   connect(action, SIGNAL(triggered()), this, SLOT(menuSetFont()));
   action->setShortcut(Qt::CTRL + Qt::Key_T);

   menu->addSeparator();

   // Font -> Bigger
   name = "Bigger";
   action = menu->addAction(name);
   connect(action, SIGNAL(triggered()), this, SLOT(menuBigger()));
   action->setShortcut(Qt::CTRL + Qt::Key_Plus);

   // Font -> Smaller 
   name = "Smaller";
   action = menu->addAction(name);
   connect(action, SIGNAL(triggered()), this, SLOT(menuSmaller()));
   action->setShortcut(Qt::CTRL + Qt::Key_Minus);
}



/********** File *********/

//! Prompts the user for a file to open. This can be either an existing
//! input/output file, or an xyz.
void InputDialog::menuOpen() {
   QFileInfo fileInfo;

   fileInfo.setFile(QFileDialog::getOpenFileName(this, tr("Open File"), 
      Preferences::LastFileAccessed()));

   if (fileInfo.exists() && fileInfo.isReadable()) {
      Preferences::LastFileAccessed(fileInfo.filePath());
      QFile file(fileInfo.filePath());
	  std::vector<Job*> jobs;
      QString coordinates;

      ReadInputFile(file, &jobs, &coordinates);

      if (!coordinates.isEmpty()) {
         insertXYZ(coordinates);
      }else if (jobs.size() != 0) {
         bool prompt(false);
         deleteAllJobs(prompt);
         std::vector<Job*>::iterator iter;
         for (iter = jobs.begin(); iter != jobs.end(); ++iter) {
             addJobToList(*iter);
         } 
         m_ui.jobList->setCurrentIndex(0);
         setWindowTitle("QChem Input File Editor - " + fileInfo.fileName());
         updatePreviewText();
         m_fileIn.setFile("");
      }
   }

   return;
}



/********** Edit *********/

//! Copies the text selected in the preview box to the clipboard
void InputDialog::menuCopy() {
   m_ui.previewText->copy();
}


void InputDialog::menuPasteXYZFromClipboard() {
   // This is mega-weird and either a bug in Qt, or there is something going on
   // with QClipboard that I don't know about.  Pasting the contents of the
   // clipboard into a QTextEdit seems to work no problem, but if I try to dump
   // the contents as a string, I only get the last line.
   QClipboard *clipboard = QApplication::clipboard();
   QString text(clipboard->text());
   QTextEdit* tmp = new QTextEdit();
   tmp->insertPlainText(text);
   text = tmp->toPlainText();
   delete tmp;

   insertXYZ(text);
}


//! Inserts the given coordinates into the current job
void InputDialog::insertXYZ(QString const& coordinates) {
   if (m_currentJob) {
      bool bailOnError(true);
      QString coords = ParseXyzFileContents(coordinates, bailOnError);

      if (coords.isEmpty()) { 
         QString msg("Invalid XYZ format.");
         QMessageBox::warning(0, "Parse Error", msg);
      }else {
         qDebug() << "    Setting coordinates";
         capturePreviewText(); // In case the user has messed with things
         m_currentJob->setCoordinates(coords);
         updatePreviewText();
      }
   }
}


//! This is supposed to allow a single undo action, but is not working yet.
void InputDialog::menuUndo() {
qDebug() << "undo called, last state:";
qDebug() << "vvvvvvvvvvvvvvvvvvvvvvvv";
qDebug() << m_rememberMe;
qDebug() << "^^^^^^^^^^^^^^^^^^^^^^^^";
   
   QString currentInput(m_ui.previewText->toPlainText());
   std::vector<Job*> jobs(ParseQChemFileContents(m_rememberMe));
   m_rememberMe = currentInput;

   if (jobs.size() > 0) {
      int jobNumber(currentJobNumber());
      bool prompt(false);
      deleteAllJobs(prompt);
      std::vector<Job*>::iterator iter;
      for (iter = jobs.begin(); iter != jobs.end(); ++iter) {
          addJobToList(*iter);
      } 
      m_ui.jobList->setCurrentIndex(jobNumber);
      updatePreviewText();
   }
}



/********** Job *********/

//! Prompts the user if they are happy to delete the current input and, if so,
//! adds a new job to the list.
void InputDialog::menuNew() {
   bool prompt(true);
   if (deleteAllJobs(prompt)) {
      appendNewJob();
   }
}


void InputDialog::appendNewJob() {
   Job* job = new Job();
   if (m_jobs.size() == 0) {
	  // The default Molecule section is set to "read", but for the first job
	  // we specify things explicitly.
      job->addSection("molecule", "0 1\n");  // HACK!!!
   }
   appendJob(job);
}


//! Adds the specified job to the list and updates the displayed output
//! by triggering the on_jobList_currentIndexChanged(int) slot.
void InputDialog::appendJob(Job* job) {
   addJobToList(job);
   m_ui.jobList->setCurrentIndex(m_jobs.size()-1);
}


//! Adds the specified job to the list, but does not update the displayed
//! output.  This prevents redundant updates when adding several jobs from a file.
void InputDialog::addJobToList(Job* job) {
   if (job != NULL) {
      m_jobs.push_back(job);

      QString comment(job->getComment());
      if (comment.trimmed().isEmpty()) {
          comment = "Job " + QString::number(m_jobs.size());
      }

      m_ui.jobList->addItem(comment);
      Q_ASSERT(m_ui.jobList->count() == int(m_jobs.size()));
   }
}



void InputDialog::menuResetJob() {
   resetControls();
   m_currentJob->init();
   updatePreviewText();
}


void InputDialog::menuProcessMonitor() {
   if (!m_processMonitor) {
      m_processMonitor = new Process::Monitor(this, m_processList);
      connect(m_processMonitor, SIGNAL(destroyed()),
         this, SLOT(processMonitorClosed()));
      connect(m_processMonitor, SIGNAL(processRemoved(Process*)),
         m_processQueue, SLOT(remove(Process*)));
   }

   m_processMonitor->show();
   m_processMonitor->raise();
   m_processMonitor->activateWindow(); 
}




/********** Font *********/

//! Prompts the user for a specific font to use in the preview box
void InputDialog::menuSetFont() {
   bool ok;
   QFont font(Preferences::PreviewFont());
   font = QFontDialog::getFont(&ok, font, this);
   if (ok) changePreviewFont(font);
}


void InputDialog::fontAdjust(bool up) {
   QFont font(Preferences::PreviewFont());
   int size = font.pointSize();
   size += up ? 1 : -1;
   font.setPointSize(size);
   changePreviewFont(font);
}


//! Changes the font used to display the preview and also updates the
//! associated preference.
void InputDialog::changePreviewFont(QFont const& font) {
   Preferences::PreviewFont(font);
   if (m_taint) updatePreviewText(); // Captures any changes in the preview box
   m_ui.previewText->clear();        // This indirectly sets m_taint to true...
   m_taint = false;                  // ...so we unset it here
   m_ui.previewText->setCurrentFont(font);
   updatePreviewText();
}


} // end namespace Qui
