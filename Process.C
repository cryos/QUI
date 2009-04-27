/*!
 *  \file Process.C
 *
 *  \author Andrew Gilbert
 *  \date   March 2009
 */

#include "FileDisplay.h"
#include "Preferences.h"
#include "Process.h"
#include "Qui.h"

#include <QDir>
#include <QList>
#include <QtDebug>
#include <signal.h>
#include <QMessageBox>
#include <QHeaderView>


namespace Qui {
namespace Process {


QString ToString(Status::ID const& state) {
   QString s;

   switch (state) {
      case Status::NotRunning: { s = "Not Running"; } break;
      case Status::Starting:   { s = "Starting";    } break;
      case Status::Running:    { s = "Running";     } break;
      case Status::Queued:     { s = "Queued";      } break;
      case Status::Crashed:    { s = "Crashed";     } break;
      case Status::Killed:     { s = "Killed";      } break;
      case Status::Error:      { s = "Error";       } break;
      case Status::Finished:   { s = "Finished";    } break;
      default:                 { s = "Unknown";     } break;
   }
   return s;
}


// ********** Process ********** //

void Process::start() {
   QProcess::start(m_program, m_arguments);
}

int Process::pid() {
   return QProcess::pid();
}

void Process::kill() {
   QProcess::kill();
}

void Process::setStandardOutputFile(QString const& fileName) {
   QProcess::setStandardOutputFile(fileName);
}

void Process::setArguments(QStringList const& arguments) {
   m_arguments = arguments;
}


//! Returns an identifyer for the status of the process.
Status::ID Process::status() const {
   int s(state());
   if (!m_started) {
      return Status::Queued;
   }else if (s == QProcess::Starting) {
      return Status::Starting;
   }else if (s == QProcess::Running) {
      return Status::Running;
   }else {
      return m_status;
   }
}


// ********** Timed ********** //

Timed::Timed(QObject* parent,
             QString const& program,
             QStringList const& arguments)
  : Process(parent, program, arguments), m_days(0), m_elapsedTime(0),
    m_dayTimer(new QTimer()) {

   m_dayTimer->setInterval(1000 * 60 * 60 * 24); // number of msec in a day
   connect(m_dayTimer, SIGNAL(timeout()), this, SLOT(anotherDay()));
   connect(this, SIGNAL(finished(int, QProcess::ExitStatus)),
      this, SLOT(finish(int, QProcess::ExitStatus)));
}


void Timed::start() {
   m_startTime.start();
   m_dayTimer->start();
   Process::start();
   m_formattedStartTime = m_startTime.toString("hh:mm:ss");
}


void Timed::finish(int, QProcess::ExitStatus) {
   m_elapsedTime = m_startTime.elapsed();
   m_dayTimer->stop();
}


QString Timed::formattedTime() {
   int time(m_elapsedTime);
   if (state() == QProcess::Running) {
      time = m_startTime.elapsed();
   }

   time /= 1000;
   int secs = time % 60;
   time /= 60;
   int mins = time % 60;
   time /= 60;
   int hours = time;

   QString t;
   if (m_days > 0) {
      t = QString::number(m_days) + " days ";
   }

   QTime qtime(hours, mins, secs);
   t += qtime.toString("hh:mm:ss");
   return t;
}



// ********** Monitored ********** //

Monitored::Monitored(QObject* parent, QString const& program,
   QStringList const& arguments)
   : Timed(parent, program, arguments), m_error("") {

   connect(this, SIGNAL(started()), this, SLOT(processStarted()));
   connect(this, SIGNAL(finished(int, QProcess::ExitStatus)),
      this, SLOT(processFinished(int, QProcess::ExitStatus)));
}


void Monitored::processStarted() {
   m_started = true;
}

void Monitored::kill() {
   Process::kill();
   m_status = Status::Killed;
}


void Monitored::processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
   if (m_status == Status::Killed) {
      // do nothing
   }else if (exitStatus == QProcess::CrashExit) {
      m_status = Status::Crashed;
   }else {
      m_status = Status::Finished;
   }
   m_exitCode = exitCode;
}


void Monitored::setOutputFile(QString const& fileName) {
   m_outputFile = fileName;
   Process::setStandardOutputFile(fileName);
}



// ********** QChem Process ********** //

QChem::QChem(QObject* parent, QString const& input, QString const& output)
  : Monitored(parent, Preferences::QChemRunScript()) {

   setInputFile(input);
   setOutputFile(output);

   QFileInfo inputFileInfo(input);
   qDebug() << "Setting Process::QChem working directory to"
            << inputFileInfo.path();
   setWorkingDirectory(inputFileInfo.path());

   QStringList args;
   args << inputFileInfo.fileName();
   setArguments(args);

   connect(this, SIGNAL(finished(int, QProcess::ExitStatus)),
      this, SLOT(cleanUp(int, QProcess::ExitStatus)));
}



void QChem::cleanUp(int, QProcess::ExitStatus) {
   checkForErrors();
   renameFchkFile();
}


void QChem::checkForErrors() {
   QFile file(outputFile());
   QStringList lines(ReadFileToList(file));
   QRegExp rx ("*Q-Chem fatal error*");
   rx.setPatternSyntax(QRegExp::Wildcard);

   int error(lines.indexOf(rx));
   if (error >= 0 && lines.size() > error+2) {
      m_error = lines[error+2];
      m_status = Status::Error;
   }
}


void QChem::renameFchkFile() {
   QFileInfo output(outputFile());
   QFileInfo fchk(output.completeBaseName() + ".FChk");

   QFileInfo tmp(output.path() + "/Test.FChk");
   if (tmp.exists()) {
      QDir dir(output.dir());
      dir.rename(tmp.filePath(), fchk.filePath());
      setAuxFile(fchk.filePath());
   }
}



// ********** Monitor ********** //

Monitor::Monitor(QWidget* parent,
   std::vector<Monitored*> const& processList,
   int updateInterval) : QMainWindow(parent)  {

   m_ui.setupUi(this);
   initializeMenus();

   m_ui.processTable->hideColumn(0);
   m_ui.processTable->hideColumn(3);
   m_ui.addProcessButton->hide();

   std::vector<Monitored*>::const_iterator iter;
   for (iter = processList.begin(); iter != processList.end(); ++iter) {
       addProcess(*iter);
   }

    m_ui.processTable->verticalHeader()->
       setDefaultSectionSize(fontMetrics().lineSpacing() + 5);

   m_timer = new QTimer(this);
   m_timer->setInterval(updateInterval);
   m_timer->start();

   connect(m_timer, SIGNAL(timeout()), this, SLOT(refresh()));
}


void Monitor::initializeMenus() {
   QMenuBar* menubar(menuBar());
   QAction*  action;
   QMenu*    menu;

   menubar->clear();

   // File
   menu = menubar->addMenu(tr("File"));

   // File -> Close
   action = menu->addAction(tr("Close"));
   connect(action, SIGNAL(triggered()), this, SLOT(menuClose()));
   action->setShortcut(QKeySequence::Close);

   // File -> Refresh
   action = menu->addAction(tr("Refresh"));
   connect(action, SIGNAL(triggered()), this, SLOT(refresh()));
   action->setShortcut(Qt::CTRL + Qt::Key_R);
}


void Monitor::menuClose() {
   close();
   deleteLater();
}


void Monitor::on_addProcessButton_clicked(bool) {
   Monitored* p = new Monitored(this, "sleep");
   addProcess(p);
   QStringList args;
   args << QString::number(60);
   p->setArguments(args);
   p->start();
}


void Monitor::on_removeProcessButton_clicked(bool) {
   ProcessMap::iterator iter(findSelectedProcess());
   if (iter != m_processList.end()) {
      if (iter->second->state() == QProcess::NotRunning) {
         qDebug() << "Need to remove process from list" << iter->first;
         m_timer->stop();
         QList<QTableWidgetItem*> items(m_ui.processTable->selectedItems());
         m_ui.processTable->removeRow(items[0]->row());
qDebug() << "Sending processRemoved() signal" << iter->second;
         processRemoved(iter->second);
         m_processList.erase(iter);
         refresh();
         m_timer->start();
      }else {
         QString msg("Cannot remove active processes from the process list.");
         QMessageBox::warning(this,"Error",msg);
      }
   }
}


Monitor::ProcessMap::iterator Monitor::findSelectedProcess() {
   QList<QTableWidgetItem*> items(m_ui.processTable->selectedItems());
   if (!items.isEmpty()) {
      int row(items[0]->row());
      QString key(m_ui.processTable->item(row,0)->text());
      return m_processList.find(key);
   }
   return m_processList.end();
}


void Monitor::on_stopProcessButton_clicked(bool) {
   ProcessMap::iterator iter(findSelectedProcess());
   if (iter != m_processList.end()) {
      if (iter->second->state() != QProcess::NotRunning) {
         iter->second->kill();
      }
   }
}


void Monitor::on_viewOutputButton_clicked(bool) {
   QList<QTableWidgetItem*> items = m_ui.processTable->selectedItems();
   if (!items.isEmpty()) {
      // Selection mode is set to single, so only one row at a time should be
      // able to be selected.
      displayOutputFile(items[0]->row());
   }
}



void Monitor::addProcess(Monitored* process) {
   m_processList[QString::number(qulonglong(process))] = process;
   QTableWidget* table(m_ui.processTable);

   int row = table->rowCount();
   table->insertRow(row);
   for (int i = 0; i < table->columnCount(); ++i) {
       table->setItem(row, i, new QTableWidgetItem());
   }
   updateRow(row, process);
}


void Monitor::refresh() {
   QTableWidget* table(m_ui.processTable);
   QTableWidgetItem* item;
   ProcessMap::iterator iter;

   table->setSortingEnabled(false);

   for (int row = 0; row < table->rowCount(); ++row) {
       item = table->item(row, 0);
       iter = m_processList.find(item->text());
       if (iter != m_processList.end() ) {
          updateRow(row, iter->second);
       }else {
          qDebug() << "!!! Could not find process" << item->text();
       }
   }

   table->setSortingEnabled(true);
   m_ui.processTable->hideColumn(0);
   m_ui.processTable->hideColumn(3);
}


void Monitor::updateRow(int row, Monitored* process) {
   QTableWidget* table(m_ui.processTable);

   QString s = QString::number(qulonglong(process));
   table->item(row,0)->setText(s);

   s = QString::number(process->pid());
   table->item(row,1)->setText(s);

   s = process->formattedStartTime();
   table->item(row,2)->setText(s);

   s = process->programName();
   table->item(row,3)->setText(s);

   s = process->arguments();
   table->item(row,4)->setText(s);

   s = process->formattedTime();
   table->item(row,5)->setText(s);

   s = ToString(process->status());
   table->item(row,6)->setText(s);
   if (s.contains("Error")) {
      table->item(row,6)->setToolTip(process->error());
   }
}


void Monitor::on_processTable_cellDoubleClicked(int row, int) {
   displayOutputFile(row);
}


void Monitor::displayOutputFile(int row) {
   QTableWidgetItem* item = m_ui.processTable->item(row, 0);
   ProcessMap::iterator iter = m_processList.find(item->text());

   if (iter != m_processList.end() ) {
      QString outputFile(iter->second->outputFile());
      if (!outputFile.isEmpty()) {
         FileDisplay* fileDisplay = new FileDisplay(this, outputFile);
         fileDisplay->show();
      }else {
         qDebug() << "empty file handle in displayOutputFile ";
      }
   }else {
      qDebug() << "!!! Could not find process in cellDoubleClicked row = "  << row;
   }
}



// ********** Queue ********** //

void Queue::submit(Process* process) {
   connect(process, SIGNAL(finished(int, QProcess::ExitStatus)),
      this, SLOT(processFinished(int, QProcess::ExitStatus)));
   m_processQueue.push(process);
   runQueue();
}


void Queue::processFinished(int, QProcess::ExitStatus) {
   --m_nProcesses;
   runQueue();
}


void Queue::remove(Process* process) {
qDebug() << "removing processs from queue" << process;
qDebug() << "  current queue size = " << m_processQueue.size();
   Process* p;
   for (unsigned int i = 0; i < m_processQueue.size(); ++i) {
       p = m_processQueue.front();
       m_processQueue.pop();
       if (p != process) m_processQueue.push(p);
   }
qDebug() << "  new queue size = " << m_processQueue.size();
}


void Queue::runQueue() {
   while (!m_processQueue.empty() && m_nProcesses < m_maxProcesses) {
      m_processQueue.front()->start();
      m_processQueue.pop();
      ++m_nProcesses;
   }
}


} } // end namespaces Qui::Process

#include "Process.moc"
