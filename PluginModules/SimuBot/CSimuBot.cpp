﻿/*! \file CSimuBot.cpp
 * A brief file description CPP.
 * A more elaborated file description.
 */
#include <QDebug>
#include "CSimuBot.h"
#include "CApplication.h"
#include "CPrintView.h"
#include "CMainWindow.h"
#include "CEEPROM.h"
#include "CDataManager.h"
#include "CAStar.h"

/*!
 * \brief normalizeAngleDeg utilitaire de normalisation d'angle en degré
 * \param angleRaw
 * \return
 */
static qreal normalizeAngleDeg(qreal angleRaw)
{
    //angleRaw=(-1.0)*angleRaw;
    while (angleRaw < -180)
        angleRaw += 360;
    while (angleRaw > 180)
        angleRaw -= 360;
    //angleRaw=(angleRaw/360.0)*TwoPi;
    return angleRaw;
}

/*! \addtogroup PluginModule_Test2
   * 
   *  @{
   */
	   
// _____________________________________________________________________
/*!
*  Constructeur
*
*/
CSimuBot::CSimuBot(const char *plugin_name)
    :CPluginModule(plugin_name, VERSION_SimuBot, AUTEUR_SimuBot, INFO_SimuBot)
{
    //on initialise l'environnement
    terrain=new GraphicEnvironnement;

    //On positionne par défaut l'affichage en radian
    //attention cette valeur est écrasée par celle du fichier eeprom si elle est renseignée
    setAndGetInRad=true;

    //init des variables de positionnement relatif du robot
    deltaAngle=0;
    deltaDistance=0;
}


// _____________________________________________________________________
/*!
*  Destructeur
*
*/
CSimuBot::~CSimuBot()
{

}


// _____________________________________________________________________
/*!
*  Initialisation du module
*   Lecture des valeurs de l'eeprom (contour du robot,
*
*/
void CSimuBot::init(CApplication *application)
{
    CPluginModule::init(application);
    setGUI(&m_ihm); // indique à la classe de base l'IHM
    setNiveauTrace(MSG_TOUS);

    // Gère les actions sur clic droit sur le panel graphique du module
    m_ihm.setContextMenuPolicy(Qt::CustomContextMenu);
    connect(&m_ihm, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onRightClicGUI(QPoint)));

    // Restore la taille de la fenêtre
    QVariant val;
    val = m_application->m_eeprom->read(getName(), "geometry", QRect(50, 50, 150, 150));
    m_ihm.setGeometry(val.toRect());

    // Restore le fait que la fenêtre est visible ou non
    val = m_application->m_eeprom->read(getName(), "visible", QVariant(true));
    if (val.toBool()) { m_ihm.show(); }
    else              { m_ihm.hide(); }
    // Restore le niveau d'affichage
    val = m_application->m_eeprom->read(getName(), "niveau_trace", QVariant(MSG_TOUS));
    setNiveauTrace(val.toUInt());
    // Restore la couleur de fond
    val = m_application->m_eeprom->read(getName(), "background_color", QVariant(DEFAULT_MODULE_COLOR));
    setBackgroundColor(val.value<QColor>());

    //récupération des contours du robot dans le fichier d'eeprom, un point est décrit par une chaine de caractères
    //formatée comme suit "(doublexdouble)", un contour existe par défaut
    val=m_application->m_eeprom->read(getName(),"polygon",QStringList());
    QStringList listePointsPolygon=val.toStringList();
    QPolygonF polygonIni;
    if(listePointsPolygon.isEmpty()){
      polygonIni << QPointF(10,-15) << QPointF(-10,-15)<< QPointF(-15,-10)<< QPointF(-15,10) << QPointF(-10,15);
      polygonIni << QPointF(10,15) << QPointF(15,10) << QPointF(15,-10) << QPointF(10,-15);
    }
    else{
      QString unStringPoint, temp, xString, yString;
      QStringList ListXY;
      double xFromString, yFromString;
      for(int ind=0;ind<listePointsPolygon.size();ind++)
      {
          unStringPoint=listePointsPolygon.at(ind);
          //qDebug() << unStringPoint;
          ListXY=unStringPoint.split('x');
          if (ListXY.size()>=2){
              temp=ListXY.at(0);
              xString=temp.remove(QChar('('), Qt::CaseInsensitive);
              xFromString=xString.toDouble();
              temp=ListXY.at(1);
              yString=temp.remove(QChar(')'), Qt::CaseInsensitive);
              yFromString=(-1.0)*yString.toDouble();
              polygonIni << QPointF(xFromString,yFromString);
          }
       }
    }

    //récupération des positions d'init
    /*float X_init_1=24;
    float Y_init_1=101;
    float Theta_init_1=0;
    float X_init_2=265;
    float Y_init_2=101;
    float Theta_init_2=180;*/
    val = m_application->m_eeprom->read(getName(), "X_init_1", QVariant(5.0));
    float X_init_1=val.toFloat();
    val = m_application->m_eeprom->read(getName(), "Y_init_1", QVariant(123.0));
    float Y_init_1=val.toFloat();
    val = m_application->m_eeprom->read(getName(), "Theta_init_1", QVariant(0.0));
    float Theta_init_1=val.toFloat();
    val = m_application->m_eeprom->read(getName(), "X_init_2", QVariant(265.0));
    float X_init_2=val.toFloat();
    val = m_application->m_eeprom->read(getName(), "Y_init_2", QVariant(101.0));
    float Y_init_2=val.toFloat();
    val = m_application->m_eeprom->read(getName(), "Theta_init_2", QVariant(180.0));
    float Theta_init_2=val.toFloat();
    equipe1.init(X_init_1,Y_init_1,Theta_init_1,true);
    equipe2.init(X_init_2,Y_init_2,Theta_init_2,false);

    //ajout des limites physiques du terrain
    QGraphicsPixmapItem *surface=new QGraphicsPixmapItem();
    surface->setPixmap(QPixmap(":/icons/terrain_2020_simubot.png"));
    surface->setPos(0,-200);
    QGraphicsRectItem *bordures=new QGraphicsRectItem(QRect(0, -200 , 300, 200));
    terrain->addItem(surface);
    terrain->addItem(bordures);

/*    QGraphicsEllipseItem *modules[14];
    modules[0]=new QGraphicsEllipseItem(QRectF(115-3,-200,6,6));
    modules[0]->setBrush(QBrush(QColor(0, 0,255, 255))); //blue
    modules[1]=new QGraphicsEllipseItem(QRectF(185-3,-200,6,6));
    modules[1]->setBrush(QBrush(QColor(255, 255,0, 255))); //yellow
    modules[2]=new QGraphicsEllipseItem(QRectF(20-3,-140,6,6));
    modules[2]->setBrush(QBrush(QColor(0, 0,255, 255))); //blue
    modules[3]=new QGraphicsEllipseItem(QRectF(100-3,-140,6,6));
    modules[4]=new QGraphicsEllipseItem(QRectF(200-3,-140,6,6));
    modules[5]=new QGraphicsEllipseItem(QRectF(280-3,-140,6,6));
    modules[5]->setBrush(QBrush(QColor(255, 255,0, 255))); //yellow
    modules[6]=new QGraphicsEllipseItem(QRectF(50-3,-90,6,6));
    modules[7]=new QGraphicsEllipseItem(QRectF(250-3,-90,6,6));
    modules[8]=new QGraphicsEllipseItem(QRectF(0,-65,6,6));
    modules[8]->setBrush(QBrush(QColor(0, 0,255, 255))); //blue
    modules[9]=new QGraphicsEllipseItem(QRectF(300-6,-65,6,6));
    modules[9]->setBrush(QBrush(QColor(255, 255,0, 255))); //yellow
    modules[10]=new QGraphicsEllipseItem(QRectF(90-3,-60,6,6));
    modules[11]=new QGraphicsEllipseItem(QRectF(210-3,-60,6,6));
    modules[12]=new QGraphicsEllipseItem(QRectF(80-3,-15,6,6));
    modules[12]->setBrush(QBrush(QColor(0, 0,255, 255))); //blue
    modules[13]=new QGraphicsEllipseItem(QRectF(220-3,-15,6,6));
    modules[13]->setBrush(QBrush(QColor(255, 255,0, 255))); //yellow
    for(int ind=0;ind<14;ind++){
        //modules[ind]->setBrush(QBrush(QColor(255, 0,0, 255)));
        terrain->addItem(modules[ind]);
    }*/
    /*QGraphicsEllipseItem *gobelet[3];
    gobelet[0]=new QGraphicsEllipseItem(QRectF(150-4.75,165-202-4.75,9.5,9.5));
    gobelet[1]=new QGraphicsEllipseItem(QRectF(25-4.75,175-202-4.75,9.5,9.5));
    gobelet[2]=new QGraphicsEllipseItem(QRectF(91-4.75,80-202-4.75,9.5,9.5));

    for(int ind1=0;ind1<3;ind1++){
        gobelet[ind1]->setBrush(QBrush(QColor(255, 255,255, 255)));
        terrain->addItem(gobelet[ind1]);
    }*/


    //ajout du robot
    GrosBot=new GraphicElement(polygonIni,255,255,255);
    GrosBot->setBrush(QBrush(QColor(255, 255,255, 255)));

    //ajout du point de référence du robot
    QPolygonF ref_GrosBotForme;
    ref_GrosBotForme << QPointF(5,-7) << QPointF(-5,-7)<< QPointF(-7,-5)<< QPointF(-7,5) << QPointF(-5,7);
    ref_GrosBotForme << QPointF(5,7) << QPointF(7,5) << QPointF(7,-5) << QPointF(5,-7);
    OldGrosBot= new GraphicElement(ref_GrosBotForme,255,255,255);
    OldGrosBot->setFlag(QGraphicsItem::ItemIsMovable, false);
    OldGrosBot->setFlag(QGraphicsItem::ItemIsSelectable, false);
    OldGrosBot->setBrush(QBrush(QColor(255,255,255, 100)));

    //on lie les deux points
    QLineF liaison_Line(GrosBot->getX(),GrosBot->getY(),OldGrosBot->getX(),OldGrosBot->getY());
    liaison_GrosBot= new QGraphicsLineItem(liaison_Line);
	
	//ajout d'un robot adverse
    QPolygonF ref_BotForme;
    ref_BotForme << QPointF(9,-15) << QPointF(-9,-15)<< QPointF(-15,-9)<< QPointF(-15,9) << QPointF(-9,15);
    ref_BotForme << QPointF(9,15) << QPointF(15,9) << QPointF(15,-9) << QPointF(9,-15);
    OtherBot= new GraphicElement(ref_BotForme,255,255,255);
    OtherBot->setFlag(QGraphicsItem::ItemIsMovable, true);
    OtherBot->setFlag(QGraphicsItem::ItemIsSelectable, true);
    OtherBot->setBrush(QBrush(QColor(255,0,0, 100)));

    //on place le robot et on crée le mécanisme d'init
    QPushButton *pushButton_init=m_ihm.findChild<QPushButton*>("pushButton_init");
    connect(pushButton_init,SIGNAL(clicked()),this,SLOT(initView()));
    QRadioButton *radioButton_couleur_1=m_ihm.findChild<QRadioButton*>("radioButton_couleur_1");
    QRadioButton *radioButton_couleur_2=m_ihm.findChild<QRadioButton*>("radioButton_couleur_2");
    connect(radioButton_couleur_1,SIGNAL(clicked()),this,SLOT(changeEquipe()));
    connect(radioButton_couleur_2,SIGNAL(clicked()),this,SLOT(changeEquipe()));

    //ecrasement des données
    connect(m_ihm.ui.lineEdit_x,SIGNAL(editingFinished()),this,SLOT(returnCapture_XY()));
    connect(m_ihm.ui.lineEdit_y,SIGNAL(editingFinished()),this,SLOT(returnCapture_XY()));
    connect(m_ihm.ui.lineEdit_theta,SIGNAL(editingFinished()),this,SLOT(returnCapture_Theta()));
    connect(m_ihm.ui.dial_rotation_bot,SIGNAL(sliderReleased()),this,SLOT(slot_dial_turned()));
    //connect(m_ihm.ui.dial_rotation_bot,SIGNAL(),this,SLOT(slot_dial_turned()));
    connect(this, SIGNAL(displayCoord(qreal,qreal)), GrosBot,SLOT(display_XY(qreal,qreal)));
    connect(this,SIGNAL(displayAngle(qreal)),GrosBot,SLOT(display_theta(qreal)));


    //pour changer de mode visu ou placement
    //TODO: profiter de l'interface de simubot pour profiter d'un vrai simulateur
    QSlider *horizontalSlider_toggle_simu=m_ihm.findChild<QSlider*>("horizontalSlider_toggle_simu");
    modeVisu=horizontalSlider_toggle_simu->value();
    connect(horizontalSlider_toggle_simu,SIGNAL(valueChanged(int)),this,SLOT(changeMode(int)));

    //on initialise et ajoute le robot au terrain
    //TODO: prendre un fichier de config pour l'emplacement et l'angle de départ pour le robot
    // pour l'instant c'est en dur dans le constructeur
    connect(terrain, SIGNAL(changed(QList<QRectF>)), this, SLOT(viewChanged(QList<QRectF>)));
    connect(GrosBot,SIGNAL(center(qreal,qreal,float)),OldGrosBot,SLOT(replace(qreal,qreal,float)));
    connect(GrosBot,SIGNAL(isDoubleClicked()),this,SLOT(catchDoubleClick()));
	
	
    terrain->addItem(OldGrosBot);
    terrain->addItem(GrosBot);
    terrain->addItem(liaison_GrosBot);
	terrain->addItem(OtherBot);

    //TODO: ajouter nouveaux element comme un MiniBot, un adversaire avec un trajet aléatoire,...
    //ajout des éléments de jeu
//    QList<QGraphicsEllipseItem*> listSpots;
//    listSpots << &QGraphicsEllipseItem(QRect(100, 100 , 6, 6));
//    //QGraphicsEllipseItem * =new QGraphicsRectItem(QRect(0, -202 , 302, 202));
//    terrain->addItem((listSpots.at(0)));


    //Mise en place du terrain
    m_ihm.simuView=m_ihm.findChild<QGraphicsView*>("simuGraphicsView");
    m_ihm.simuView->setRenderHint(QPainter::Antialiasing);
    m_ihm.simuView->centerOn(QPointF(163,118));
    m_ihm.simuView->setCacheMode(QGraphicsView::CacheBackground);
    m_ihm.simuView->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    m_ihm.simuView->setDragMode(QGraphicsView::ScrollHandDrag);
    m_ihm.simuView->resize(326, 236);
    m_ihm.simuView->setScene(terrain);
    connect(m_ihm.ui.verticalSlider_zoom_scene,SIGNAL(valueChanged(int)),this,SLOT(zoom(int)));
    //m_ihm.simuView->scale(2.0,2.0);

    //pour le mode visu on se connecte aux changements du datamanager
    connect(m_application->m_data_center,SIGNAL(valueChanged(CData*)),this,SLOT(coordChanged(CData*)));

    //changement de repère suivant le choix de coordonnées relatives au terrain ou au robot
    // Restore le fait que la fenêtre est visible ou non
    val = m_application->m_eeprom->read(getName(), "isRelativToBot", QVariant(true));
    QRadioButton *radioButton_robot_relative=m_ihm.findChild<QRadioButton*>("radioButton_robot_relative");
    QRadioButton *radioButton_terrain_relative=m_ihm.findChild<QRadioButton*>("radioButton_terrain_relative");
    if (val.toBool()) {radioButton_robot_relative->setChecked(true); }
    else              {radioButton_terrain_relative->setChecked(true); }

    GrosBot->isRelativToBot=val.toBool();
    OldGrosBot->isRelativToBot=val.toBool();
    OtherBot->isRelativToBot=val.toBool();

    //affichage et saisi en radian ou en degré
    //setAndGetInRad
    val = m_application->m_eeprom->read(getName(), "anglesEnRadian", QVariant(true));
    QRadioButton *radioButton_radian=m_ihm.findChild<QRadioButton*>("radioButton_radian");
    QRadioButton *radioButton_degre=m_ihm.findChild<QRadioButton*>("radioButton_degre");
    if (val.toBool()) {radioButton_radian->setChecked(true); setAndGetInRad=true; }
    else              {radioButton_degre->setChecked(true); setAndGetInRad=false; }


    //pour calculer une trajectoire d'evitement
    connect(m_ihm.ui.pb_Astar,SIGNAL(clicked()),this,SLOT(slot_getPath()));

    // Positions x, y, teta du robot physique
    m_application->m_data_center->write("x_pos", 0);
    m_application->m_data_center->write("y_pos", 0);
    m_application->m_data_center->write("teta_pos", 0);
    connect(m_application->m_data_center->getData("x_pos"), SIGNAL(valueChanged(QVariant)), this, SLOT(real_robot_position_changed()));
    connect(m_application->m_data_center->getData("y_pos"), SIGNAL(valueChanged(QVariant)), this, SLOT(real_robot_position_changed()));
    connect(m_application->m_data_center->getData("teta_pos"), SIGNAL(valueChanged(QVariant)), this, SLOT(real_robot_position_changed()));


    //init de ces variables dans le data manager
    m_application->m_data_center->write("Cde_MotG", 0);
    m_application->m_data_center->write("Cde_MotD", 0);
    m_application->m_data_center->write("Simubot.Telemetres.AVG", 99);
    m_application->m_data_center->write("Simubot.Telemetres.AVD", 99);
    m_application->m_data_center->write("Simubot.Telemetres.ARG", 99);
    m_application->m_data_center->write("Simubot.Telemetres.ARD", 99);
    m_application->m_data_center->write("Simubot.OtherBot.x", 0);
    m_application->m_data_center->write("Simubot.OtherBot.y", 0);
    m_application->m_data_center->write("Simubot.Bot.x", 0);
    m_application->m_data_center->write("Simubot.Bot.y", 0);
    m_application->m_data_center->write("Simubot.Bot.teta", 0);
    //pour la simu
    cadenceur=new QTimer();
    connect(m_ihm.ui.pB_US,SIGNAL(clicked(bool)),this,SLOT(estimate_Environment_Interactions()));
    connect(m_ihm.ui.pB_playOther,SIGNAL(clicked(bool)),this,SLOT(playOther()));
    connect(m_ihm.ui.pB_stopOther,SIGNAL(clicked(bool)),this,SLOT(stopOther()));
    connect(cadenceur, SIGNAL(timeout()), this, SLOT(nextStepOther()));
    connect(m_ihm.ui.chkBox_enableMoveOther, SIGNAL(stateChanged(int)), this, SLOT(enableMoveOther(int)));
    QStringList QS_Labels;
    QS_Labels << "x" << "y" << "teta";
    m_ihm.ui.tableWidget->setHorizontalHeaderLabels(QS_Labels);
    addLineOther(0,-50,0,0);
    addLineOther(200,-50,0,1);
    addLineOther(200,70,0,2);
    addLineOther(0,70,0,3);
    addLineOther(0,0,0,4);


    //positionnement par défaut
    initEquipe(equipe1, equipe2);
    initView();
}


// _____________________________________________________________________
/*!
*  Fermeture du module
*
*/
void CSimuBot::close(void)
{
  // Mémorise en EEPROM l'état de la fenêtre
  m_application->m_eeprom->write(getName(), "geometry", QVariant(m_ihm.geometry()));
  m_application->m_eeprom->write(getName(), "visible", QVariant(m_ihm.isVisible()));
  m_application->m_eeprom->write(getName(), "background_color", QVariant(getBackgroundColor()));
  m_application->m_eeprom->write(getName(), "niveau_trace", QVariant((unsigned int)getNiveauTrace()));
}

// _____________________________________________________________________
/*!
*  Création des menus sur clic droit sur la fenêtre du module
*
*/
void CSimuBot::onRightClicGUI(QPoint pos)
{
  QMenu *menu = new QMenu();

  menu->addAction("Select background color", this, SLOT(selectBackgroundColor()));
  menu->exec(m_ihm.mapToGlobal(pos));
}


/*!
 * \brief CSimuBot::viewChanged on met à jour l'IHM quand la scene graphique a été modifiée
 * \param regions
 *
 * TODO: relier à un simulateur de robot pour donner les interactions avec l'environnement
 */
void CSimuBot::viewChanged(QList<QRectF> regions)
{
  Q_UNUSED(regions)
    //récupération des différentes coordonnées (graphique et réelles)
    qreal x_view=GrosBot->getX();
    qreal y_view=GrosBot->getY();
    qreal x_graphic=GrosBot->x();
    qreal y_graphic=GrosBot->y();
    qreal theta_view=GrosBot->getTheta();

    //récupération des différentes coordonnées (graphique et réelles) pour l'ancienne position du robot
    qreal x_prim_view=OldGrosBot->getX();
    qreal y_prim_view=OldGrosBot->getY();
    qreal x_prim_graphic=OldGrosBot->x();
    qreal y_prim_graphic=OldGrosBot->y();

    //on redessine la ligne entre l'ancienne et la nouvelle position
    liaison_GrosBot->setLine(x_graphic,y_graphic,x_prim_graphic,y_prim_graphic);

    //on calcule la distance entre l'ancienne et la nouvelle position
    deltaDistance= sqrt(pow((x_graphic-x_prim_graphic),2)+pow((y_graphic-y_prim_graphic),2));

    //on calcule de l'angle par rapport à l'ancienne position
    if (x_view==x_prim_view)
    {
        if (y_view>y_prim_view)
            deltaAngle=Pi/2;
        else if (y_view==y_prim_view)
            deltaAngle=0;
        else if (y_view<y_prim_view)
            deltaAngle=-Pi/2;
    }
    else if (x_view>x_prim_view)
        deltaAngle=atan((y_view-y_prim_view)/(x_view-x_prim_view));
    else if (x_view<x_prim_view)
    {
        if (y_view>y_prim_view)
            deltaAngle=atan((y_view-y_prim_view)/(x_view-x_prim_view))+Pi;
        else if (y_view==y_prim_view)
            deltaAngle=Pi;
        else if (y_view<y_prim_view)
            deltaAngle=atan((y_view-y_prim_view)/(x_view-x_prim_view))-Pi;
    }

    //Affichage des nouvelles valeurs position, angle du robot
    if (setAndGetInRad)
        m_ihm.ui.lcdNumber_theta->display(theta_view);
    else
        m_ihm.ui.lcdNumber_theta->display(normalizeAngleDeg(180*theta_view/Pi));
    m_ihm.ui.lcdNumber_x->display(x_view);
    m_ihm.ui.lcdNumber_y->display(y_view);
    m_ihm.ui.lcdNumber_x_terrain->display(GrosBot->getX_terrain());
    m_ihm.ui.lcdNumber_y_terrain->display(GrosBot->getY_terrain());

    //Si on est en mode TEST, on affiche les valeurs complémentaires vis à vis de l'ancienne position du robot
    //et on met à jour le data manager
    switch(modeVisu)
    {
    case SIMUBOT::TEST :
        m_ihm.ui.lcdNumber_distance->display(deltaDistance);
        if (setAndGetInRad){
            m_ihm.ui.lcdNumber_angle->display(deltaAngle);
            m_ihm.ui.lineEdit_theta->setValue(theta_view);
            m_application->m_data_center->write("PosTeta_robot", theta_view);
            m_application->m_data_center->write("DirAngle_robot", deltaAngle);
        }
        else{
            m_ihm.ui.lcdNumber_angle->display(normalizeAngleDeg(180*deltaAngle/Pi));
            m_ihm.ui.lineEdit_theta->setValue(normalizeAngleDeg(180*theta_view/Pi));
            m_application->m_data_center->write("PosTeta_robot", normalizeAngleDeg(180*theta_view/Pi));
            m_application->m_data_center->write("DirAngle_robot", normalizeAngleDeg(180*deltaAngle/Pi));
        }
        m_ihm.ui.lineEdit_x->setValue(x_view);
        m_ihm.ui.lineEdit_y->setValue(y_view);
        //m_ihm.ui.lineEdit_theta->setValue(theta_view);

        // Informe le DataCenter de la mise à jour
        m_application->m_data_center->write("PosX_robot", x_view);
        m_application->m_data_center->write("PosY_robot", y_view);
        m_application->m_data_center->write("DirDistance_robot", deltaDistance);
        m_application->m_data_center->write("Simubot.Bot.x", GrosBot->getX_terrain());
        m_application->m_data_center->write("Simubot.Bot.y", GrosBot->getY_terrain());
        m_application->m_data_center->write("Simubot.Bot.teta", GrosBot->getTheta());
        m_application->m_data_center->write("Simubot.OtherBot.x", OtherBot->getX_terrain());
        m_application->m_data_center->write("Simubot.OtherBot.y", OtherBot->getY_terrain());
        break;
    case SIMUBOT::VISU :
        break;
    case SIMUBOT::SIMU:
        //TODO SIMU: donner les interactions avec l'environnement au datamanger
        estimate_Environment_Interactions();
        /*m_application->m_data_center->write("Simubot.Bot.x", GrosBot->x());
        m_application->m_data_center->write("Simubot.Bot.y", GrosBot->y());
        m_application->m_data_center->write("Simubot.Bot.teta", GrosBot->getTheta());
        m_application->m_data_center->write("Simubot.OtherBot.x", OtherBot->x());
        m_application->m_data_center->write("Simubot.OtherBot.y", OtherBot->y());*/
        m_application->m_data_center->write("Simubot.Bot.x", GrosBot->getX_terrain());
        m_application->m_data_center->write("Simubot.Bot.y", GrosBot->getY_terrain());
        m_application->m_data_center->write("Simubot.Bot.teta", GrosBot->getTheta());
        m_application->m_data_center->write("Simubot.OtherBot.x", OtherBot->getX_terrain());
        m_application->m_data_center->write("Simubot.OtherBot.y", OtherBot->getY_terrain());
        break;

    default:
        break;

    }

}

void CSimuBot::initView(void){

    if(m_ihm.ui.radioButton_robot_relative->isChecked()){
        GrosBot->isRelativToBot=true;
        OldGrosBot->isRelativToBot=true;
        OtherBot->isRelativToBot=true;
    }
    else{
        GrosBot->isRelativToBot=false;
        OldGrosBot->isRelativToBot=false;
        OtherBot->isRelativToBot=false;
    }

    if(m_ihm.ui.radioButton_radian->isChecked())
        setAndGetInRad=true;
    else
        setAndGetInRad=false;

    qreal x_init=m_ihm.ui.lineEdit_X_init->value();
    qreal y_init=m_ihm.ui.lineEdit_Y_init->value();
    qreal theta_init=m_ihm.ui.lineEdit_Theta_init->value();

    if (setAndGetInRad)
    {
        GrosBot->setAsservInit(m_ihm.ui.sB_X_init_asserv->value(),
                               m_ihm.ui.sB_Y_init_asserv->value(),
                               180*(m_ihm.ui.sB_Theta_init_asserv->value())/Pi);
        OtherBot->setAsservInit(0,0,0);
        GrosBot->raz(x_init,y_init,normalizeAngleDeg(180*theta_init/Pi));
        OldGrosBot->raz(x_init,y_init,normalizeAngleDeg(180*theta_init/Pi));
        OtherBot->raz(equipeOther.x,equipeOther.y,normalizeAngleDeg(180*equipeOther.teta/Pi));
    }
    else
    {
        GrosBot->setAsservInit(m_ihm.ui.sB_X_init_asserv->value(),
                               m_ihm.ui.sB_Y_init_asserv->value(),
                               m_ihm.ui.sB_Theta_init_asserv->value());
        OtherBot->setAsservInit(0,0,0);
        GrosBot->raz(x_init,y_init,theta_init);
        OldGrosBot->raz(x_init,y_init,theta_init);
        OtherBot->raz(equipeOther.x,equipeOther.y,equipeOther.teta);
    }

    qreal x_reel_init=GrosBot->getX();
    qreal y_reel_init=GrosBot->getY();
    qreal theta_reel_init=GrosBot->getTheta();

    m_application->m_data_center->write("PosX_robot", x_reel_init);
    m_application->m_data_center->write("PosY_robot", y_reel_init);

    if (setAndGetInRad)
        m_application->m_data_center->write("PosTeta_robot", theta_reel_init);
    else
        m_application->m_data_center->write("PosTeta_robot", normalizeAngleDeg(180*theta_reel_init/Pi));

    m_ihm.ui.lcdNumber_x_terrain->display(GrosBot->getX_terrain());
    m_ihm.ui.lcdNumber_y_terrain->display(GrosBot->getY_terrain());
}

void CSimuBot::initEquipe(Coord equipe, Coord equipe_adverse)
{
    m_ihm.ui.lineEdit_X_init->setValue(equipe.x);
    m_ihm.ui.lineEdit_Y_init->setValue(equipe.y);
    m_ihm.ui.lineEdit_Theta_init->setValue(equipe.teta);

    //sauvegarde des coordonnées d'init du robot adverse
    equipeOther.x=equipe_adverse.x;
    equipeOther.y=equipe_adverse.y;
    equipeOther.teta=equipe_adverse.teta;

    //sens trigo appliqué au robot placé à gauche sur l'image repère orthogonal
    if(equipe.ortho)
    {
        GrosBot->sensOrtho=1;
        OldGrosBot->sensOrtho=1;
        OtherBot->sensOrtho=1;
    }
    else
    {
        GrosBot->sensOrtho=-1;
        OldGrosBot->sensOrtho=-1;
        OtherBot->sensOrtho=-1;
    }
}

void CSimuBot::changeEquipe(void)
{
    QObject *radioButton_couleur=QObject::sender();
    QString name_radio_button;
    if(radioButton_couleur)
        name_radio_button=radioButton_couleur->objectName();
    else
        name_radio_button="radioButton_couleur_1";


    if(name_radio_button.compare("radioButton_couleur_1")==0) //bleu
    {
        initEquipe(equipe1,equipe2);
        //qDebug() << "bleu";
    }
    else if(name_radio_button.compare("radioButton_couleur_2")==0) //jaune
    {
        initEquipe(equipe2,equipe1);
        //qDebug() << "jaune";
    }
    initView();
}

void CSimuBot::returnCapture_Theta()
{
    double new_angle=m_ihm.ui.lineEdit_theta->value();
    if(m_ihm.ui.lineEdit_theta->hasFocus())
    {
        if (setAndGetInRad)
        {
            m_ihm.ui.dial_rotation_bot->setValue((int)(10000*new_angle/Pi));
            emit displayAngle(new_angle);
        }
        else
        {
            m_ihm.ui.dial_rotation_bot->setValue((int)(10000*new_angle/180));
            emit displayAngle(Pi*normalizeAngleDeg(new_angle)/180);
        }
    }
}

/*!
 * \brief CSimuBot::changeMode gère les modes TEST, VISU et SIMU
 * \param iMode mode choisi
 *
 * # COMPORTEMENT
 * - rend effectif le mode choisi
 * - Active/Désactive des éléments de l'IHM selon le mode (par exemple il faut rendre impossible la manipulation du robot pendant le mode VISU ou
 * il est inutile d'afficher l'ancienne position du robot en dehors du mode TEST)
 */
void CSimuBot::changeMode(int iMode)
{
    modeVisu=iMode;

    switch (modeVisu) {
    case SIMUBOT::TEST:
        GrosBot->setFlag(QGraphicsItem::ItemIsMovable, true);
        GrosBot->setFlag(QGraphicsItem::ItemIsSelectable, true);
        OldGrosBot->show();
        liaison_GrosBot->show();
        m_ihm.ui.lineEdit_x->setEnabled(true);
        m_ihm.ui.lineEdit_y->setEnabled(true);
        m_ihm.ui.lineEdit_theta->setEnabled(true);
        m_ihm.ui.checkBox_setSequence->setEnabled(true);
        break;
    case SIMUBOT::VISU: //pour l'instant même IHM pour VISU et SIMU
    case SIMUBOT::SIMU:
        GrosBot->setFlag(QGraphicsItem::ItemIsMovable, false);
        GrosBot->setFlag(QGraphicsItem::ItemIsSelectable, false);
        OldGrosBot->hide();
        liaison_GrosBot->hide();
        m_ihm.ui.lcdNumber_distance->display(0);
        m_ihm.ui.lcdNumber_angle->display(0);

        m_ihm.ui.lineEdit_x->clear();
        m_ihm.ui.lineEdit_y->clear();
        m_ihm.ui.lineEdit_theta->clear();
        m_ihm.ui.lineEdit_x->setEnabled(false);
        m_ihm.ui.lineEdit_y->setEnabled(false);
        m_ihm.ui.lineEdit_theta->setEnabled(false);
        m_ihm.ui.checkBox_setSequence->setChecked(false);
        m_ihm.ui.checkBox_setSequence->setEnabled(false);
        break;
    default:
        break;
    }
}

/*!
 * \brief CSimuBot::coordChanged appelé à chaque modification du data manager (on extrait les coordonnées robot)
 * \param data
 */
void CSimuBot::coordChanged(CData *data)
{
    // en mode VISU ou SIMU on extrait les coordonnées qui auraient changé
    if((modeVisu==SIMUBOT::VISU)||(modeVisu==SIMUBOT::SIMU))
    {
        QString dataName=data->getName();
        if(dataName.compare("PosX_robot")==0)
            GrosBot->display_XY(data->read().toDouble(),5000);
        if(dataName.compare("PosY_robot")==0)
            GrosBot->display_XY(5000,data->read().toDouble());
        if(dataName.compare("PosTeta_robot")==0)
        {
            if(setAndGetInRad)
                GrosBot->display_theta(data->read().toDouble());
            else
                GrosBot->display_theta(Pi*normalizeAngleDeg(data->read().toDouble())/180);
        }
    }
}


/*!
 * \brief CSimuBot::zoom pour zoomer ou dezoomer l'affichage du terrain
 * \param value
 */
void CSimuBot::zoom(int value)
{
    float factor=value*1.0;
    m_ihm.simuView->resetTransform();
    m_ihm.simuView->scale(factor,factor);
    m_ihm.simuView->centerOn(QPointF(151,101));
}

void CSimuBot::returnCapture_XY()
{
    QString lineEdit_coord_name=QObject::sender()->objectName();

    if (lineEdit_coord_name.compare(m_ihm.ui.lineEdit_x->objectName())==0)
        if (m_ihm.ui.lineEdit_x->hasFocus())
            emit displayCoord(m_ihm.ui.lineEdit_x->value(),5000);

    if (lineEdit_coord_name.compare(m_ihm.ui.lineEdit_y->objectName())==0)
        if (m_ihm.ui.lineEdit_y->hasFocus())
            emit displayCoord(5000,m_ihm.ui.lineEdit_y->value());
}

void CSimuBot::slot_dial_turned(void)
{
    double angle_percent=m_ihm.ui.dial_rotation_bot->value();
    double new_angle;

    if (setAndGetInRad)
    {
        new_angle=Pi*angle_percent/10000;

        emit displayAngle(new_angle);
    }
    else
    {
        new_angle=180*angle_percent/10000;
        emit displayAngle(Pi*normalizeAngleDeg(new_angle)/180);
    }
}

void CSimuBot::slot_getPath(void)
{
    /*AStar Recherche(,
                    );*/
    AStar *Recherche=new AStar();
    int xA, yA, xB, yB; //A depart, B objetcif
    int xC,yC;

    xA=GrosBot->getX_terrain();
    yA=GrosBot->getY_terrain();
    xB=0;
    yB=0;
    xC=OtherBot->getX_terrain();
    yC=OtherBot->getY_terrain();

    //initialisation de la carte des obstacles
    Recherche->initMap(xC,yC);

    int nb_points_asser=0;

        //lancement de l'algorithme
        clock_t start = std::clock();
    int nb_points=Recherche->pathFind(xA, yA, xB, yB);
    clock_t end = std::clock();
    if(nb_points<=0)
    {
        qDebug()<<"Pas de chemin trouve!";
        qDebug()<<"Dimension carte (X,Y):\t"<<n<<"x"<<m;
                qDebug()<<"Depart=>Objectif:\t("<<xA<<","<<yA<<")=>("<<xB<<","<<yB<<")";
                //qDebug()<<"Scenario:\t\t"<<scenar<<std::endl;
                qDebug()<<"Autre robot:\t\t("<<xC<<","<<yC<<")";
                double time_elapsed = double(end - start);
                qDebug()<<"Temps calcul (ms):\t"<<time_elapsed;
    }
    else
        nb_points_asser=Recherche->pathBuild(xA,yA);



    if(nb_points>0)
	{
        qDebug()<<"Dimension carte (X,Y):\t"<<n<<"x"<<m;
                qDebug()<<"Depart=>Objectif:\t("<<xA<<","<<yA<<")=>("<<xB<<","<<yB<<")";
                //qDebug()<<"Scenario:\t\t"<<scenar<<std::endl;
                qDebug()<<"Autre robot:\t\t("<<xC<<","<<yC<<")";
                double time_elapsed = double(end - start);
                qDebug()<<"Temps calcul (ms):\t"<<time_elapsed;
                qDebug()<<"\nRoute:\t\t\t"<<nb_points<<" pts";
                std::vector<int>::reverse_iterator it;
                for(it=Recherche->path_dir.rbegin(); it!=Recherche->path_dir.rend(); ++it)
                    qDebug()<<(*it);



                qDebug()<<"Trajectoire:\t\t"<<nb_points_asser<<" pts";
                std::vector<int>::iterator it1;
                std::vector<int>::iterator it2;
                it1=Recherche->x_dir.begin();
                it2=Recherche->y_dir.begin();
                int x1,y1;

                //QPolygon loophole;
                if (!(evitement.isEmpty()))
                        for(int k=0;k<evitement.size();k++ ){
                            //QGraphicsLineItem
                            terrain->removeItem(evitement.at(k));
                        }
                evitement.clear();

                int i_points=0;
                int x_temp=0;
                int y_temp=0;

                while((it1!=Recherche->x_dir.end())&&(it1!=Recherche->x_dir.end()))
                {
                    if(i_points==0)
                        i_points++;
                    else
                        evitement.append(new QGraphicsLineItem(x_temp,y_temp,(*it1),-(*it2)));

                    x_temp=(*it1);
                    y_temp=-(*it2);

                    //qDebug()<<"("<<x_temp<<","<<-y_temp<<");";
                    ++it1;
                    ++it2;
                }

                for(int k=0;k<evitement.size();k++)
                {
                    evitement.at(k)->setPen(QPen(Qt::red,3));
                    terrain->addItem(evitement.at(k));
                }

    }
}


// _____________________________________________________________________
/*!
 * \brief CSimuBot::real_robot_position_changed slot appelé quand l'une des coordonnées du robot issues du datamanager change
 *
 * # COMPORTEMENT
 *  Permet au mode VISU et SIMU de faire évoluer le robot dans le simulateur selon les vrais évolutions des coordonnées
 */
void CSimuBot::real_robot_position_changed()
{
    //Si on est en mode VISU
    if ((modeVisu==SIMUBOT::VISU) || (modeVisu==SIMUBOT::SIMU))
    {
        //on récupère les nouvelles coordonnées du robot
        float pos_x = m_application->m_data_center->getData("x_pos")->read().toFloat();
        float pos_y = m_application->m_data_center->getData("y_pos")->read().toFloat();
        float teta_pos = m_application->m_data_center->getData("teta_pos")->read().toFloat();

        //mise à jour des coordonnées dans simubot
        emit displayCoord(pos_x, pos_y);
        emit displayAngle(teta_pos);
    }
}

void CSimuBot::catchDoubleClick()
{
    if(m_ihm.ui.checkBox_setSequence->isChecked())
        emit setSequence();
}

/*!
 * \brief CSimuBot::estimateEnvironmentVariables
 *
 * # COMPORTEMENT
 * Calcule toutes les interactions entre le robot et l'environnement
 * - capteurs US
 * - blocage terrain
 */

void CSimuBot::estimate_Environment_Interactions()
{
    //estimation de l'environnement US
    //récupération des différentes coordonnées (graphique et réelles) de l'adversaire
    qreal x_other_graphic=OtherBot->getX_terrain();
    qreal y_other_graphic=OtherBot->getY_terrain();

    //récupération des différentes coordonnées (graphique et réelles) de notre robot
    qreal x_bot_graphic=GrosBot->getX_terrain();
    qreal y_bot_graphic=GrosBot->getY_terrain();
    qreal theta_bot=GrosBot->getTheta();

    //on calcule la distance entre l'ancienne et la nouvelle position
    float distanceAdversaire = sqrt(pow((x_other_graphic-x_bot_graphic),2)+pow((y_other_graphic-y_bot_graphic),2));
    float angleAdversaire=0.0;

    //on calcule l'angle entre les 2 robots
    if (x_other_graphic==x_bot_graphic)
    {
        if (y_other_graphic>y_bot_graphic)
            angleAdversaire=Pi/2;
        else if (y_other_graphic==y_bot_graphic)
            angleAdversaire=0;
        else if (y_other_graphic<y_bot_graphic)
            angleAdversaire=-Pi/2;
    }
    else if (x_other_graphic>x_bot_graphic)
    {
        angleAdversaire=atan((y_other_graphic-y_bot_graphic)/(x_other_graphic-x_bot_graphic));
    }
    else if (x_other_graphic<x_bot_graphic)
    {
        if (y_other_graphic>y_bot_graphic)
            angleAdversaire=atan((y_other_graphic-y_bot_graphic)/(x_other_graphic-x_bot_graphic))+Pi;
        else if (y_other_graphic==y_bot_graphic)
            angleAdversaire=Pi;
        else if (y_other_graphic<y_bot_graphic)
            angleAdversaire=atan((y_other_graphic-y_bot_graphic)/(x_other_graphic-x_bot_graphic))-Pi;
    }

    double arad=angleAdversaire-theta_bot; //angle en radian
    if(GrosBot->sensOrtho<0)
        arad=arad+Pi;
    double adeg= arad*180/Pi; //angle en degré
    //modulo 360
    while (adeg < 0)
        adeg += 360;
    while (adeg > 360)
        adeg -= 360;
    /*
    //Affichage des nouvelles valeurs position, angle du robot
    qDebug() << "coord graphique: G(" <<x_prim_graphic <<","<<y_prim_graphic<<")\tAd("<<x_graphic<<","<<y_graphic<<")";
    qDebug() << "coord réelles: G(" <<x_prim_view <<","<<y_prim_view<<")\tAd("<<x_view<<","<<y_view<<")";
    qDebug() << "distance : " << distanceAdversaire;
    qDebug() << "angle : " << adeg;
    */
    distanceAdversaire=distanceAdversaire-15.0; //prise en compte du diamètre moyen du robot adverse
    double US_AVG=99;
    double US_AVD=99;
    double US_ARG=99;
    double US_ARD=99;
    //qDebug() << distanceAdversaire << "," << adeg;
    if(distanceAdversaire<=120)
    {
        if(((adeg>=0)&&(adeg<45))||((adeg>340)&&(adeg<=360))) //robot adverse devant à gauche
        {
            US_AVG=sqrt(pow((distanceAdversaire*cos(arad)-12),2)+pow((distanceAdversaire*sin(arad)-16),2));
        }
        if(((adeg>315)&&(adeg<=360))||((adeg>=0)&&(adeg<20))) //robot adverse devant à droite
        {
            US_AVD=sqrt(pow((distanceAdversaire*cos(arad)-12),2)+pow((distanceAdversaire*sin(arad)+16),2));
        }
        if((adeg>135)&&(adeg<=200)) //robot adverse arrière à gauche
        {
            US_ARG=sqrt(pow((distanceAdversaire*cos(arad)+12),2)+pow((distanceAdversaire*sin(arad)-16),2));
        }
        if((adeg>=160)&&(adeg<225)) //robot adverse arrière à droite
        {
            US_ARD=sqrt(pow((distanceAdversaire*cos(arad)+12),2)+pow((distanceAdversaire*sin(arad)+16),2));
        }
    }

    m_application->m_data_center->write("Simubot.Telemetres.AVG", US_AVG);
    m_application->m_data_center->write("Simubot.Telemetres.AVD", US_AVD);
    m_application->m_data_center->write("Simubot.Telemetres.ARG", US_ARG);
    m_application->m_data_center->write("Simubot.Telemetres.ARD", US_ARD);

    /*
    qDebug() << "AVG "<<US_AVG<<" - AVD "<<US_AVD;
    qDebug() << "ARG "<<US_ARG<<" - ARD "<<US_ARD;
    qDebug() << "------------------------------------------------------";
    */


    //estimation de blocage sur le terrain
    //l'idée est d'interdire le mouvement d'un côté si un blocage est détecté à gauche ou a droite
    if(m_ihm.ui.checkBox_enableBlocking->isChecked())
    {
        float cmde_MotG = m_application->m_data_center->getData("Cde_MotG")->read().toFloat();
        float cmde_MotD = m_application->m_data_center->getData("Cde_MotD")->read().toFloat();
        bool blocage_G=false;
        bool blocage_D=false;

        //conversion en degre
        double teta_deg= theta_bot*180/Pi; //angle en degré
        //modulo 360 degré
        while (teta_deg < 0)
            teta_deg += 360;
        while (teta_deg > 360)
            teta_deg -= 360;
        //collision avec la bordure en x=0
        double x_limit=fabs(23.5*sin(theta_bot+(Pi/4)));

        if(x_bot_graphic<=x_limit)
        {
            if(((teta_deg<=1)||(teta_deg>=359))&&(cmde_MotG<0)&&(cmde_MotD<0))
            {
                blocage_G=true;
                blocage_D=true;
            }
            else if (((teta_deg>1)&&(teta_deg<90))&&(cmde_MotG<0))
            {
                blocage_G=true;
            }
            else if (((teta_deg>270)&&(teta_deg<359))&&(cmde_MotD<0))
            {
                blocage_D=true;
            }
        }

        m_application->m_data_center->write("Simubot.blocage.gauche", blocage_G);
        m_application->m_data_center->write("Simubot.blocage.droite", blocage_D);

    }

}

/*!
 * \brief CSimuBot::estimateEnvironmentVariables
 *
 * # COMPORTEMENT
 * Calcule toutes les interactions entre le robot et l'environnement
 * - capteurs US
 * - blocage terrain
 */

void CSimuBot::playOther()
{

    double speedOther=m_ihm.ui.doubleSpinBox_speed->value();
    if(speedOther>0)
    {
        currentIndex=0;
        QTableWidgetItem * widget_x=m_ihm.ui.tableWidget->item(currentIndex,0);
        QTableWidgetItem * widget_y=m_ihm.ui.tableWidget->item(currentIndex,1);
        QTableWidgetItem * widget_teta=m_ihm.ui.tableWidget->item(currentIndex,2);
        if((widget_x)&&(widget_y)&&(widget_teta))
        {
            QString str_x_record=m_ihm.ui.tableWidget->item(currentIndex,0)->text();
            QString str_y_record=m_ihm.ui.tableWidget->item(currentIndex,1)->text();
            QString str_teta_record=m_ihm.ui.tableWidget->item(currentIndex,2)->text();
            if((!str_x_record.isEmpty())&&(!str_y_record.isEmpty())&&(!str_teta_record.isEmpty()))
            {
                m_ihm.ui.pB_playOther->setDisabled(true);
                m_ihm.ui.doubleSpinBox_speed->setDisabled(true);
                m_ihm.ui.pB_stopOther->setDisabled(false);

                OtherBot->setSpeed(speedOther);
                float x_record=str_x_record.toFloat();
                float y_record=str_y_record.toFloat();
                //float teta_record=str_teta_record.toFloat();
                //qDebug() << "[SimuBot] Demande n°1 de déplacement du robot adverse ("<<x_record<<","<<y_record<<") à la vitesse "<<speedOther;
                OtherBot->display_XY(x_record,y_record);
                cadenceur->start(25);
            }
        }
    }
}

void CSimuBot::stopOther()
{
    cadenceur->stop();
    currentIndex=0;
    OtherBot->setSpeed(0);

    m_ihm.ui.pB_playOther->setDisabled(false);
    m_ihm.ui.doubleSpinBox_speed->setDisabled(false);
    m_ihm.ui.pB_stopOther->setDisabled(true);
}

void CSimuBot::nextStepOther()
{
    if(OtherBot->isConvergence)
    {
        currentIndex++;
        if(currentIndex<=10)
        {
            QTableWidgetItem * widget_x=m_ihm.ui.tableWidget->item(currentIndex,0);
            QTableWidgetItem * widget_y=m_ihm.ui.tableWidget->item(currentIndex,1);
            QTableWidgetItem * widget_teta=m_ihm.ui.tableWidget->item(currentIndex,2);
            if((widget_x)&&(widget_y)&&(widget_teta))
            {
                QString str_x_record=m_ihm.ui.tableWidget->item(currentIndex,0)->text();
                QString str_y_record=m_ihm.ui.tableWidget->item(currentIndex,1)->text();
                QString str_teta_record=m_ihm.ui.tableWidget->item(currentIndex,2)->text();
                if((!str_x_record.isEmpty())&&(!str_y_record.isEmpty())&&(!str_teta_record.isEmpty()))
                {
                    float x_record=str_x_record.toFloat();
                    float y_record=str_y_record.toFloat();
                    //float teta_record=str_teta_record.toFloat();
                    //qDebug() << "[SimuBot] Demande n°"<<currentIndex<<" de déplacement du robot adverse ("<<x_record<<","<<y_record<<")";
                    OtherBot->display_XY(x_record,y_record);
                }
                else
                    stopOther();
            }
            else
            {
                stopOther();
            }
        }
        else
        {
            stopOther();
        }
    }
    else
        OtherBot->moveAtSpeed();
}

void CSimuBot::addLineOther(double x, double y, double teta, int row)
{
    QTableWidgetItem * qTbW_x=new QTableWidgetItem();
    QTableWidgetItem * qTbW_y=new QTableWidgetItem();
    QTableWidgetItem * qTbW_teta=new QTableWidgetItem();

    QString str_x, str_y, str_teta;
    qTbW_x->setText(str_x.setNum(x));
    qTbW_y->setText(str_y.setNum(y));
    qTbW_teta->setText(str_teta.setNum(teta));

    m_ihm.ui.tableWidget->setItem(row,0,qTbW_x);
    m_ihm.ui.tableWidget->setItem(row,1,qTbW_y);
    m_ihm.ui.tableWidget->setItem(row,2,qTbW_teta);
}

void CSimuBot::enableMoveOther(int state)
{
    if(state==Qt::Checked)
    {
        m_ihm.ui.pB_playOther->setDisabled(false);
        m_ihm.ui.doubleSpinBox_speed->setDisabled(false);
    }
    else
    {
        stopOther();
        m_ihm.ui.pB_playOther->setDisabled(true);
        m_ihm.ui.doubleSpinBox_speed->setDisabled(true);
    }
}

