/*! \file CImageProcessing.h
 * A brief file description.
 * A more elaborated file description.
 */
#ifndef _CPLUGIN_MODULE_ImageProcessing_H_
#define _CPLUGIN_MODULE_ImageProcessing_H_

#include <QMainWindow>
#include <QThread>
#include <QCameraInfo>

#include "CPluginModule.h"
#include "ui_ihm_ImageProcessing.h"

#include "video_thread.h"

 class Cihm_ImageProcessing : public QMainWindow
{
    Q_OBJECT
public:
    Cihm_ImageProcessing(QWidget *parent = 0)  : QMainWindow(parent) { ui.setupUi(this); }
    ~Cihm_ImageProcessing() { }

    Ui::ihm_ImageProcessing ui;

    CApplication *m_application;
 };



 /*! \addtogroup ImageProcessing
   * 
   *  @{
   */

	   
class VideoWorker;
/*! @brief class CImageProcessing
 */	   
class CImageProcessing : public CPluginModule
{
    Q_OBJECT
#define     VERSION_ImageProcessing   "1.0"
#define     AUTEUR_ImageProcessing    "Laguiche"
#define     INFO_ImageProcessing      "Traitement d'images OpenCV"

public:
    CImageProcessing(const char *plugin_name);
    ~CImageProcessing();

    Cihm_ImageProcessing *getIHM(void) { return(&m_ihm); }

    virtual void init(CApplication *application);
    virtual void close(void);
    virtual bool hasGUI(void)           { return(true); }
    virtual QIcon getIcon(void)         { return(QIcon(":/icons/edit_add.png")); }  // Précise l'icône qui représente le module
    virtual QString getMenuName(void)   { return("PluginModule"); }                 // Précise le nom du menu de la fenêtre principale dans lequel le module apparaît

private:
    Cihm_ImageProcessing m_ihm;
    QList<QCameraInfo> m_cameras;
    QThread m_video_worker_thread;
    VideoWorker *m_video_worker;

    void refresh_camera_list();

private slots :
    void onRightClicGUI(QPoint pos);

     void video_worker_init(QString video_source);

public slots:
     void videoHandleResults(tVideoResult result);
     void videoThreadStopped();
     void videoWorkStarted();
     void videoWorkFinished();

     void startVideoWork();
     void stopVideoWork();

     void initVideoThread();
     void killVideoThread();

     void activeDebug(bool on_off);

signals:
    void operate(tVideoInput param);
    void stopWork();
};

#endif // _CPLUGIN_MODULE_ImageProcessing_H_

/*! @} */

