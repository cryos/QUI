/*!
 *  \file KeywordSection.C 
 *
 *  \brief Non-inline member functions of the KeywordSection class, see
 *  KeywordSection.h for details.
 *   
 *  \author Andrew Gilbert
 *  \date January 2008
 */

#include "RemSection.h"
#include "OptSection.h"
#include "MoleculeSection.h"
#include "ExternalChargesSection.h"


namespace Qui {

// Factory - we use this to generate a derived KeywordSection based on its
// name.  Note that if you add additional specialized KeywordSections, this
// Factory needs to be made aware of them.
KeywordSection* KeywordSectionFactory(QString const& type) {
   QString t(type.toLower());

   if (t == "molecule") {
      return new MoleculeSection();
   }else if (t == "rem") {
      return new RemSection();
   }else if (t == "opt") {
      return new OptSection();
   }else if (t == "external_charges") {
      return new ExternalChargesSection();
   }else {
      return new GenericSection(t);
   }
}


QString KeywordSection::format() {
   return m_print ? dump() : QString();
}



// ---------- GenericSection ----------

// This a fallback section that simply holds the data in a string.  It is
// useful when no other formating or processing is required.

QString GenericSection::dump() {
   QString s;
   s += "$" + name() + "\n";
   if (!m_data.isEmpty()) s += m_data + "\n";
   s += "$end\n";
   return s;
}

QString GenericSection::rawData() {
   return m_data;
}

void GenericSection::read(QString const& data) {
    m_data = data.trimmed();
}


GenericSection* GenericSection::clone() const {
   return new GenericSection(name(), m_data, m_print);
}


} // end namespace Qui
