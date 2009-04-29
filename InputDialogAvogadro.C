/*!
 *  \file InputDialogAvogadro.C
 *
 *  \brief This file contains extensions to the InputDialog class that are only
 *  relevant when the QUI is compiled as an Avogadro extension
 *
 *  \author Andrew Gilbert
 *  \date   November 2008
 */

#ifndef AVOGADRO
#error Macro AVOGADRO not defined in InputDialogAvogadro.C
#endif

#include <QDialog>
#include <QTextStream>
#include <QtDebug>
#include <QObject>
#include "InputDialog.h"
#include "Job.h"

#include <avogadro/extension.h>
#include <avogadro/molecule.h>

using namespace Avogadro;

namespace Qui {


void InputDialog::setMolecule(Avogadro::Molecule* molecule) {
   // Disconnect the old molecule first...
   if (m_molecule) disconnect(m_molecule, 0, this, 0);

   if (molecule) {
      m_molecule = molecule;
      // Update the preview text whenever primitives are changed
      connect(m_molecule, SIGNAL(atomRemoved(Atom *)),
           this, SLOT(updatePreviewText()));
      connect(m_molecule, SIGNAL(atomAdded(Atom *)),
           this, SLOT(updatePreviewText()));
      connect(m_molecule, SIGNAL(atomUpdated(Atom *)),
           this, SLOT(updatePreviewText()));
      updatePreviewText();
   }
}


} // end namespace Qui
