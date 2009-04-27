/*!
 *  \file ProcessQChemKill.C
 *
 *  \brief Contains the kill function for terminating QChem processes, taking
 *  into account all the wrapper scripts etc and platform dependencies.
 *
 *  \author Andrew Gilbert
 *  \date   March 2009
 */

#include "Process.h"
#include <QMessageBox>

#include <QtDebug>

namespace Qui {
namespace Process {


void QChem::kill() {
   if (status() != Status::Running) {
      return; //no process to kill
   }

   if (QMessageBox::question(0, "Kill Job?", 
       "Are you sure you want to terminate the Q-Chem job " + m_inputFile, 
       QMessageBox::Cancel | QMessageBox::Ok) == QMessageBox::Cancel) {
       return;
   }

   qDebug() << "About to kill QProcess" << QProcess::pid() ;

   int  id(pid());

   if (id > 0) {
      qDebug() << "qcprog.exe found on process" << id;
      if (!KillProcess(id)) {
         QString msg("Unable to kill process ");
         msg += QString::number(id);
         QMessageBox::warning(0, "Kill Job Failed", msg);
      }
   }else {
      QString msg("Unable to determine process ID for job termination");
      QMessageBox::warning(0, "Kill Job Failed", msg);
   }
}



#ifdef Q_WS_WIN

int QChem::pid() const {
   Option opt;
   QString cmd;
   QProcess kill;
   QStringList args;
   bool ok(false);  // should be true only if the job was sucessfully killed

   if (m_db.get("QUI_WINDOWS_KILL_COMMAND", opt)) {
      cmd  = opt.getDefaultValue();
      args = cmd.split(QRegExp("\\s+"), QString::SkipEmptyParts);
      cmd  = args.first();
      args.removeFirst();

      if (m_db.get("QUI_WINDOWS_DIRECTORY", opt)) {
         QString dir(opt.getDefaultValue());
         kill.setWorkingDirectory(dir);
         qDebug() << "Executing command " << cmd << "with args:" << args;
         kill.start(cmd, args);
         if (kill.waitForFinished(5000)) {
            ok = true;
         }else {
            QString msg("Unable to determine process ID for job termination");
            QMessageBox::warning(0, "Kill Job Failed", msg);
            return;
         }
      }
   }

   if (!ok) {
      QString msg("Uninitialized Job termination command. Please contact Q-Chem Inc.");
      QMessageBox::warning(0, "Kill Job Failed", msg);
   }
}

#else

int QChem::pid() const {

   QProcess ps;
   QStringList args;
   int id(QProcess::pid());
   bool found(false);

   args << "xww" << "-o" << "ppid,pid,command";
   qDebug() << "Executing command /bin/ps" << "with args:" << args;
   ps.start("/bin/ps", args);

   if (ps.waitForFinished(5000)) {  // Give ps 5 seconds to respond
      QString      psOutput(ps.readAllStandardOutput());
      QStringList lines(psOutput.split(QRegExp("\\n")));
      QStringList tokens;
       
      for (int i = 0; i < lines.size(); ++i) {
          lines[i] = lines[i].trimmed();
      }

      // count is simply to prevent infinite loops if something wacky
      // goes on
      int count(0);
      while (!found && count < 12) { 
         ++count;
         for (int i = 0; i < lines.size(); ++i) {
             if (lines[i].startsWith(QString::number(id)+" ")) {
                // We have found a child of pid
                tokens = lines[i].split(QRegExp("\\s+"), QString::SkipEmptyParts);
                id = tokens[1].toInt();
                
                if (lines[i].contains("qcprog.exe")) {
                   // We have found the qcprog.exe child of m_currentProcess
                   found = true;
                   break;
                }  
             }
         }
      }

   }

   return found ? id : 0;
}


bool KillProcess(int const pid, int const signal) {
   QStringList args;
   QProcess assassin;
   args << QString::number(signal) << QString::number(pid);
   qDebug() << "Executing command /bin/kill" << "with args:" << args;
   assassin.start("/bin/kill", args);

   if (assassin.waitForFinished(5000)) {
      return true;
   }else {
      assassin.kill();
      return false;
   }
}

#endif

} } // end namespaces Qui::Process
