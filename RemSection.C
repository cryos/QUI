/*!
 *  \file RemSection.C 
 *
 *  \brief Non-inline member functions of the RemSection class, see
 *  RemSection.h for details.
 *   
 *  \author Andrew Gilbert
 *  \date January 2008
 */

#include "RemSection.h"
#include "Option.h"
#include "OptionDatabase.h"
#include <QtDebug>

#include <math.h>



namespace Qui {


std::map<QString,QString> RemSection::m_adHoc;

//! Sets up the ad-hoc map which converts values in the QUI to values used in
//! QChem and vice versa.  value1 is the value used by the QUI and value2  is
//! that used by QChem.  The values are stored as rem::value1 => value2 and also
//! rem::value2 => value1.  This function is called in 
//! InputDialog::initializeControl() which avoids referencing the database
//! unnecessarily.
void RemSection::addAdHoc(QString const& rem, QString const& value1, 
   QString const& value2) {
   m_adHoc[rem.toUpper() + "::" + value1] = value2;
   m_adHoc[rem.toUpper() + "::" + value2] = value1;
}


void RemSection::init() {
   m_options.clear();
   m_options["QUI_CHARGE"] = "0";
   m_options["QUI_MULTIPLICITY"] = "1";
   m_options["QUI_COORDINATES"] = "Cartesian";

   m_options["EXCHANGE"] = "HF";
   m_toPrint.insert("EXCHANGE");

   m_options["BASIS"] = "6-31G";
   m_toPrint.insert("BASIS");

   m_options["GUI"] = "1";
   m_toPrint.insert("GUI");

   // Necessary for obsure reasons.  Essentially this is a hack for when we
   // want to combine several controls into the one rem.  Only one of them
   // triggers the print to the input file, but the others also have to be in
   // the m_options list as they will be referenced.
   m_options["QUI_RADIAL_GRID"] = "50";

   m_options["QUI_XOPT_SPIN1"]  = "Low";
   m_options["QUI_XOPT_IRREP1"] = "1";
   m_options["QUI_XOPT_STATE1"] = "0";
   m_options["QUI_XOPT_SPIN2"]  = "Low";
   m_options["QUI_XOPT_IRREP2"] = "1";
   m_options["QUI_XOPT_STATE2"] = "0";
}


void RemSection::read(QString const& input) {
   init();
   // Bit of a hack here.  The file to be read in may not have GUI set, so we
   // clear it here to avoid including it prematurely.
   printOption("GUI",false);  
   m_options["GUI"] = "0";

   QStringList lines( input.trimmed().split("\n", QString::SkipEmptyParts) );
   QStringList tokens;
   QString line;

   for (int i = 0; i < lines.count(); ++i) {
       line = lines[i].replace(QChar('='),QChar(' '));
       tokens = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);
       if (tokens.count() >= 2) {
          QString rem(tokens[0].toUpper());
          QString value(tokens[1]);
          fixOptionForQui(rem, value);
          setOption(rem, value);
          printOption(rem, true);
       }else {
          qDebug() << "ERROR: RemSection::read() line:" << lines[i];
       }
   }
}



QString RemSection::dump()  {
   QString s("$rem\n");
   std::map<QString,QString>::const_iterator iter;
   QString name, value;

   for (iter = m_options.begin(); iter != m_options.end(); ++iter) {
       name  = iter->first;
       value = iter->second;
       //qDebug() << "RemSection::dump()" << name << value << printOption(name);
       if (printOption(name) && fixOptionForQChem(name, value)) {
          //s += QString("   %1  %2").arg(name,-25).arg(value,-20) + "\n";
          s += "   " + name + "  =  " + value + "\n";
       }
   }

   s += "$end\n";
   return s;
}


RemSection* RemSection::clone() const {
   RemSection* rs = new RemSection();
   rs->setOptions(m_options);
   rs->setPrint(m_toPrint);
   return rs;
}


void RemSection::setOptions(std::map<QString,QString> const& options) {
   m_options.clear();
   m_options = options;
}


void RemSection::setPrint(std::set<QString> const& toPrint) {
   m_toPrint.clear();
   m_toPrint = toPrint;
}


void RemSection::printOption(QString const& option, bool print) {
   if (print) {
      m_toPrint.insert(option);
   }else { 
       m_toPrint.erase(option);
   }
}


bool RemSection::printOption(QString const& name) {
   std::set<QString>::iterator iter(m_toPrint.find(name));
   return !(iter == m_toPrint.end());
}



std::map<QString,QString> RemSection::getOptions() {
   return m_options;
} 


QString RemSection::getOption(QString const& name) {
   QString val;
   if (m_options.count(name) > 0) {
      val = m_options[name];
   }
   return val;
}

bool RemSection::fixOptionForQui(QString& name, QString& value) {
   //qDebug() << "Fixing option for QUI" << name << "=" << value;
   Option opt;
   OptionDatabase& db = OptionDatabase::instance();
   bool inDatabase(db.get(name, opt));

   // Perform some ad hoc conversions.  These are all triggered in the database
   // by having an entry of the form a//b where a is replaced by b in the input
   // file.  These are set in the InputDialog::initializeControl member function
   QString key = name + "::" + value;
   if (m_adHoc.count(key)) value = m_adHoc[key];

   //fix logicals
   if (inDatabase && opt.getType() == Option::Type_Logical) {
      if (value.toLower() == "true") {
         value = "1";
      }else if (value.toLower() == "false") {
         value = "0";
      }else if (value.toInt() != 0) {
         value = "1";
      }
   }

   // Fix CCman real variables
   if (name == "CC_DIIS_MAXIMUM_OVERLAP" || 
       name == "CC_DOV_THRESH" ||
       name == "CC_DTHRESHOLD" ||
       name == "CC_HESSIAN_THRESH" ||
       name == "CC_THETA_STEPSIZE") {

       //qDebug() << "1) " << value;
       QString tmp(value);
       tmp.chop(2);
       bool ok(true);
       int exp = value.right(2).toInt(&ok);
       double man = tmp.toDouble(&ok);
       //qDebug() << "2) " << man << "x10-e" << exp;
       man *= pow(10.0,-exp);
       value = QString::number(man);
       
   }else if (inDatabase && opt.getType() == Option::Type_Real) {
      //fix other reals
      value = QString::number(value.toDouble() * opt.doubleStep());
   }

   if (name == "SCF_GUESS_MIX") {
      value = QString::number(value.toInt()*10);
   }

   if (name == "XC_GRID") {
      bool isInt(false);
      int g = value.toInt(&isInt); 
      if (isInt) {
         int a = g % 1000000;
         int r = g / 1000000;
         value =QString::number(a); 
         m_options["QUI_RADIAL_GRID"] = QString::number(r);
      }
   }

   //qDebug() << "Fixing option for QUI" << name << "=" << value;
   return true;
}


bool RemSection::fixOptionForQChem(QString& name, QString& value) {
   //qDebug() << "Fixing option for QChem" << name << "=" << value;
   bool shouldPrint(true);
   bool isInt;
   Option opt;
   OptionDatabase& db = OptionDatabase::instance();
   bool inDatabase(db.get(name, opt));

   // Skip internal QUI options
   if (name.startsWith("qui_",Qt::CaseInsensitive)) shouldPrint = false;

   // Perform some ad hoc conversions.  These are all triggered in the database
   // by having an entry of the form a//b where a is replaced by b in the input
   // file.  These are set in the InputDialog::initializeControl member function
   QString key = name + "::" + value;
   if (m_adHoc.count(key)) value = m_adHoc[key];

   //fix logicals
   if (inDatabase && opt.getType() == Option::Type_Logical) {
      if (name == "GUI") {
         value = value.toInt() == 0 ? QString::number(0) : QString::number(2);
      }else if (value.toInt() == Qt::Checked) {
         value = QString::number(1);
      }
   }



   //fix reals
   if (name == "CC_DIIS_MAXIMUM_OVERLAP" || 
       name == "CC_DOV_THRESH" ||
       name == "CC_DTHRESHOLD" ||
       name == "CC_HESSIAN_THRESH" ||
       name == "CC_THETA_STEPSIZE") {

       bool ok(true);
       double val = value.toDouble(&ok);

       int exp = floor(log10(val));
       int man = 100 * val * pow(10,-exp);
       //qDebug() << "2) " << man << "x10e" << exp+2;

       value = QString::number(man) + QString("%1").arg(exp+2,2,10,QChar('0'));

   }else if (inDatabase && opt.getType() == Option::Type_Real) {
      value = QString::number(value.toDouble() / opt.doubleStep());
   }


   if (name == "SCF_GUESS_MIX") {
      value = QString::number(value.toInt()/10);
   }

   if (name == "QUI_FROZEN_CORE" && value.toInt() != 0) {
      name = "N_FROZEN_CORE";
      value = "FC";
      shouldPrint = true;
   }

   if (name == "XC_GRID") {
      int ang(value.toInt(&isInt));

      if (isInt) {
         value = QString("%1").arg(ang);
         value = m_options["QUI_RADIAL_GRID"] + value.rightJustified(6,'0');
      }

   }

  
   if (name == "QUI_XOPT1" && value.toInt() > 0) {
      name = "XOPT_STATE_1";
      shouldPrint = true;

      // This is crappy
      key = "QUI_XOPT_SPIN1::";      
      key += m_options["QUI_XOPT_SPIN1"];
      if (m_adHoc.count(key)) m_options["QUI_XOPT_SPIN1"] = m_adHoc[key];

      value = "[" + m_options["QUI_XOPT_SPIN1"]  + ", "
                  + m_options["QUI_XOPT_IRREP1"] + ", "
                  + m_options["QUI_XOPT_STATE1"] + "]";

   }

   if (name == "QUI_XOPT2" && value.toInt() > 0)  {
      name = "XOPT_STATE_2";
      shouldPrint = true;

      key = "QUI_XOPT_SPIN2::";      
      key += m_options["QUI_XOPT_SPIN2"];
      if (m_adHoc.count(key)) m_options["QUI_XOPT_SPIN2"] = m_adHoc[key];

      value = "[" + m_options["QUI_XOPT_SPIN2"]  + ", "
                  + m_options["QUI_XOPT_IRREP2"] + ", "
                  + m_options["QUI_XOPT_STATE2"] + "]";
   }
 

   //qDebug() << "Fixing option for QChem" << name << "=" << value << "print = " << shouldPrint;
   return shouldPrint;
}


void RemSection::printAdHoc() {
   std::map<QString,QString>::iterator iter;
   for (iter = m_adHoc.begin(); iter != m_adHoc.end(); ++iter) {
      qDebug() << "ADHOC::" << iter->first << "->" << iter->second;
   }
}

} // end namespace Qui
