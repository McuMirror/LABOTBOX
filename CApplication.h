/*! \file CApplication.h
 * A brief file description header.
 * A more elaborated file description.
 */

#ifndef _LABOTBOX_H_
#define _LABOTBOX_H_

#include <QObject>
#include <QList>

/*! \addtogroup LaBotBox
   *  Additional documentation for group DataManager
   *  @{
   */

class CDataManager;
class CMainWindow;
class CPrintView;
class CEEPROM;
class CDataView;
class CDataPlayer;
class CRS232;
class CJoystick;
class CEcran;
class CExternalControler;
//##_NEW_CLASS_BASIC_MODULE_HERE_##


class CModuleDesigner;
class CTestUnitaire;
class CDataGraph;
class CSimuBot;
class CStrategyDesigner;
class CMessagerieBot;
class CSensorElectroBot;
class CActuatorElectrobot;
class CSensorView;
class CAsserv;
class CUserGuides;
class CActuatorSequencer;
class CBotCam;
//##_NEW_CLASS_PLUGIN_MODULE_HERE_##

class CBasicModule;
class CPluginModule;
class CModule;


typedef QList <CBasicModule *> t_list_basic_modules;
typedef QList <CPluginModule *> t_list_plugin_modules;
typedef QList <CModule *> t_list_modules;


/*! @brief class CApplication in @link DataManager.
 */	   
class CApplication : public QObject
{
    Q_OBJECT

public:
    CApplication();
    ~CApplication();

    void run(void);

public:
   CMainWindow          *m_main_windows;
   CDataManager         *m_data_center;
   CPrintView         *m_print_view;
   CEEPROM              *m_eeprom;
   CDataView     *m_DataView;
   CDataGraph     *m_DataGraph;
   CDataPlayer     *m_DataPlayer;
   CRS232     *m_RS232_robot;
   CRS232     *m_RS232_cmucam;
   CJoystick     *m_Joystick;
   CUserGuides     *m_UserGuides;
   CModuleDesigner       *m_module_creator;
   CExternalControler     *m_ExternalControler;
//##_NEW_BASIC_MODULE_CLASS_POINTER_DEFINITION_##

   CTestUnitaire     *m_TestUnitaire;
   CSimuBot     *m_SimuBot;
   CStrategyDesigner     *m_StrategyDesigner;
   CMessagerieBot     *m_MessagerieBot;
   CSensorElectroBot     *m_SensorElectroBot;
   CActuatorElectrobot     *m_ActuatorElectrobot;
   CSensorView     *m_SensorView;
   CAsserv     *m_Asserv;
   CActuatorSequencer     *m_ActuatorSequencer;
   CBotCam     *m_BotCam;
   CEcran     *m_Ecran;
//##_NEW_PLUGIN_MODULE_CLASS_POINTER_DEFINITION_##


   // Liste de l'ensemble des basic modules
   t_list_basic_modules m_list_basic_modules;

   // Liste de l'ensemble des plugin modules
   t_list_plugin_modules m_list_plugin_modules;

   // Liste de l'ensemble des modules (indépendamment du fait qu'il soit un BasicModule ou un PluginModule)
   t_list_modules m_list_modules;

public :
   //! Répertoire par défaut de sortie des fichiers de log
   QString m_pathname_log_file;
   //! Répertoire par défaut des fichiers de config
   QString m_pathname_config_file;

private:
   void createBasicModules(void);
   void initBasicModules(void);
   void closeBasicModules(void);
   void deleteBasicModules(void);

   void createPluginModules(void);
   void initPluginModules(void);
   void closePluginModules(void);
   void deletePluginModules(void);

public slots :
   void TermineApplication(void);


};


#endif // _LABOTBOX_H_

/*! @} */
