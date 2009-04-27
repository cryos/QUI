/*!
 *  \file InputDialog.C
 *
 *  \brief This is the main class for the QChem input file generator.  If you
 *  think this class is bloated, it is because it is.  Further member functions
 *  can be found in InputDialogLogic.C and InputDialogSlots.C
 *
 *  \author Andrew Gilbert
 *  \date   August 2008
 */

#include <QList>
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>
#include <QTextStream>
#include <QKeySequence>
#include <QtDebug>
#include <QResizeEvent>

#include "InputDialog.h"
#include "OptionRegister.h"
#include "OptionDatabase.h"
#include "Conditions.h"
#include "Option.h"
#include "QtNode.h"
#include "Qui.h"
#include "Job.h"
#include "RemSection.h"
#include "LJParametersSection.h"
#include "Preferences.h"
#include "Process.h"

namespace Qui {


InputDialog::InputDialog(QWidget* parent) :
#ifdef AVOGADRO
   QDialog(parent),
#else
   QMainWindow(parent),
#endif
   m_molecule(0),
   m_fileIn(""),
   m_fileOut(""),
   m_db(OptionDatabase::instance()),
   m_reg(OptionRegister::instance()),
   m_taint(false),
   m_currentJob(0),
   m_currentProcess(0),
   m_avogadro(0),
   m_processMonitor(0),
   m_processQueue(0) {

   m_ui.setupUi(this);

#ifdef AVOGADRO
   const QIcon icon0 = QIcon(QString::fromUtf8(":/icons/edit_remove.png"));
   m_ui.deleteJobButton->setIcon(icon0);
   const QIcon icon1 = QIcon(QString::fromUtf8(":/icons/edit_add.png"));
   m_ui.addJobButton->setIcon(icon1);
#else
   m_ui.qui_coordinates->hide();
   m_ui.label_coordinates->hide();
   initializeMenus();
#endif

   InitializeQChemLogic();
   initializeQuiLogic();
   initializeControls();

   resize(Preferences::MainWindowSize());

   QFileInfo file(Preferences::LastFileAccessed());
   file.setFile(file.dir(),"untitled.inp");
   Preferences::LastFileAccessed(file.filePath());

   setWindowTitle("QChem Input File Editor - " + file.fileName());


   m_ui.previewText->setCurrentFont(Preferences::PreviewFont());

   on_addJobButton_clicked(true);
   m_processQueue = new Process::Queue(this, 1);
}


InputDialog::~InputDialog() {
   std::vector<Job*>::iterator iter1;
   for (iter1 = m_jobs.begin(); iter1 != m_jobs.end(); ++iter1) {
       delete *iter1;
   }
   std::vector<Action*>::iterator iter2;
   for (iter2 = m_resetActions.begin(); iter2 != m_resetActions.end(); ++iter2) {
       delete *iter2;
   }
   std::map<QString,Update*>::iterator iter3;
   for (iter3 = m_setUpdates.begin(); iter3 != m_setUpdates.end(); ++iter3) {
       delete iter3->second;
   }
}


void InputDialog::resizeEvent(QResizeEvent* event)  {
   Preferences::MainWindowSize(event->size());
}



// Accessing and changing the different control widgets (QComboBox, QSpinBox
// etc.) requires different member functions and so the controls cannot be
// treated polymorphically.  The following routine initializes the controls and
// (in the initializeControl subroutines) also binds Actions to be used later to
// reset and update the controls.  This allows all the controls to be treated in
// a similar way.  What this means is that this function should be the only one
// that has the implementation case switch logic and also the only one that needs
// to perform dynamic casts on the controls.
void InputDialog::initializeControls() {

   QList<QWidget*> controls(findChildren<QWidget*>());
   QWidget* control;
   QString name, value;
   Option opt;

   for (int i = 0; i < controls.count(); ++i) {
       control = controls[i];
       name = control->objectName().toUpper();

       if (m_db.get(name, opt)) {

          switch (opt.getImplementation()) {

             case Option::Impl_None:
             break;

             case Option::Impl_Combo: {
                QComboBox* combo = qobject_cast<QComboBox*>(control);
                Q_ASSERT(combo);
                initializeControl(opt, combo);
             }
             break;

             case Option::Impl_Check: {
                QCheckBox* check = qobject_cast<QCheckBox*>(control);
                Q_ASSERT(check);
                initializeControl(opt, check);
             }
             break;

             case Option::Impl_Text: {
                QLineEdit* edit = qobject_cast<QLineEdit*>(control);
                Q_ASSERT(edit);
                initializeControl(opt, edit);
             }
             break;

             case Option::Impl_Spin: {
                QSpinBox* spin = qobject_cast<QSpinBox*>(control);
                Q_ASSERT(spin);
                initializeControl(opt, spin);
             }
             break;

             case Option::Impl_DSpin: {
                QDoubleSpinBox* dspin = qobject_cast<QDoubleSpinBox*>(control);
                Q_ASSERT(dspin);
                initializeControl(opt, dspin);
             }
             break;

             case Option::Impl_Radio: {
                QRadioButton* radio = qobject_cast<QRadioButton*>(control);
                Q_ASSERT(radio);
                initializeControl(opt, radio);
             }
             break;

             default: {
                qDebug() << "Error in QChem::InputDialog::initializeControl():\n"
                         << "  Could not initialize control " << name << "\n"
                         << "  Widget does not match database.  Impl:"
                         << opt.getImplementation();
             }
             break;
          }


          if (m_reg.exists(name)) m_reg.get(name).setValue(opt.getDefaultValue());

       }

    }
}



//! A simple loop for reseting the controls to their default values.  This
//! routine takes advantage of the reset Actions that are set up in the
//! initializeControl functions.
void InputDialog::resetControls() {
   std::vector<Action*>::iterator iter;
   for (iter = m_resetActions.begin(); iter != m_resetActions.end(); ++iter) {
       (*iter)->operator()();
   }
}


//! A simple loop for synchronizing controls with the list of string values
//! contained in a Job object.  This routine takes advantage of the Update
//! functions that are bind'ed in the initializeControl functions.  These Updates
//! allow a QControl widget to have its value changed based on a string,
//! irrespective of its type.
void InputDialog::setControls(Job* job) {
   StringMap::iterator iter;
   StringMap opts(job->getOptions());
   for (iter = opts.begin(); iter != opts.end(); ++iter) {
       if (m_setUpdates.count(iter->first)) {
          m_setUpdates[iter->first]->operator()(iter->second);
       }else {
          qDebug() << "Warning: Update not initialised for"
                   << iter->first << "in InputDialog::setControls";
          qDebug() << " did you forget about it?";
       }
   }
}



//! The (overloaded) initializeControl routine is responsible for the following
//! tasks:
//!  - Ensuring the control displays the appropriate options based on the
//!    contents of the OptionDatabase.
//!  - Adding the ToolTip documentation found in the database.
//!  - Adding connections (signals & slots) between the Nodes in the
//!    OptionRegister and the control.  This allows for synchronization of the
//!    logic in InitializeQChemLogic and initializeQuiLogic.
//!  - Binding an Action to enable the control to be reset to the default value.
//!  - Binding an Update to enable the control to be reset to a string value.
void InputDialog::initializeControl(Option const& opt, QComboBox* combo) {

   QString name = opt.getName();
   QStringList opts = opt.getOptions();
   QStringList split;

   // This allows for ad hoc text replacements.  This is useful so that more
   // informative text can be presented to the user which is then obfiscated
   // before being passed to QChem.  The replacements should be set in the
   // option database and have the form text//replacement.
   for (int i = 0; i < opts.size(); ++i) {
       split = opts[i].split("//");

       if (split.size() == 1) {
          opts[i] = split[0];
       }else if (split.size() == 2) {
          opts[i] = split[0];
          QString key(name.toUpper() + "::" + split[0]);
          //Job::addAdHoc(key,split[1]);
          RemSection::addAdHoc(name, split[0], split[1]);
       }else {
          qDebug() << "InputDialog::initialiseComboBox:\n"
                   << " replacement for option" << name << "is invalid:" << opts[i];
       }
   }

   combo->clear();
   combo->addItems(opts);

#if QT_VERSION >= 0x040400
   // This just allows us to add some spacers to the lists
   bool keepLooking(true);
   while (keepLooking) {
      int i = combo->findText("---", Qt::MatchStartsWith);
      if (i > 0) {
         combo->removeItem(i);
         combo->insertSeparator(i);
      }else {
         keepLooking = false;
      }
   }
#endif

   connectControl(opt, combo);
   combo->setToolTip(opt.getDescription());

   Action* action = new Action(
      boost::bind(&QComboBox::setCurrentIndex, combo, opt.getDefaultIndex()) );
   m_resetActions.push_back(action);

   Update* update = new Update(
      boost::bind(
         static_cast<void(*)(QComboBox*, QString const&)>(SetControl), combo, _1));
   m_setUpdates[name] = update;
}


void InputDialog::initializeControl(Option const& opt, QCheckBox* check) {
   connectControl(opt, check);
   check->setToolTip(opt.getDescription());

   Action* action = new Action(
      boost::bind(&QCheckBox::setChecked, check, opt.getDefaultIndex()) );
   m_resetActions.push_back(action);

   Update* update = new Update(
      boost::bind(
         static_cast<void(*)(QCheckBox*, QString const&)>(SetControl), check, _1));
   QString name = opt.getName();
   m_setUpdates[name] = update;
}


void InputDialog::initializeControl(Option const& opt, QSpinBox* spin) {
   connectControl(opt, spin);
   spin->setToolTip(opt.getDescription());
   spin->setRange(opt.intMin(), opt.intMax());
   spin->setSingleStep(opt.intStep());

   Action* action = new Action(
      boost::bind(&QSpinBox::setValue, spin, opt.intDefault()) );
   m_resetActions.push_back(action);

   Update* update = new Update(
      boost::bind(
         static_cast<void(*)(QSpinBox*, QString const&)>(SetControl), spin, _1));
   QString name = opt.getName();
   m_setUpdates[name] = update;
}


void InputDialog::initializeControl(Option const& opt, QDoubleSpinBox* dspin) {
   connectControl(opt, dspin);
   dspin->setToolTip(opt.getDescription());
   dspin->setRange(opt.doubleMin(), opt.doubleMax());
   dspin->setSingleStep(opt.doubleStep());

   Action* action = new Action(
      boost::bind(&QDoubleSpinBox::setValue, dspin, opt.doubleDefault()) );
   m_resetActions.push_back(action);

   Update* update = new Update(
      boost::bind(
         static_cast<void(*)(QDoubleSpinBox*, QString const&)>(SetControl), dspin, _1));
   QString name = opt.getName();
   m_setUpdates[name] = update;
}


void InputDialog::initializeControl(Option const& opt, QRadioButton* radio) {
   connectControl(opt, radio);
   radio->setToolTip(opt.getDescription());

   Action* action = new Action(
      boost::bind(&QRadioButton::setChecked, radio, opt.getDefaultIndex()) );
   m_resetActions.push_back(action);

   Update* update = new Update(
      boost::bind(
         static_cast<void(*)(QRadioButton*, QString const&)>(SetControl), radio, _1));
   QString name = opt.getName();
   m_setUpdates[name] = update;
}


void InputDialog::initializeControl(Option const& opt, QLineEdit* edit) {
   connectControl(opt, edit);
   edit->setToolTip(opt.getDescription());

   Action* action = new Action(
      boost::bind(&QLineEdit::setText, edit, opt.getOptionString()) );
   m_resetActions.push_back(action);

   Update* update = new Update(
      boost::bind(
         static_cast<void(*)(QLineEdit*, QString const&)>(SetControl), edit, _1));
   QString name = opt.getName();
   m_setUpdates[name] = update;
}



//! The connectControl routines make the necessary signal-slot connections
//! between the control, the current Job and Nodes in the OptionRegister (for
//! program logic purposes).
void InputDialog::connectControl(Option const& opt, QComboBox* combo) {
   QString name = opt.getName();

   connect(combo, SIGNAL(currentIndexChanged(const QString&)),
      this, SLOT(widgetChanged(const QString&)));

   if (combo->isEditable()) {
      connect(combo, SIGNAL(editTextChanged(const QString&)),
         this, SLOT(widgetChanged(const QString&)));
   }

   if (m_reg.exists(name)) {
      connect(&(m_reg.get(name)),
         SIGNAL(valueChanged(QString const&, QString const&)),
         this, SLOT(changeComboBox(QString const&, QString const&)) );
   }
}


void InputDialog::connectControl(Option const& opt, QRadioButton* radio) {
   QString name = opt.getName();

   connect(radio, SIGNAL(toggled(bool)),
      this, SLOT(widgetChanged(bool)));

   if (m_reg.exists(name)) {
      connect(&(m_reg.get(name)),
         SIGNAL(valueChanged(QString const&, QString const&)),
         this, SLOT(changeRadioButton(QString const&, QString const&)) );
   }
}


void InputDialog::connectControl(Option const& opt, QCheckBox* check) {
   QString name = opt.getName();

   connect(check, SIGNAL(stateChanged(int)),
      this, SLOT(widgetChanged(int)));

   if (m_reg.exists(name)) {
      connect(&(m_reg.get(name)),
         SIGNAL(valueChanged(QString const&, QString const&)),
         this, SLOT(changeCheckBox(QString const&, QString const&)) );
   }
}


void InputDialog::connectControl(Option const& opt, QDoubleSpinBox* dspin) {
   QString name = opt.getName();

   connect(dspin, SIGNAL(valueChanged(const QString&)),
      this, SLOT(widgetChanged(const QString&)));

   if (m_reg.exists(name)) {
      connect(&(m_reg.get(name)),
         SIGNAL(valueChanged(QString const&, QString const&)),
         this, SLOT(changeDoubleSpinBox(QString const&, QString const&)) );
   }
}


void InputDialog::connectControl(Option const& opt, QSpinBox* spin) {
   QString name = opt.getName();

   connect(spin, SIGNAL(valueChanged(int)),
      this, SLOT(widgetChanged(int)));

   if (m_reg.exists(name)) {
      connect(&(m_reg.get(name)),
         SIGNAL(valueChanged(QString const&, QString const&)),
         this, SLOT(changeSpinBox(QString const&, QString const&)) );
   }
}


void InputDialog::connectControl(Option const& opt, QLineEdit* edit) {
   QString name = opt.getName();

   connect(edit, SIGNAL(textChanged(const QString&)),
      this, SLOT(widgetChanged(const QString&)));

   if (m_reg.exists(name)) {
      connect(&(m_reg.get(name)),
         SIGNAL(valueChanged(QString const&, QString const&)),
         this, SLOT(changeLineEdit(QString const&, QString const&)) );
   }
}


/***********************************************************************
 *
 *  Private Member functions
 *
 ***********************************************************************/

//! The Job options are synchronised using the widgetChanged slot, but we still
//! need to determine if the options should be printed as part of the job.  This
//! is based on whether or not the associated control is enabled or not.
void InputDialog::finalizeJob() {
   if (!m_currentJob) return;

   QWidget* w;
   QString name;
   StringMap::const_iterator iter;

   StringMap s = m_currentJob->getOptions();
   for (iter = s.begin(); iter != s.end(); ++iter) {
       name  = iter->first;
       w = findChild<QWidget*>(name.toLower());
       // If there is no wiget of this name, then we are probably dealing with
       // something the user wrote into the preview box, so we just leave
       // things alone.
       if (w) m_currentJob->printOption(name, w->isEnabled());
   }
}


// ****** THE FOLLOWING needs further error checking

//! If the text has been altered by the user (as indicate by m_taint),
//! capturePreviewText takes the text from the qTextEdit panel and breaks it up
//! into blocks which are dealt with by ParseQChemFileContents
void InputDialog::capturePreviewText() {
   if (m_taint) {
      m_taint = false;
      QString text(m_ui.previewText->toPlainText());

      int currentJobIndex(0), i(0);
      std::vector<Job*>::iterator iter;
      for (iter = m_jobs.begin(); iter != m_jobs.end(); ++iter, ++i) {
          if (m_currentJob == *iter) currentJobIndex = i;
      }

      bool prompt(false);
      deleteAllJobs(prompt);
      std::vector<Job*> jobs = ParseQChemFileContents(text);

      for (iter = jobs.begin(); iter != jobs.end(); ++iter) {
          addJobToList(*iter);
      }

      m_ui.jobList->setCurrentIndex(currentJobIndex);
   }
}


void InputDialog::updatePreviewText() {
   bool preview(true);
   QStringList jobStrings(generateInputDeckJobs(preview));

   if ((unsigned int)jobStrings.size() != m_jobs.size()) {
      qDebug() << "ERROR: Job numbers do not match";
   }

   m_ui.previewText->clear();
   // This shouldn't really be required, but sometimes when the comemnt is
   // empty the default font is activated.
   m_ui.previewText->setCurrentFont(Preferences::PreviewFont());

   QString buffer;

   int pos(0), nJobs(m_jobs.size());
   m_ui.previewText->setTextColor("darkgrey");
   QString jobSeparator("\n@@@\n");

   for (int i = 0; i < nJobs ; ++i) {
       if (m_jobs[i] == m_currentJob) {
          pos = buffer.size();
          m_ui.previewText->setTextColor("black");
       }
       buffer += jobStrings.value(i);
       m_ui.previewText->append(jobStrings.value(i));
       m_ui.previewText->setTextColor("darkgrey");
       if (i != nJobs-1) {
          buffer += jobSeparator;
          m_ui.previewText->append(jobSeparator);
       }
   }

   if (m_rememberMe != buffer) m_rememberMe = buffer;

   // This is a bit micky mouse, but I don't know of a better way of doing it.
   // ensureCursorVisible only seeks a minimal amount, so to ensure as much as
   // possible of the required section is showing, we seek to the end of the
   // text before seeking to the start of the section.
   QTextCursor cursor(m_ui.previewText->textCursor());
   cursor.setPosition(buffer.size());
   m_ui.previewText->setTextCursor(cursor);
   m_ui.previewText->ensureCursorVisible();
   cursor.setPosition(pos);
   m_ui.previewText->setTextCursor(cursor);
   m_ui.previewText->ensureCursorVisible();

   m_taint = false;
}


//! Generates the input deck based on the list of Jobs and prints this to the
//! preview text box.
QString InputDialog::generateInputDeck(bool preview) {
   return generateInputDeckJobs(preview).join("\n@@@\n\n");
}


//! Generates a list of strings containing the input for each job.
QStringList InputDialog::generateInputDeckJobs(bool preview) {
   QStringList jobStrings;

   if (m_currentJob) finalizeJob();
   capturePreviewText();
#ifdef AVOGADRO
   if (m_currentJob) {
/// FIXME - this function is currently missing
//      m_currentJob->setSection("molecule",
//         ExtractGeometry(m_molecule, m_currentJob->getOption("QUI_COORDINATES")));
   }
#endif
   for (unsigned int i = 0; i < m_jobs.size(); ++i) {
       jobStrings << m_jobs[i]->format(preview);
   }

   return jobStrings;
}


int InputDialog::currentJobNumber() {
   for (unsigned int i = 0; i < m_jobs.size(); ++i) {
       if (m_currentJob == m_jobs[i]) return i;
   }
   return 0;
}


bool InputDialog::firstJob(Job* job) {
   bool first(false);
   if (job && m_jobs.size() > 0) {
      first = (job == m_jobs[0]);
   }
   return first;
}


bool InputDialog::hasValidMultiplicity() {
   bool isValid(true); // Avoid upsetting things in the standalone QUI

#ifdef AVOGADRO
   if (m_molecule) {
      int Z = TotalChargeOfNuclei(m_molecule);
      int Q = m_currentJob->getOption("QUI_CHARGE").toInt();
      int M = m_currentJob->getOption("QUI_MULTIPLICITY").toInt();
      int N = Z - Q;

      if (N <= 0) {
         isValid = false;
      }else if ( (M <= N+1) &&
                 ((N % 2) != (M % 2)) ) {
         isValid = true;
      }else {
         isValid = false;
      }
   }
#endif

   return isValid;
}


void InputDialog::printSection(String const& name, bool doPrint) {
   if (m_currentJob) m_currentJob->printSection(name, doPrint);
}

void InputDialog::updateLJParameters() {
   if (m_currentJob) {
      LJParametersSection* lj = new LJParametersSection();
      lj->generateData(m_currentJob->getCoordinates());
      m_currentJob->addSection(lj);
   }
}

void InputDialog::appendNewJob()
{}
void InputDialog::appendJob(Job*)
{}

} // end namespace Qui

#include "InputDialog.moc"
