#ifndef QUI_CONDITIONS_H
#define QUI_CONDITIONS_H

/*!
 *  \file Conditions.h 
 *
 *  \brief declarations for custom Condition functions.
 *   
 *  \author Andrew Gilbert
 *  \date August 2008
 */


namespace Qui {

bool isCompoundFunctional();
bool isPostHF();
bool isDFT();
bool requiresDerivatives();
bool requiresFirstDerivatives();
bool requiresSecondDerivatives();

} // end namespace Qui
#endif
