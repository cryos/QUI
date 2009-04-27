#ifndef QUI_REMSECTION_H
#define QUI_REMSECTION_H

/*!
 *  \class RemSection
 *
 *  \brief A KeywordSection class representing a $rem block
 *   
 *  \author Andrew Gilbert
 *  \date January 2008
 */

#include <map>
#include <set>
#include <QString>
#include "KeywordSection.h" 


namespace Qui {

class RemSection : public KeywordSection {
   public:
      RemSection() : KeywordSection("rem") { init(); }

      void read(QString const& data);
      RemSection* clone() const;

      void printOption(QString const& option, bool print);
      bool printOption(QString const& option);

      QString getOption(QString const& name);

      static void printAdHoc();

      std::map<QString,QString> getOptions();
      
      static void addAdHoc(QString const& rem, QString const& v1, QString const& v2);
      void setOption(QString const& name, QString const& value) { 
           m_options[name] = value; 
      }


   protected:
      QString dump();


   private:
      // ---------- Data ---------
      static std::map<QString,QString> m_adHoc;

      std::map<QString,QString> m_options;
      std::set<QString> m_toPrint;

      // ---------- Member Functions ---------
      void init();
      bool fixOptionForQui(QString& name, QString& value);
      bool fixOptionForQChem(QString& name, QString& value);

      bool printOption(QString const& option) const {
         return m_toPrint.count(option) > 0;
      }

      void setOptions(std::map<QString,QString> const& options);
      void setPrint(std::set<QString> const& toPrint);
};

} // end namespace Qui
#endif
