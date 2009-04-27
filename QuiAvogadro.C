/*!
 *  \file InputDialogAvogadro.C
 *
 *  \brief This file contains extensions to the InputDialog class that are only
 *  relevant when the QUI is compiled as an Avogadro extension.
 *
 *  \author Andrew Gilbert
 *  \date   November 2008
 */

#ifndef AVOGADRO
#error Macro AVOGADRO not defined in InputDialogAvogadro.C
#endif

#include <QTextStream>
#include <QtDebug>
#include "Qui.h"

#include <openbabel/mol.h>
#include <avogadro/molecule.h>
#include <avogadro/atom.h>

using namespace Avogadro;
using namespace OpenBabel;

namespace Qui {

int TotalChargeOfNuclei(Avogadro::Molecule* molecule) {
   int count(0);

   if (molecule) {
      foreach(Atom *atom, molecule->atoms())
         count += atom->atomicNumber();
   }

   return count;
}


QString ExtractGeometry(Avogadro::Molecule* molecule, QString const& coords) {

   QString buffer;
   QTextStream mol(&buffer);
   if (!molecule) return buffer;

   if (coords.toUpper() == "CARTESIAN") {
      foreach(Atom *atom, molecule->atoms()) {
         mol << qSetFieldWidth(3) << right
             << QString(etab.GetSymbol(atom->atomicNumber()))
             << qSetFieldWidth(12) << qSetRealNumberPrecision(5) << forcepoint
             << fixed << right << atom->pos()->x() << atom->pos()->y() << atom->pos()->z()
             << qSetFieldWidth(0) << "\n";
      }
    }

    // Z-matrix
    else if (coords.toUpper() == "Z-MATRIX") {
      OBAtom *a, *b, *c;
      double r, w, t;

      /* Taken from OpenBabel's gzmat file format converter */
      std::vector<OBInternalCoord*> vic;
      vic.push_back((OpenBabel::OBInternalCoord*)NULL);
      OpenBabel::OBMol obmol = molecule->OBMol();
      FOR_ATOMS_OF_MOL(atom, &obmol)
        vic.push_back(new OpenBabel::OBInternalCoord);
      CartesianToInternal(vic, obmol);

      FOR_ATOMS_OF_MOL(atom, &obmol)
      {
        a = vic[atom->GetIdx()]->_a;
        b = vic[atom->GetIdx()]->_b;
        c = vic[atom->GetIdx()]->_c;

        mol << qSetFieldWidth(4) << right
            << QString(etab.GetSymbol(atom->GetAtomicNum())
                       + QString::number(atom->GetIdx()))
            << qSetFieldWidth(0);
        if (atom->GetIdx() > 1)
          mol << " " << QString(etab.GetSymbol(a->GetAtomicNum())
                                + QString::number(a->GetIdx()))
              << " r" << atom->GetIdx();
        if (atom->GetIdx() > 2)
          mol << " " << QString(etab.GetSymbol(b->GetAtomicNum())
                                + QString::number(b->GetIdx()))
              << " a" << atom->GetIdx();
        if (atom->GetIdx() > 3)
          mol << " " << QString(etab.GetSymbol(c->GetAtomicNum())
                                + QString::number(c->GetIdx()))
              << " d" << atom->GetIdx();
        mol << "\n";
      }

      mol << "\n";
      FOR_ATOMS_OF_MOL(atom, &obmol)
      {
        r = vic[atom->GetIdx()]->_dst;
        w = vic[atom->GetIdx()]->_ang;
        if (w < 0.0)
          w += 360.0;
        t = vic[atom->GetIdx()]->_tor;
        if (t < 0.0)
          t += 360.0;
        if (atom->GetIdx() > 1)
          mol << "   r" << atom->GetIdx() << " = " << qSetFieldWidth(15)
              << qSetRealNumberPrecision(5) << forcepoint << fixed << right
              << r << qSetFieldWidth(0) << "\n";
        if (atom->GetIdx() > 2)
          mol << "   a" << atom->GetIdx() << " = " << qSetFieldWidth(15)
              << qSetRealNumberPrecision(5) << forcepoint << fixed << right
              << w << qSetFieldWidth(0) << "\n";
        if (atom->GetIdx() > 3)
          mol << "   d" << atom->GetIdx() << " = " << qSetFieldWidth(15)
              << qSetRealNumberPrecision(5) << forcepoint << fixed << right
              << t << qSetFieldWidth(0) << "\n";
      }
      foreach (OpenBabel::OBInternalCoord *c, vic)
        delete c;
    }
    else {
      OBAtom *a, *b, *c;
      double r, w, t;

      /* Taken from OpenBabel's gzmat file format converter */
      std::vector<OBInternalCoord*> vic;
      vic.push_back((OpenBabel::OBInternalCoord*)NULL);
      OpenBabel::OBMol obmol = molecule->OBMol();
      FOR_ATOMS_OF_MOL(atom, &obmol)
        vic.push_back(new OpenBabel::OBInternalCoord);
      CartesianToInternal(vic, obmol);

      FOR_ATOMS_OF_MOL(atom, &obmol)
      {
        a = vic[atom->GetIdx()]->_a;
        b = vic[atom->GetIdx()]->_b;
        c = vic[atom->GetIdx()]->_c;
        r = vic[atom->GetIdx()]->_dst;
        w = vic[atom->GetIdx()]->_ang;
        if (w < 0.0)
          w += 360.0;
        t = vic[atom->GetIdx()]->_tor;
        if (t < 0.0)
          t += 360.0;

        mol << qSetFieldWidth(4) << right
            << QString(etab.GetSymbol(atom->GetAtomicNum())
                       + QString::number(atom->GetIdx()));
        if (atom->GetIdx() > 1)
          mol << qSetFieldWidth(6) << right
              << QString(etab.GetSymbol(a->GetAtomicNum())
                         + QString::number(a->GetIdx())) << qSetFieldWidth(15)
              << qSetRealNumberPrecision(5) << forcepoint << fixed << right << r;
        if (atom->GetIdx() > 2)
          mol << qSetFieldWidth(6) << right
                 << QString(etab.GetSymbol(b->GetAtomicNum())
                         + QString::number(b->GetIdx())) << qSetFieldWidth(15)
              << qSetRealNumberPrecision(5) << forcepoint << fixed << right << w;
        if (atom->GetIdx() > 3)
          mol << qSetFieldWidth(6) << right
              << QString(etab.GetSymbol(c->GetAtomicNum())
                         + QString::number(c->GetIdx())) << qSetFieldWidth(15)
              << qSetRealNumberPrecision(5) << forcepoint << fixed << right << t;
        mol << qSetFieldWidth(0) << "\n";
      }
      foreach (OpenBabel::OBInternalCoord *c, vic)
        delete c;
    }

   return buffer.trimmed();
}


} // end namespace Qui
