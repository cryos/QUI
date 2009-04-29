/*!
 *  \file MoleculeSection.C 
 *
 *  \brief Non-inline member functions of the MoleculeSection class, see
 *  MoleculeSection.h for details.
 *   
 *  \author Andrew Gilbert
 *  \date February 2008
 */

#include "MoleculeSection.h"

#include <QRegExp>
#include <QStringList>
#include <QMessageBox>

#include <QtDebug>


namespace Qui {


void MoleculeSection::read(QString const& input) {
   QStringList lines( input.trimmed().split(QRegExp("\\n")) );
   //QStringList lines( input.trimmed().split(QRegExp("\\n"), QString::SkipEmptyParts) );
   bool okay(false);

   if (lines.count() > 0) {
      QStringList tokens(lines[0].split(QRegExp("\\s+"), QString::SkipEmptyParts));
      qDebug() << "Error:" << tokens.count() << lines[0];
      lines.removeFirst();

      if (tokens.count() == 1) {
         if (tokens[0].toLower() == "read") {
            setCoordinates("read");
            okay = true;
         }
      }
      else if (tokens.count() == 2) {
         // line 1 is charge + multiplicity
         // everything else is the molecule
         bool c,m;
         m_charge = tokens[0].toInt(&c);
         m_multiplicity = tokens[1].toInt(&m);
         okay = c && m;
         setCoordinates(lines.join("\n"));
      }
   }

   // TODO: This should really load a molecule object so that the coordinate
   // conversion can be done.
   if (!okay) {
      QString msg("Problem reading $molecule section: \n");
      msg += input;
      QMessageBox::warning(0, "Parse Error", msg);
   }
}



QString MoleculeSection::dump() {
   QString s("$molecule\n");
   if (m_coordinates == "read") {
      s += m_coordinates+ "\n";
   }else {
      s += QString::number(m_charge) + " ";
      s += QString::number(m_multiplicity) + "\n";
      if (m_coordinates != "") {
         s += m_coordinates + "\n";
      }
   }
   s += "$end\n";
   return s;
}


MoleculeSection* MoleculeSection::clone() const {
   return new MoleculeSection(m_coordinates, m_charge, m_multiplicity);
}


void MoleculeSection::setCoordinates(QString const& coordinates) { 
   m_coordinates = coordinates; 
   parseCoordinates();
}

// TODO: this should be smartened up a bit, for the time being all I want is
// the number of atoms.
void MoleculeSection::parseCoordinates() {
   m_coordinates = m_coordinates.trimmed();
   if (m_coordinates == "read") {
      m_numberOfAtoms = 0;
   }else {
      m_numberOfAtoms = m_coordinates.count(QRegExp("\\n")) + 1; 
   }
   return;
}

} // end namespace Qui
