/*!
 *  \file InputDialogSlots.C 
 *  
 *  \brief Contains all the slots for the InputDialog class.
 *  
 *  \author Andrew Gilbert
 *  \date   August 2008
 */

#include <cstdlib>

#include <QList>
#include <QMessageBox>
#include <QFileDialog>
#include <QtDebug>
#include <QSettings>
#include <QClipboard>
#include <QMimeData>
#include <QTemporaryFile>
#include <QFont>
#include <QFontDialog>


#include "GeometryConstraint.h"
#include "Preferences.h"
#include "KeywordSection.h"
#include "OptSection.h"
#include "ExternalChargesSection.h"
#include "InputDialog.h"
#include "FileDisplay.h"
#include "Option.h"
#include "QtNode.h"
#include "Qui.h"
#include "Job.h"
#include "OptionDatabase.h"
#include "Process.h"

#ifdef AVOGADRO
#include <avogadro/glwidget.h>
#endif



namespace Qui {

//! Automatic Slots, these are set by the m_ui.setupUi(this) call.
void InputDialog::on_advancedOptionsTree_itemClicked(QTreeWidgetItem* item, int) {
   if (item)  {
      QString label("Advanced");
      label += item->text(0).replace(" ","");
      QWidget* widget = m_ui.advancedOptionsStack->findChild<QWidget*>(label);

      if (widget) {
         m_ui.advancedOptionsStack->setCurrentWidget(widget);
      }else {
         qDebug() << "InputDialog::on_advancedOptionsTree_itemClicked:\n"
                  << "  Widget not found: " << label;
      }
   }
}


void InputDialog::on_job_type_currentIndexChanged(QString const& text) {
   QString label(text);
   if (label == "Transition State") label = "Geometry";
   label =  "Options" + label.replace(" ","");

   QWidget* widget = m_ui.stackedOptions->findChild<QWidget*>(label);
   if (widget) {
      m_ui.stackedOptions->setCurrentWidget(widget);
   }else {
     qDebug() << "InputDialog::on_job_type_currentIndexChanged:\n"
              << "  Widget not found: " << label;
   }
}


void InputDialog::on_stackedOptions_currentChanged(int index) {
    for (int i = 0; i < m_ui.stackedOptions->count(); ++i) {
        m_ui.stackedOptions->widget(i)->setEnabled(false);
    }
    m_ui.stackedOptions->widget(index)->setEnabled(true);
    updatePreviewText();
}


void InputDialog::on_qui_title_textChanged() {
   QString text(m_ui.qui_title->text());
   if (m_currentJob) {
      m_currentJob->addSection("comment", text);
      m_currentJob->printSection("comment", true);
   }

   if (text.size() > 10) {
      text.truncate(10); 
      text += "...";
   }

   int i(m_ui.jobList->currentIndex());
   m_ui.jobList->setItemText(i,text);

}


void InputDialog::on_qui_charge_valueChanged(int value) {
   if (m_currentJob) m_currentJob->setCharge(value);
}


void InputDialog::on_qui_multiplicity_valueChanged(int value) {
   if (m_currentJob) m_currentJob->setMultiplicity(value);
}

 


void InputDialog::on_jobList_currentIndexChanged(int index) {
   if (0 <= index && index < int(m_jobs.size())) {
      if (m_currentJob) {
         capturePreviewText();
         finalizeJob();
      }
      m_currentJob = 0;
      resetControls();
      m_currentJob = m_jobs[index];
      setControls(m_currentJob);
      if (m_currentJob->getCoordinates().contains("read",Qt::CaseInsensitive)) {
         m_ui.qui_multiplicity->setEnabled(false);
         m_ui.qui_charge->setEnabled(false);
      }else {
         m_ui.qui_multiplicity->setEnabled(true);
         m_ui.qui_charge->setEnabled(true);
      }
      updatePreviewText();
   }
}


void InputDialog::on_deleteJobButton_clicked(bool) {
   QString msg("Are you sure you want to delete ");
   msg += m_ui.jobList->currentText() + " section?";
   if (QMessageBox::question(this, "Delete section?",msg,
       QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel)  {
       return;
   }

   int i(m_ui.jobList->currentIndex());  //This is the Job we are deleting
   m_taint = false;  //This may not be right if the user has edited *other* jobs

   m_currentJob = 0;
   delete m_jobs[i];
   m_jobs.erase(m_jobs.begin()+i);
   m_ui.jobList->removeItem(i);

   if (m_jobs.size() == 0) {
      on_addJobButton_clicked(true);
   }else {
      int j = (i == 0) ? 0 : i-1;
      m_ui.jobList->setCurrentIndex(j);
   }

   Q_ASSERT(m_ui.jobList->count() == int(m_jobs.size()));
}



void InputDialog::editPreferences() {
   Preferences::Browser* prefs = new Preferences::Browser(this);
   prefs->show();
}


void InputDialog::readCharges() {
   if (m_currentJob) {
      QFileInfo lastFile(Preferences::LastFileAccessed());
      QString file(QFileDialog::getOpenFileName(0, QString("Select File"),
         lastFile.absolutePath() ));
   
      if (!file.isEmpty()) {
         QString name("external_charges");
         KeywordSection* ks = m_currentJob->getSection(name);
         ExternalChargesSection* charges;
         charges = dynamic_cast<ExternalChargesSection*>(ks);

         if (!charges) {
            charges = new ExternalChargesSection();
            m_currentJob->addSection(charges);
         }

         m_currentJob->printSection(name, true);
         m_reg.get("QUI_SECTION_EXTERNAL_CHARGES").setValue("1");
         m_reg.get("QUI_SECTION_EXTERNAL_CHARGES").setValue("2");

         QFile f(file);
         charges->read(ReadFile(f));
         updatePreviewText();
      }
   }
}



void InputDialog::editConstraints() {
   if (m_currentJob) {
      KeywordSection* ks = m_currentJob->getSection("opt");
      OptSection* opt;
      opt = dynamic_cast<OptSection*>(ks);

      if (!opt) {
         opt = new OptSection();
         m_currentJob->addSection(opt);
      } 
 
      int nAtoms = m_currentJob->getNumberOfAtoms();
      if (nAtoms < 2) {
         QString msg("Too few atoms to allow constraints.");
         QMessageBox::warning(0, "Don't Bother", msg);
      }else {
         GeometryConstraint::Dialog dialog(this, opt, nAtoms);
         dialog.exec();
         updatePreviewText();
      }
   }
}



void InputDialog::build() {
   if (m_taint) updatePreviewText();
   if (m_currentJob && m_currentJob->getNumberOfAtoms() > 0) {
      // create a temporary xyz file to pass to Avogadro.
      QByteArray xyz;
      xyz.append(QString::number(m_currentJob->getNumberOfAtoms()));
      xyz.append("\n\n");
      xyz.append(m_currentJob->getCoordinates());

      qDebug() << "Contents of file to Avogadro:";
      qDebug() << xyz;
      qDebug() << QDir::tempPath();

      QTemporaryFile* tmp = 
         new QTemporaryFile(QDir::tempPath() + "/qui_build.XXXXXX.xyz", this);
      tmp->open();
      tmp->write(xyz);
      QString fileName(tmp->fileName());
      //qDebug() << "Creating temporary file" << fileName;
      tmp->close();
      launchAvogadro(fileName);
   }else {
      launchAvogadro();
   }
}


void InputDialog::launchAvogadro(QString const& fileName) {
   QString avogadroPath(Preferences::AvogadroPath());
   QFileInfo exeFile;
   bool okay(false);

   if (!avogadroPath.isEmpty()) {
      exeFile.setFile(avogadroPath);
      if (exeFile.exists()) okay = true;
   }

   if (!okay) {
      QString msg("The Avogadro environment has not been set up.\n\n");
      msg += "Please ensure Avogadro has been correctly installed"
             " and that the Avogadro preferences have been set.";
      QMessageBox* box = new QMessageBox(QMessageBox::Information,
         tr("Avogadro Not Found"), msg, QMessageBox::Ok, this);

      QAbstractButton *editButton =
            box->addButton(tr("Edit Preferences"), QMessageBox::ActionRole);

      box->exec();

      if (box->clickedButton() == editButton) {
         editPreferences();
      }
      return;
   } 

   QFile file(fileName);

   //if (!m_avogadro) {
      m_avogadro =  new QProcess(this);

      connect(m_avogadro, SIGNAL(started()), 
         this, SLOT(avogadroStarted()) );
      connect(m_avogadro, SIGNAL(finished(int, QProcess::ExitStatus)), 
         this, SLOT(avogadroFinished(int, QProcess::ExitStatus)) );


      QStringList args;
      QString prog;

#if defined(Q_WS_MAC)
	  // Using 'open' on OS X returns immediately, so the m_avogadro is deleted
	  // (via avogadroFinished()) when Avogadro is still open.  This is not a
	  // problem as if we 'open' an app which is already open, then is simply
	  // brings that window to the front, which is what we are after.
      prog = "open";
      args << "-a";
      args << avogadroPath;
#elif defined(Q_WS_WIN)
	  // XP doesn't like paths in the exectuable file, instead we must set the
	  // working directory to be that in which the exe is.
      QDir::setCurrent(exeFile.absolutePath());
      prog = exeFile.fileName();
#else
      prog = avogadroPath;
#endif

     if (file.exists()) args << fileName;
     qDebug() << "Starting Avogadro prog:" << prog;
     qDebug() << "        With arguments:" << args;
     m_avogadro->start(prog, args);
   //}
}



void InputDialog::avogadroStarted() {
   //m_ui.buildButton->disable(false);
   qDebug() << "avogadroStarted Called";
}
   

void InputDialog::avogadroFinished(int, QProcess::ExitStatus) {
   //m_ui.buildButton->enable(true);
   qDebug() << "avogadroFinished Called";
   delete m_avogadro;
   m_avogadro = 0;
}






/***********************************************************************
 *   
 *  Process control: submitting jobs, reading output etc
 *  
 ***********************************************************************/


//! Saves the input file to the file specified by m_fileIn, prompting the user
//! is this is empty or if we want to over-ride the exising file name (save as).
bool InputDialog::saveFile(bool prompt) {

   if (!hasValidMultiplicity()) {
      QString msg("The specified charge/multiplcity combination is "
         "invalid for this system.  Please correct before saving.");
      QMessageBox::warning(0, "Invalid Charge/Multiplicity", msg);
      return false;
   }

   QFileInfo tmp(m_fileIn);
   bool saved(false);

   if (tmp.fileName().isEmpty() || prompt) {
      tmp.setFile(QFileDialog::getSaveFileName(this, tr("Save File"),
         Preferences::LastFileAccessed()));
   }

   if (tmp.fileName().isEmpty()) return false;

   Preferences::LastFileAccessed(tmp.filePath());

   QFile file(tmp.filePath());
   if (file.exists() && tmp.isWritable()) file.remove();

   if (file.open(QIODevice::WriteOnly | QIODevice::Text )) {
      qDebug() << "Writing to file" << tmp.filePath();
      QByteArray buffer;
      bool preview(false);
      buffer.append(generateInputDeck(preview));
      file.write(buffer);
      file.close();
      m_fileIn = tmp;
      saved = true;
   }

   if (saved) {
      setWindowTitle("QChem Input File Editor - " + file.fileName());
   }else {
      QString msg("Could not write to file '");
      msg += tmp.fileName() + "'\nInput file not saved\n";
      QMessageBox::warning(0, "File Not Saved", msg);
   }

   return saved;
}



void InputDialog::submitJob() {
   bool okay = false;
   QString runQChem(Preferences::QChemRunScript());

   // Check the runqchem script is there
   if (!runQChem.isEmpty()) {
      QFile exe(runQChem);
      if (exe.exists()) okay = true;
   }

   // If we have a problem, issue a warning and allow the user to edit the
   // preferences.
   if (!okay) {
      QString msg("The QChem run script has not been set.\n\n");
      msg += "Please ensure Q-Chem has been correctly installed"
             " and that the Q-Chem preferences have been set.";
      QMessageBox* box = new QMessageBox(QMessageBox::Information,
         tr("Q-Chem Not Found"), msg, QMessageBox::Ok, this);
      QAbstractButton *editButton =
         box->addButton(tr("Edit Preferences"), QMessageBox::ActionRole);
      box->exec();
      if (box->clickedButton() == editButton) {
         editPreferences();
      }
      return;
   } 

   // Ensure we have saved the input file and can find it.
   if (!saveFile()) return; 
      
   if (!m_fileIn.exists()) {
      QString msg("Can not find input file: '");
      msg += m_fileIn.filePath() + "'";
      QMessageBox::warning(0, "File Not Found", msg);
      return;
   }else if (!QDir::setCurrent(m_fileIn.path())) {
      QString msg("Can not change to working directory: ");
      msg += m_fileIn.path();
      QMessageBox::warning(0, "Directory Not Found", msg);
      return;
   }

   // Determine the output name based on the input name
   QString output = m_fileIn.completeBaseName() + ".out";
   m_fileOut.setFile(m_fileIn.absoluteDir(), output);

   if (m_fileOut.exists()) {
      QString msg("Output file ");
      msg += m_fileOut.fileName();
      msg += " exists, overwite?";

      QMessageBox* box = new QMessageBox(QMessageBox::Question,
         tr("Overwrite File?"), msg, QMessageBox::Ok, this);

      QAbstractButton *saveAsButton =
         box->addButton(tr("Save As"), QMessageBox::ActionRole);
      QAbstractButton *cancelButton =
         box->addButton(QMessageBox::Cancel);
      box->exec();

      if (box->clickedButton() == saveAsButton) {
         QString tmp =
         QFileDialog::getSaveFileName(this, tr("Save File"),
            m_fileOut.filePath()) ;
         if (tmp.isEmpty()) {
             return;
         }else {
            m_fileOut. setFile(tmp);
            output = m_fileOut.fileName();
         }
      }else if (box->clickedButton() == cancelButton) {
          return;
      }

   }

   // Create the process for running QChem
   Process::QChem* process = new Process::QChem(this, 
      m_fileIn.filePath(), m_fileOut.filePath());

   connect(process, SIGNAL(started()), this, SLOT(jobStarted()) );
   connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), 
      this, SLOT(jobFinished(int, QProcess::ExitStatus)) );


   QStringList args;
   args << m_fileIn.fileName();
   qDebug() << "Executing shell command" << runQChem << "with args:" << args
            << "in directory" << m_fileIn.path();
   
   m_processQueue->submit(process);
   watchProcess(process);

   m_currentProcess = process;
}


void InputDialog::watchProcess(Process::Monitored* process) {
   m_processList.push_back(process);
   if (m_processMonitor) {
      m_processMonitor->addProcess(process);
   }
}



void InputDialog::jobStarted() {
   qDebug() << "Job Started";
}


void InputDialog::jobFinished(int, QProcess::ExitStatus exitStatus) {
   qDebug() << "Job Finished";

   Process::Monitored* process = qobject_cast<Process::Monitored*>(sender());
   if (!process) return;

   QString output(process->outputFile());

   // Rename the FChk file, if it exits.
   QString msg("Job file '");
   msg += output;
   if (exitStatus == QProcess::NormalExit) {
      msg += "' has finished";
   }else {
      msg += "' has failed.  See output file for details.";
   }

   QMessageBox* box = new QMessageBox(QMessageBox::Information,
      tr("Job Finished"), msg, QMessageBox::Ok, this);

   QAbstractButton *displayButton;
   displayButton =
      box->addButton(tr("Display Output"), QMessageBox::ActionRole);

   box->exec();

   if (box->clickedButton() == displayButton) {
      QFileInfo file(output);
      if (file.exists()) {
         FileDisplay* fileDisplay = new FileDisplay(this, output);
         fileDisplay->show();
      }else {
         QString mesg("Output file ");
         mesg += output + " was not found";
         QMessageBox::warning(0, "No Output Found", msg);
      }
   }
}





// Despite the name of this function, we actually pass the QChem output file to
// Avogadro and it assumes the existence of the Fchk file.
void InputDialog::displayCheckpointFile() {
   Option file;
   bool needFchk(false);

   if (m_db.get("QUI_AVOGADRO_VISUALIZE_FILE", file)) {
      if (file.getDefaultIndex() == 0) needFchk = true;
   }

   if (needFchk && !m_fileFchk.exists()) {
      QString msg("Checkpoint file ");
      msg += m_fileFchk.filePath();
      msg += " was not found.  Please ensure the GUI rem option is set "
             "to 2 when submitting jobs for visualization.";
      QMessageBox::warning(0, "No Checkpoint File Found", msg);
      return;
   }

   if (!m_fileOut.exists()) {
      QString msg("Output file ");
      msg += m_fileOut.filePath();
      msg += " was not found.  Please ensure the job has been submitted "
             "and completed before visualization.";
      QMessageBox::warning(0, "No Output File Found", msg);
      return;
   }

   if (needFchk) {
      launchAvogadro(m_fileFchk.filePath() );
   }else {
      launchAvogadro(m_fileOut.filePath() );
   }

}






//! This routine leaves the m_currentJob pointer uninitialised, and should be
//! used with caution.  Currently it is only used when reading in an input file
//! in the InputDialog::on_readButton_clicked routine and the m_currentJob
//! pointer is set afterwards.
bool InputDialog::deleteAllJobs(bool const prompt) {
   if (prompt) {
      QString msg("Are you sure you want to delete all generated input?");
      if (QMessageBox::question(this, "Delete input?",msg,
          QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel)  {
          return false;
      }
   }

   m_currentJob = 0;

   std::vector<Job*>::iterator iter;
   for (iter = m_jobs.begin(); iter != m_jobs.end(); ++iter) {
       delete *iter;
   }
   
   m_jobs.clear();
   m_ui.jobList->clear();
   m_ui.previewText->clear();
   m_taint = false;
   return true;
}




/***********************************************************************
 *   
 *  Stacked Widgets controlled by radio buttons
 *  
 *  The solvent models and large molecule options panels have stacked widgets
 *  which control what options are visible.  These need special treatment as
 *  appropriate slots do not exist to connect directly.
 *  
 ***********************************************************************/

// The following is used to change the displayed page on a QStackedWidget based
// on a radio box group. 
void InputDialog::toggleStack(QStackedWidget* stack, bool on, QString model) {
   QWidget* widget;
   if (on) {
      widget = stack->findChild<QWidget*>(model);
      Q_ASSERT(widget);
      widget->setEnabled(true);
      stack->setCurrentWidget(widget);
   }else {
      widget = stack->findChild<QWidget*>(model);
      if (widget) widget->setEnabled(false);
   }
}

void InputDialog::on_qui_cfmm_toggled(bool on) {
   toggleStack(m_ui.largeMoleculesStack, on, "LargeMoleculesCFMM");
}

void InputDialog::on_use_case_toggled(bool on) {
   toggleStack(m_ui.largeMoleculesStack, on, "LargeMoleculesCASE");
}

void InputDialog::on_ftc_toggled(bool on) {
   toggleStack(m_ui.largeMoleculesStack, on, "LargeMoleculesFTC");
}

void InputDialog::on_qui_solvent_onsager_toggled(bool on) {
   toggleStack(m_ui.solventStack, on, "SolventOnsager");
}

void InputDialog::on_qui_solvent_none_toggled(bool on) {
   toggleStack(m_ui.solventStack, on, "SolventNone");
}

void InputDialog::on_chemsol_toggled(bool on) {
   toggleStack(m_ui.solventStack, on, "SolventChemSol");
}

void InputDialog::on_smx_solvation_toggled(bool on) {
   toggleStack(m_ui.solventStack, on, "SolventSM8");
}

void InputDialog::on_svp_toggled(bool on) {
   toggleStack(m_ui.solventStack, on, "SolventSVP2");
   toggleStack(m_ui.solventStack, on, "SolventSVP");
}




/***********************************************************************
 *   
 *  Manual Slots used for changing controls
 *  
 ***********************************************************************/

// The following are required as wrappers for the SetControl() functions
// as the signature needs to match the signals.

void InputDialog::changeComboBox(QString const& name, QString const& value) {
   QComboBox* combo = findChild<QComboBox*>(name.toLower());
   combo ? SetControl(combo, value) : widgetError(name);
}


void InputDialog::changeDoubleSpinBox(QString const& name, QString const& value) {
   QDoubleSpinBox* spin = findChild<QDoubleSpinBox*>(name.toLower());
   spin ? SetControl(spin, value) : widgetError(name);
}


void InputDialog::changeSpinBox(QString const& name, QString const& value) {
   QSpinBox* spin = findChild<QSpinBox*>(name.toLower());
   spin ? SetControl(spin, value) : widgetError(name);
}


void InputDialog::changeCheckBox(QString const& name, QString const& value) {
   QCheckBox* check = findChild<QCheckBox*>(name.toLower());
   check ? SetControl(check, value) : widgetError(name);
}


void InputDialog::changeRadioButton(QString const& name, QString const& value) {
   QRadioButton* radio = findChild<QRadioButton*>(name.toLower());
   radio ? SetControl(radio, value) : widgetError(name);
}


void InputDialog::changeLineEdit(QString const& name, QString const& value) {
   QLineEdit* edit = findChild<QLineEdit*>(name.toLower());
   edit ? SetControl(edit, value) : widgetError(name);
}


void InputDialog::widgetError(QString const& name) {
   qDebug() << "Error in QChem::InputDialog:\n"
            << "Could not find widget" << name;
}



// These widgetChanged functions are used to connect widgets to QtNodes
void InputDialog::widgetChanged(QString const& value) {
   QObject* orig = qobject_cast<QObject*>(sender());
   widgetChanged(orig, value);
}


void InputDialog::widgetChanged(int const& value) {
   QObject* orig = qobject_cast<QObject*>(sender());
   QString val(QString::number(value));
   widgetChanged(orig, val);
}

void InputDialog::widgetChanged(bool const& value) {
   QObject* orig = qobject_cast<QObject*>(sender());
   QString val = value ? QString::number(Qt::Checked) 
                       : QString::number(Qt::Unchecked);
   widgetChanged(orig, val);
}


void InputDialog::widgetChanged(QObject* orig, QString const& value) {
   QString name(orig->objectName().toUpper());
   if (m_reg.exists(name)) m_reg.get(name).setValue(value);
   if (m_currentJob) {
      m_currentJob->setOption(name, value);
      updatePreviewText();
   }
}


} // end namespace Qui
