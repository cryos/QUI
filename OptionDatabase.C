/*!
 *  \file OptionDatabase.C
 *
 *  \brief Non-inline member functions of the OptionDatabase class, see
 *  OptionDatabase.h for details.
 *
 *  \author Andrew Gilbert
 *  \date August 2008
 */

#include <QString>
#include <QDebug>
#include <QVariant>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlDriver>
#include <QSqlError>
#include <QMessageBox>
#include <QStringList>
#include <QApplication>
#include <QSqlDatabase>

#include "OptionDatabase.h"
#include "Option.h"

#include <cstdlib>
#include <iostream>  // tmp


namespace Qui {

OptionDatabase* OptionDatabase::s_instance = 0;
std::vector<QString> OptionDatabase::s_dbFields;
bool OptionDatabase::s_okay;


OptionDatabase& OptionDatabase::instance() {
   if (s_instance == 0) {
      s_instance = new OptionDatabase();
      if (!s_okay) QApplication::exit();
      std::atexit(OptionDatabase::destroy);
   }

   return *s_instance;
}


void OptionDatabase::init() {
   s_dbFields.push_back("Name");
   s_dbFields.push_back("Type");
   s_dbFields.push_back("Default");
   s_dbFields.push_back("Options");
   s_dbFields.push_back("Description");
   s_dbFields.push_back("Implementation");
}



void OptionDatabase::destroy() {
}



OptionDatabase::OptionDatabase() {
   s_okay = false;
   QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "QChem");

   QString dbFilename(QCoreApplication::applicationDirPath());

#ifdef Q_WS_MAC
   dbFilename += "/../Resources/qchem_option.db";
#else
   dbFilename += "/qchem_option.db";
#endif


   qDebug() << "Database file set to:" << dbFilename;
   db.setDatabaseName(dbFilename);

   if (db.open()) {
      qDebug() << "Database file opened okay";
      QStringList tables = db.tables();

      if (tables.contains("options")) {
         s_okay = true;
      }else {
         qDebug() << "ERROR: Option data not found in" << dbFilename;
      }

   }else {
      QString msg("Could not open option database file: ");
      msg += dbFilename;
      msg += "\n\nQSqlDatabase Error:\n";
      msg += db.lastError().text();
      msg += "\nAvailable drivers::\n";
      msg += QSqlDatabase::drivers().join(", ");
      QMessageBox::critical(0, "EGAD!", msg);
   }
}


QStringList OptionDatabase::all() {

   bool okay(false);
   QString buf("select * from options");

   QSqlQuery query( QSqlDatabase::database("QChem") );
   query.setForwardOnly(true);

   if (query.prepare(buf) && query.exec()) {
      okay = true;
   }else {
     QString msg("Database transaction failed:\n");
     msg += buf + "\n";
     msg += query.lastError().databaseText();
     QMessageBox::warning(0, "EGAD!", msg);
     exit(1);
   }

   QStringList options;

   if (okay) {
      while (query.next()) {
         options.append(query.value(0).toString());
      }
   }

   return options;
}



//! Executes an SQL command, displaying an error box if things go awry.
bool OptionDatabase::execute(QString const& sql) {
   bool okay(false);
   QSqlQuery query( QSqlDatabase::database("QChem") );
   query.setForwardOnly(true);

   if (query.prepare(sql) && query.exec()) {
      okay = true;
   }else {
     QString msg("Database transaction failed:\n");
     msg += sql + "\n";
     msg += query.lastError().databaseText();
     QMessageBox::warning(0, "EGAD!", msg);
   }

   return okay;
}



//! Inserts an Option into the database, overwriting the record if the
//! option name already exists.  The user is prompted if an overwrite will
//! occur and if \var promptOnOverwrite is set to true.
bool OptionDatabase::insert(Option const& opt, bool const promptOnOverwrite) {
   Option tmp;
   QString name(opt.getName());

   if ( get(name, tmp) ) {
      if (promptOnOverwrite) {
         QString msg("Option name ");
         msg += name;
         msg += " already exists in database, overwrite?";
         int ret = QMessageBox::question(0, "Option Exists",msg,
            QMessageBox::Ok | QMessageBox::Cancel);
         if (ret == QMessageBox::Cancel) {
            return false;
         }
      }
      remove(name,false);
   }

   QString buf("insert into options( 'Name', 'Type', 'Default', 'Options', "
      "'Description', 'Implementation' ) values ( '");
   buf += opt.getName() + "', ";
   buf += QString::number(opt.getType()) + ", ";
   buf += QString::number(opt.getDefaultIndex()) + ", '";
   buf += opt.getOptionString() + "', ";   // No quote!

   QSqlField desc("Description", QVariant::String);
   desc.setValue(opt.getDescription());
   buf += QSqlDatabase::database("QChem").driver()->formatValue(desc);
   buf += ", ";
   buf += QString::number(opt.getImplementation()) + ");";

   std::cout << "Database insert: " << buf.toStdString() << std::endl;

   return execute(buf);
}



//! Deletes the named Option record from the OptionDatabase.  The user is
//! prompted if \var prompt is set to true.
bool OptionDatabase::remove(QString const& optionName, bool const prompt) {

   if (prompt) {
      QString msg("Permanently delete the ");
      msg += optionName;
      msg += " record from the option database?";

      int ret = QMessageBox::question(0, "Delete Option?",msg,
         QMessageBox::Ok | QMessageBox::Cancel);
      if (ret == QMessageBox::Cancel) {
         return false;
      }
   }

   QString buf("delete from options where Name = '");
   buf += optionName + "';";

   std::cout << "Database remove: " << buf.toStdString() << std::endl;

   return execute(buf);
}



//! Searches the OptionDatabase for \var optionName and, if found, returns the
//! Option record in \var option.  Returns true if found.
bool OptionDatabase::get(QString const& optionName, Option& option) {

   bool found(false);
   bool okay(false);

   QString buf("select * from options where Name = '");
   buf += optionName;
   buf += "';";

   QSqlQuery query( QSqlDatabase::database("QChem") );
   query.setForwardOnly(true);

   if (query.prepare(buf) && query.exec()) {
      okay = true;
   }else {
     QString msg("Database transaction failed:\n");
     msg += buf + "\n";
     msg += query.lastError().databaseText();
     qDebug() << msg;
     exit(1);
     //QMessageBox::warning(0, "EGAD!", msg);
   }


   if (okay) {
      if (query.next()) {
         found = true;

         option.setName(query.value(0).toString());
         option.setType(query.value(1).toInt());
         option.setDefault(query.value(2).toInt());
         option.setOptions(query.value(3).toString());
         option.setDescription(query.value(4).toString());
         option.setImplementation(query.value(5).toInt());

         if (query.next()) {
            QString msg("More than one record for ");
            msg += optionName;
            msg += " found in database.";
            QMessageBox::information(0, "EGAD!", msg);
         }

      }
      while (query.next()) {}
   }

   return found;
}


} // end namespace Qui
