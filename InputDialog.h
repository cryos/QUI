#ifndef QUI_INPUTDIALOG_H 
#define QUI_INPUTDIALOG_H 
/*!
 *  \class InputDialog 
 *  
 *  \brief This is the main dialog for setting input options for QChem.
 *  
 *  \author Andrew Gilbert
 *  \date   August 2008
 */


//#include "ui_InputDialog.h"
#include "ui_QuiMainWindow.h"

#include "OptionRegister.h"
#include <QFileInfo>
#include <QFont>
#include <QProcess>
#include <vector>


#ifdef AVOGADRO
namespace Avogadro {
   class Molecule;
}
#endif


class QResizeEvent;


namespace Qui {

class OptionDatabase;
class QtNode;
class Option;
class Job;

namespace Process {
   class Monitor;
   class Monitored;
   class Queue;
}

template<class K, class T> class Register;

typedef std::map<String,String> StringMap;
typedef boost::function<void(String const&)> Update;

class InputDialog : public QMainWindow {

   Q_OBJECT

   public:
      InputDialog(QWidget* parent = 0);
      ~InputDialog();


   private Q_SLOTS:
	  // Automatic slots, these are connected by the moc based on the function
	  // names.
      void on_advancedOptionsTree_itemClicked(QTreeWidgetItem* item, int col);
      void on_job_type_currentIndexChanged(QString const& text);
      void on_jobList_currentIndexChanged(int);
      void on_stackedOptions_currentChanged(int);
      void on_previewText_textChanged() { m_taint = true; }

      void on_editConstraintsButton_clicked(bool)  { editConstraints(); }
      void on_editConstraintsButton2_clicked(bool) { editConstraints(); }
      void on_readChargesButton_clicked(bool) {readCharges(); }

      void on_qui_title_textChanged();
      void on_qui_charge_valueChanged(int);
      void on_qui_multiplicity_valueChanged(int);

      void on_buildButton_clicked(bool) { build(); }
      void on_submitButton_clicked(bool) { submitJob(); }
      void on_addJobButton_clicked(bool) { appendNewJob(); }
      void on_deleteJobButton_clicked(bool); 


      // Radio toggles for switching pages on stacked widgets
      void toggleStack(QStackedWidget* stack, bool on, QString model);
      void on_use_case_toggled(bool);
      void on_ftc_toggled(bool);
      void on_qui_cfmm_toggled(bool);
      void on_qui_solvent_onsager_toggled(bool);
      void on_qui_solvent_none_toggled(bool);
      void on_smx_solvation_toggled(bool on);
      void on_svp_toggled(bool);
      void on_chemsol_toggled(bool);
  

      // Manual slots
      void widgetChanged(QObject* orig, QString const& value);
      void widgetChanged(QString const& value);
      void widgetChanged(int const& value);
      void widgetChanged(bool const& value);

      void changeComboBox(QString const& name, QString const& value);
      void changeCheckBox(QString const& name, QString const& value);
      void changeSpinBox(QString const& name, QString const& value);
      void changeDoubleSpinBox(QString const& name, QString const& value);
      void changeLineEdit(QString const& name, QString const& value);
      void changeRadioButton(QString const& name, QString const& value);

      void updatePreviewText();

      // QProcess slots
      void jobStarted();
      void jobFinished(int exitCode, QProcess::ExitStatus exitStatus);
      void avogadroStarted();
      void avogadroFinished(int exitCode, QProcess::ExitStatus exitStatus);

      void processMonitorClosed() { m_processMonitor = 0; }


      /********** Menu Slots **********/
      void menuOpen();
      void menuSave()   { saveFile(false); }
      void menuSaveAs() { saveFile(true);  }
      void menuClose()  { close();  }

      void menuCopy();
      void menuPasteXYZFromClipboard();
      void menuUndo();
      void menuEditPreferences() { editPreferences(); }

      void menuNew();
      void menuAppendJob() { appendNewJob(); }
      void menuResetJob();
      void menuBuildMolecule() { build(); }
      void menuSubmit() { submitJob(); }
      void menuProcessMonitor();

      void menuSetFont();
      void menuBigger()  { fontAdjust(true);  }
      void menuSmaller() { fontAdjust(false); }


   protected:
      void resizeEvent(QResizeEvent* event);


   private:
      // ---------- Data ----------
#ifdef AVOGADRO
      Avogadro::Molecule* m_molecule;
#else
      void* m_molecule;
#endif
      Ui::MainWindow m_ui;

      // m_fileIn holds the name of the current input file.
      QFileInfo m_fileIn;

      QFileInfo m_fileOut;
      QFileInfo m_fileTmp;
      QFileInfo m_fileFchk;

      OptionDatabase& m_db;
      OptionRegister& m_reg;

      bool m_taint;

      Job* m_currentJob;
      std::vector<Job*> m_jobs;
      std::vector<Action*> m_resetActions;
      std::map<String,Update*> m_setUpdates;

      QProcess* m_currentProcess;
      QProcess* m_avogadro;

      // This contains a list of all the menu actions so that we can selectively
      // activate them later on.
      std::map<QString, QAction*> m_menuActions;

      // This contains the last preview text in case we want to undo
      QString m_rememberMe;

      Process::Monitor* m_processMonitor;
      std::vector<Process::Monitored*> m_processList;
      Process::Queue* m_processQueue;

      // ---------- Functions ----------
 #ifdef AVOGADRO
      void setMolecule(Avogadro::Molecule* molecule);
#endif

      int  currentJobNumber();
      bool firstJob(Job*);
      void capturePreviewText();
      void initializeQuiLogic();
      void widgetError(QString const& name);
      bool deleteAllJobs(bool const prompt = true);
      void addJob(Job*);

      void finalizeJob();
      void setControls(Job* job);
      void resetControls();
      void initializeMenus();
      void initializeControls();
      void initializeControl(Option const& opt, QComboBox* combo);
      void initializeControl(Option const& opt, QCheckBox* check);
      void initializeControl(Option const& opt, QLineEdit* edit);
      void initializeControl(Option const& opt, QSpinBox*  spin);
      void initializeControl(Option const& opt, QDoubleSpinBox* dspin);
      void initializeControl(Option const& opt, QRadioButton* radio);

      void connectControl(Option const& opt, QComboBox* combo);
      void connectControl(Option const& opt, QCheckBox* check);
      void connectControl(Option const& opt, QLineEdit* edit);
      void connectControl(Option const& opt, QSpinBox* spin);
      void connectControl(Option const& opt, QRadioButton* radio);
      void connectControl(Option const& opt, QDoubleSpinBox* dspin);

      void printSection(String const& name, bool doPrint);
      void updateLJParameters();
      bool hasValidMultiplicity();
      bool saveFile(bool prompt = false);
      void editConstraints();
      void readCharges();
      void insertXYZ(QString const& coordinates);

      QString generateInputDeck(bool preview);
      QStringList generateInputDeckJobs(bool preview);
      void watchProcess(Process::Monitored* process);

      void fontAdjust(bool);
      void changePreviewFont(QFont const& font);
      void launchAvogadro(QString const& fileName = "");

      void displayCheckpointFile();
      void displayJobOutput();
      void build();
      void submitJob();
      void addJobToList(Job*);
      void appendNewJob();
      void appendJob(Job*);
      void editPreferences();

};

} // end namespace Qui

#endif
