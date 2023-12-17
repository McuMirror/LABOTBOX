/*! \file CLidar.h
 * A brief file description.
 * A more elaborated file description.
 */
#ifndef _CPLUGIN_MODULE_Lidar_H_
#define _CPLUGIN_MODULE_Lidar_H_

#include <QMainWindow>
#include <QTimer>

#include "CPluginModule.h"
#include "ui_ihm_Lidar.h"

#include "sick_tim561.h"
#include "lidar_data.h"


 class Cihm_Lidar : public QMainWindow
{
    Q_OBJECT
public:
    Cihm_Lidar(QWidget *parent = 0)  : QMainWindow(parent) { ui.setupUi(this); }
    ~Cihm_Lidar() { }

    Ui::ihm_Lidar ui;

    CApplication *m_application;
 };



 /*! \addtogroup Lidar
   * 
   *  @{
   */

	   
/*! @brief class CLidar
 */	   
class CLidar : public CPluginModule
{
    Q_OBJECT
#define     VERSION_Lidar   "1.0"
#define     AUTEUR_Lidar    "Nico"
#define     INFO_Lidar      "Gestion du LIDAR"

public:
    CLidar(const char *plugin_name);
    ~CLidar();

    Cihm_Lidar *getIHM(void) { return(&m_ihm); }

    virtual void init(CApplication *application);
    virtual void close(void);
    virtual bool hasGUI(void)           { return(true); }
    virtual QIcon getIcon(void)         { return(QIcon(":/icons/edit_add.png")); }  // Précise l'icône qui représente le module
    virtual QString getMenuName(void)   { return("PluginModule"); }                 // Précise le nom du menu de la fenêtre principale dans lequel le module apparaît

private:
    Cihm_Lidar m_ihm;

    SickTIM651 m_lidar;
    QTimer m_read_timer;
    QTimer m_timer_test;

    void init_polar_qcustomplot();
    void init_linear_qcustomplot();

    QCPPolarGraph *m_polar_graph;
    QCPPolarAxisAngular *m_angular_axis;

    bool m_logger_active;
    bool m_first_log;
    QFile m_logger_file;
    const QString CSV_SEPARATOR = ";";


    void log_data(CLidarData &data);

private slots :
    void onRightClicGUI(QPoint pos);

    void lidar_connected();
    void lidar_disconnected();
    void read_sick();
    void on_changed_read_period(int period);

    void on_changed_zoom_distance(int zoom_mm);
signals :
    //void signal_data(CLidarData data);
    void test();

public slots :
    void refresh_graph(CLidarData &data);

    void open_sick();

    void logger_start(QString pathfilename);
    void logger_start();
    void logger_stop();
    void logger_select_file();

    // Gestion du rejeu de trace
    void on_PB_choixTrace_clicked(void);

};

#endif // _CPLUGIN_MODULE_Lidar_H_

/*! @} */

