/*! \file CModule.h
 * A brief Classe de base pour tous les BasicModules.
 * A more elaborated file description.
 */
#ifndef _CMODULE_H_
#define _CMODULE_H_

#include <QObject>
#include <QString>
#include <QIcon>
#include <QColor>
#include <QColorDialog>
#include <QWidget>
#include <QFileInfo>


#include "CLaBotBox.h"

 /*! \addtogroup Module
   * 
   *  @{
   */
	   
/*! @brief classe de base pour tous les modules (CBasicModule / CPluginModule).
 */	   
class CModule : public QObject
{
    Q_OBJECT

public:
#define DEFAULT_MODULE_COLOR QColor(240, 240, 240)

    CModule()
        :   m_niveau_trace(0), // active uniquement la remont�e des messages d'erreurs par d�faut
            m_application(NULL)

    { }

    CModule(const char *name, const char *version, const char *auteur, const char *description)
        :   m_niveau_trace(0),
            m_GUI(NULL),
            m_background_color(DEFAULT_MODULE_COLOR),
            m_application(NULL)
    {
        m_name          = QString(name);
        m_version       = QString(version);
        m_auteur        = QString(auteur);
        m_description   = QString(description);
    }

    // _____________________________________________________________________
    /*!
    *  M�thode appel�e pour initialiser les ressources du basic module
    */
    virtual void init(CLaBotBox *application) { m_application = application; }

    // _____________________________________________________________________
    /*!
    *  M�thode appel�e avant la fin de l'application pour lib�rer les ressources du basic module
    */
    virtual void close(void) { }
    // _____________________________________________________________________
    /*!
    *  Indique si le module poss�de une IHM
    */
    virtual bool hasGUI(void) { return(false); }
    // _____________________________________________________________________
    /*!
    *  Indique si le module poss�de une documentation
    */
    virtual bool hasUserGuide(void) {
        QFileInfo fileInfo("://user_guide/" + getName() + ".html");
        return(fileInfo.exists());
    }
    // _____________________________________________________________________
    /*!
    *  R�cup�re la documentation du module si elle existe
    */
    virtual QString getUserGuide(void) {
        QString filename = "://user_guide/" + getName() + ".html";
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { return ""; }
        return file.readAll();
    }
    // _____________________________________________________________________
    /*!
    *  Renvoie l'icon qui repr�sente le module
    */
    virtual QIcon getIcon(void) { return(QIcon("")); }
    // _____________________________________________________________________
    /*!
    *  R�cup�re l'IHM
    *
    *  \return l'IHM du module s'il en poss�de une
    */
    QWidget *getGUI(void)       { return(m_GUI); }
    // _____________________________________________________________________
    /*!
    *  Indique l'IHM
    */
    void setGUI(QWidget *GUI)
    {
        if (GUI != NULL) {
            m_GUI = GUI;
            m_GUI->setWindowTitle(getName());
            m_GUI->setWindowIcon(getIcon());
        }
    }
    // _____________________________________________________________________
    /*!
    *  Indique la couleur de fond de l'IHM
    */
    void setBackgroundColor(QColor bg_color)
    {
        if (m_GUI != NULL) {
            if (bg_color.isValid()) {
                m_background_color = bg_color;
                m_GUI->setStyleSheet(QString::fromLatin1("QMainWindow > .QWidget { background-color: %1; }").arg(bg_color.name()));
            }
        }
    }
    // _____________________________________________________________________
    /*!
    *  R�cup�re la couleur de fond de l'IHM
    */
    QColor getBackgroundColor()  { return m_background_color; }
    // _____________________________________________________________________
    /*!
    *  R�cup�re le nom du menu dans lequel le module doit apparaitre sur l'IHM
    */
    virtual QString getMenuName(void)  { return(""); }

    // _____________________________________________________________________
    /*!
    *  R�cup�re le nom du basic module
    *
    *  \return le nom du BasicModule
    */
    QString getName(void) { return(m_name); }

    // _____________________________________________________________________
    /*!
    *  R�cup�re la version du basic module
    *
    *  \return la version du BasicModule
    */
    QString getVersion(void) { return(m_version); }

    // _____________________________________________________________________
    /*!
    *  R�cup�re le nom de l'auteur du basic module
    *
    *  \return le nom de l'auteur du BasicModule
    */
    QString getAuteur(void) { return(m_auteur); }

    // _____________________________________________________________________
    /*!
    *  R�cup�re les infos g�n�rales sur le basic module
    *
    *  \return le nom de l'auteur du BasicModule
    */
    QString getDescription(void) { return(m_description); }

    // _____________________________________________________________________
    /*!
    *  R�cup�re les messages autoris�s � �tre affich�s
    *
    *  \return le niveau des messages autoris�s
    */
    unsigned long getNiveauTrace(void) { return(m_niveau_trace); }
    // _____________________________________________________________________
    /*!
    *  Force le niveau de trace (types des messages autoris�s � �tre affich�s)
    *
    */
    void setNiveauTrace(unsigned long niveau) { m_niveau_trace = niveau; }
    // _____________________________________________________________________
    /*!
    *  Ajoute un (ou des) niveau de trace au niveau existants
    *
    */
    void activeNiveauTrace(unsigned long niveau) { m_niveau_trace |= niveau; }
    // _____________________________________________________________________
    /*!
    *  Ajoute un (ou des) niveau de trace au niveau existants
    *
    */
    void inhibeNiveauTrace(unsigned long niveau) { m_niveau_trace &= ~(niveau); }

private:
    //! Nom du basic module
    QString m_name;
    //! Version du basic module
    QString m_version;
    //! Nom  du d�veloppeur du basic module
    QString m_auteur;
    //! Description g�n�rale sur le module
    QString m_description;
    //! Niveau de trace remont�es (champ de bit -> 32 niveaux de trace configuraables)
    unsigned long m_niveau_trace;
    //! L'IHM du module s'il en poss�de (m�moriser l'IHM du module dans un QWidget permet d'accueillir tout type d'IHM qui en h�rite (QMainWindow, QDialog, ...)
    QWidget *m_GUI;
    //! Couleur de fond de l'IHM
    QColor m_background_color;

protected:
    CLaBotBox *m_application;

public slots :
    // _____________________________________________________________________
    /*!
    *  Indique si le module doit �tre visible ou cach� (cas des basic module avec une IHM)
    */
    virtual void setVisible(void) { if (m_GUI) { m_GUI->show(); } }
    // _____________________________________________________________________
    /*!
    *  Ouvre la popup et s�lectionne la couleur de fond de la fen�tre
    *
    */
    void selectBackgroundColor(void)
    {
      QColorDialog colordlg;
      QColor color = colordlg.getColor();
      if (color.isValid()) {
        setBackgroundColor(color);
      }
    }

};

#endif // _CMODULE_H_

/*! @} */

