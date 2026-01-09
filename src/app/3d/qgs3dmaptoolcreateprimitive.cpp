/***************************************************************************
    qgs3dmaptoolcreatecube.cpp
    -------------------
    begin                : November 2025
    copyright            : (C) 2025 Oslandia
    email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgs3dmaptoolcreateprimitive.h"

#include "qgisapp.h"
#include "qgs3dcreateprimitiveconedialog.h"
#include "qgs3dcreateprimitivecubedialog.h"
#include "qgs3dcreateprimitivecylinderdialog.h"
#include "qgs3dcreateprimitivedialog.h"
#include "qgs3dcreateprimitivespheredialog.h"
#include "qgs3dcreateprimitivetorusdialog.h"
#include "qgs3drendercontext.h"
#include "qgs3dutils.h"
#include "qgsaction.h"
#include "qgsactionmanager.h"
#include "qgsattributedialog.h"
#include "qgsattributeform.h"
#include "qgscameracontroller.h"
#include "qgsfeatureaction.h"
#include "qgsforwardrenderview.h"
#include "qgsframegraph.h"
#include "qgsgeotransform.h"
#include "qgsraycastcontext.h"
#include "qgsraycasthit.h"
#include "qgsraycastingutils.h"
#include "qgsrubberband3d.h"
#include "qgssfcgalgeometry.h"
#include "qgsvectorlayer.h"
#include "qgswindow3dengine.h"

#include <QMouseEvent>
#include <Qt3DExtras/QConeMesh>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/QTorusMesh>
#include <Qt3DRender/QRenderSettings>
#include <Qt3DRender/QScreenRayCaster>

Qgs3DMapToolCreatePrimitive::Qgs3DMapToolCreatePrimitive( Qgs3DMapCanvas *canvas, PrimitiveType type )
  : Qgs3DMapTool( canvas ), mType( type )
{
  // Dialog
  switch ( type )
  {
    case Cube:
    case Box:
      mDialog.reset( new Qgs3DCreatePrimitiveCubeDialog() );
      break;
    case Sphere:
      mDialog.reset( new Qgs3DCreatePrimitiveSphereDialog() );
      break;
    case Cylinder:
      mDialog.reset( new Qgs3DCreatePrimitiveCylinderDialog() );
      break;
    case Torus:
      mDialog.reset( new Qgs3DCreatePrimitiveTorusDialog() );
      break;
    case Cone:
      mDialog.reset( new Qgs3DCreatePrimitiveConeDialog() );
      break;
  }

  connect( mDialog.get(), &Qgs3DCreatePrimitiveDialog::valueChanged, this, [this]() { updatePrimitive(); } );
  connect( mDialog.get(), &Qgs3DCreatePrimitiveDialog::createClicked, this, [this]() { createPrimitive(); } );
  connect( mDialog.get(), &QDialog::finished, this, [this]() { finish(); } );
}

Qgs3DMapToolCreatePrimitive::~Qgs3DMapToolCreatePrimitive() = default;

void Qgs3DMapToolCreatePrimitive::activate()
{
  qDebug() << u"%1 #%2:"_s.arg( __FUNCTION__ ).arg( __LINE__ ).toStdString();
  restart();

  // Show dialog
  if ( mShowPrimitiveDialog )
    mDialog->show();
}

void Qgs3DMapToolCreatePrimitive::deactivate()
{
  finish();

  // Hide dialog
  mDialog->hide();
}

void Qgs3DMapToolCreatePrimitive::finish()
{
  qDebug() << u"%1 #%2:"_s.arg( __FUNCTION__ ).arg( __LINE__ ).toStdString();
  mCanvas->setCursor( cursor() );
  if ( mShowPrimitiveDialog )
    mDialog->unfocusCreateButton();

  mPrimitiveLineEntity.reset();

  mCurrentFieldIdx = -1;
  mPointOnMap.clear();
  mDialog->resetData();
  mMouseClickPos = QPoint();

  mDone = true;
}

QCursor Qgs3DMapToolCreatePrimitive::cursor() const
{
  return Qt::CrossCursor;
}

void Qgs3DMapToolCreatePrimitive::restart()
{
  qDebug() << u"%1 #%2:"_s.arg( __FUNCTION__ ).arg( __LINE__ ).toStdString();
  mDone = false;

  mRubberBand.reset( new QgsRubberBand3D( *mCanvas->mapSettings(), mCanvas->engine(), mCanvas->engine()->frameGraph()->rubberBandsRootEntity() ) );
  const QgsSettings settings;
  const int myRed = settings.value( u"qgis/default_measure_color_red"_s, 222 ).toInt();
  const int myGreen = settings.value( u"qgis/default_measure_color_green"_s, 155 ).toInt();
  const int myBlue = settings.value( u"qgis/default_measure_color_blue"_s, 67 ).toInt();
  mRubberBand->setWidth( 3 );
  mRubberBand->setColor( QColor( myRed, myGreen, myBlue ) );
}

QgsPoint Qgs3DMapToolCreatePrimitive::screenToMap( const QPoint &screenPos ) const
{
  QgsRayCastContext context;
  context.setSingleResult( false );
  context.setMaximumDistance( mCanvas->cameraController()->camera()->farPlane() );
  context.setAngleThreshold( 0.5f );
  const QgsRayCastResult results = mCanvas->castRay( screenPos, context );

  if ( results.isEmpty() )
    return QgsPoint();

  QgsVector3D mapCoords;
  double minDist = -1;
  const QList<QgsRayCastHit> allHits = results.allHits();
  for ( const QgsRayCastHit &hit : allHits )
  {
    const double resDist = hit.distance();
    if ( minDist < 0 || resDist < minDist )
    {
      minDist = resDist;
      mapCoords = hit.mapCoordinates();
    }
  }
  if ( std::isnan( mapCoords.z() ) )
    return QgsPoint( mapCoords.x(), mapCoords.y(), 0 );

  return QgsPoint( mapCoords.x(), mapCoords.y(), mapCoords.z() );
}

void Qgs3DMapToolCreatePrimitive::updatePrimitive()
{
  double sX = 1.0, sY = 1.0, sZ = 1.0;
  double rX = 0.0, rY = 0.0, rZ = 0.0;
  double tX = 0.0, tY = 0.0, tZ = 0.0;
  QgsGeoTransform *transform;
  if ( mPrimitiveLineEntity.get() == nullptr )
  {
    mPrimitiveLineEntity.reset( new Qt3DCore::QEntity( mCanvas->engine()->frameGraph()->rubberBandsRootEntity() ) );
    mPrimitiveLineEntity->setObjectName( "new_primitive" );

    switch ( mType )
    {
      case Cube:
      case Box:
      {
        QgsPrivate::Qgs3DWiredMesh *mesh = new QgsPrivate::Qgs3DWiredMesh;
        QgsAABB box = QgsAABB( -0.5f, -0.5f, 0, //
                               0.5f, 0.5f, 1.0 );
        mesh->setVertices( box.verticesForLines() );
        mPrimitiveLineEntity->addComponent( mesh );
        mCurrentMesh = mesh;
        break;
      }
      case Sphere:
      {
        Qt3DExtras::QSphereMesh *mesh = new Qt3DExtras::QSphereMesh();
        mesh->setRadius( 0.5 );
        mesh->setRings( 6 );
        mesh->setSlices( 6 );
        mPrimitiveLineEntity->addComponent( mesh );
        mCurrentMesh = mesh;
        break;
      }
      case Cylinder:
      {
        Qt3DExtras::QCylinderMesh *mesh = new Qt3DExtras::QCylinderMesh();
        mesh->setRadius( 0.5 );
        mesh->setLength( 1.0 );
        mesh->setRings( 2 );
        mesh->setSlices( 6 );
        mPrimitiveLineEntity->addComponent( mesh );
        mCurrentMesh = mesh;
        rX = 90;
        break;
      }
      case Torus:
      {
        Qt3DExtras::QTorusMesh *mesh = new Qt3DExtras::QTorusMesh();
        mesh->setRadius( 0.5 );
        mesh->setMinorRadius( 0.5 );
        mesh->setRings( 6 );
        mesh->setSlices( 6 );
        mPrimitiveLineEntity->addComponent( mesh );
        mCurrentMesh = mesh;
        break;
      }
      case Cone:
      {
        Qt3DExtras::QConeMesh *mesh = new Qt3DExtras::QConeMesh();
        mesh->setBottomRadius( 0.5 );
        mesh->setLength( 1.0 );
        mesh->setTopRadius( 0.5 );
        mesh->setRings( 2 );
        mesh->setSlices( 6 );
        mPrimitiveLineEntity->addComponent( mesh );
        rX = 90;
        mCurrentMesh = mesh;
        break;
      }
    }

    Qt3DExtras::QPhongMaterial *material = new Qt3DExtras::QPhongMaterial;
    material->setAmbient( Qt::blue );
    mPrimitiveLineEntity->addComponent( material );

    transform = new QgsGeoTransform( mPrimitiveLineEntity.get() );
    mPrimitiveLineEntity->addComponent( transform );
  }
  else
  {
    for ( auto trans : mPrimitiveLineEntity->findChildren<QgsGeoTransform *>() )
    {
      transform = trans;
      break;
    }

    switch ( mType )
    {
      case Cube:
      case Box:
      {
        sX = mDialog->getParam( 0 );
        sY = mDialog->getParam( 1 );
        sZ = mDialog->getParam( 2 );
        break;
      }
      case Sphere:
      {
        Qt3DExtras::QSphereMesh *mesh = dynamic_cast<Qt3DExtras::QSphereMesh *>( mCurrentMesh );
        mesh->setRadius( mDialog->getParam( 0 ) );
        mesh->setRings( std::min( 6, static_cast<int>( mDialog->getParam( 1 ) ) ) );
        mesh->setSlices( std::min( 6, static_cast<int>( mDialog->getParam( 2 ) ) ) );
        break;
      }
      case Cylinder:
      {
        Qt3DExtras::QCylinderMesh *mesh = dynamic_cast<Qt3DExtras::QCylinderMesh *>( mCurrentMesh );
        mesh->setRadius( mDialog->getParam( 0 ) );
        mesh->setLength( mDialog->getParam( 1 ) );
        mesh->setSlices( std::min( 6, static_cast<int>( mDialog->getParam( 2 ) ) ) );
        rX = 90;
        tZ = 0.5 * mDialog->getParam( 1 );
        break;
      }
      case Torus:
      {
        Qt3DExtras::QTorusMesh *mesh = dynamic_cast<Qt3DExtras::QTorusMesh *>( mCurrentMesh );
        mesh->setRadius( mDialog->getParam( 0 ) );
        mesh->setMinorRadius( mDialog->getParam( 1 ) );
        mesh->setRings( std::min( 6, static_cast<int>( mDialog->getParam( 2 ) ) ) );
        mesh->setSlices( std::min( 6, static_cast<int>( mDialog->getParam( 3 ) ) ) );
        break;
      }
      case Cone:
      {
        Qt3DExtras::QConeMesh *mesh = dynamic_cast<Qt3DExtras::QConeMesh *>( mCurrentMesh );
        mesh->setBottomRadius( mDialog->getParam( 0 ) );
        mesh->setLength( mDialog->getParam( 1 ) );
        mesh->setTopRadius( mDialog->getParam( 2 ) );
        mesh->setSlices( std::min( 6, static_cast<int>( mDialog->getParam( 3 ) ) ) );
        rX = 90;
        tZ = 0.5 * mDialog->getParam( 1 );
        break;
      }
    }
  }

  transform->setOrigin( mCanvas->mapSettings()->origin() );
  transform->setRotationX( static_cast<float>( mDialog->rotX() + rX ) );
  transform->setRotationY( static_cast<float>( mDialog->rotY() + rY ) );
  transform->setRotationZ( static_cast<float>( mDialog->rotZ() + rZ ) );
  transform->setGeoTranslation( { static_cast<float>( mDialog->transX() + tX ), static_cast<float>( mDialog->transY() + tY ), static_cast<float>( mDialog->transZ() + tZ ) } );
  transform->setScale3D( { static_cast<float>( mDialog->scaleX() * sX ), static_cast<float>( mDialog->scaleY() * sY ), static_cast<float>( mDialog->scaleZ() * sZ ) } );
}

void Qgs3DMapToolCreatePrimitive::handleClick( QMouseEvent *event )
{
  qDebug() << u"%1 #%2:"_s.arg( __FUNCTION__ ).arg( __LINE__ ).toStdString();
  if ( mCurrentFieldIdx == 0 )
  {
    qDebug() << u"%1 #%2:"_s.arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "First click";
    mMouseClickPos = event->pos();

    mPointOnMap << screenToMap( event->pos() );
    mDialog->setTranslation( mPointOnMap.last() );
    updatePrimitive();

    QgsPoint rbPoint( mPointOnMap.last() );
    rbPoint.setZ( rbPoint.z() / mCanvas->mapSettings()->terrainSettings()->verticalScale() );
    mRubberBand->addPoint( rbPoint );
    mRubberBand->addPoint( rbPoint );

    ++mCurrentFieldIdx;
  }
  else if ( mCurrentFieldIdx <= mDialog->creationParamNumber() )
  {
    QgsPoint pointMap = screenToMap( event->pos() );
    double length = constraintMapPoint( pointMap, event->modifiers() );
    mDialog->setParam( mCurrentFieldIdx - 1, length );

    mPointOnMap << pointMap;

    QgsPoint rbPoint( pointMap );
    rbPoint.setZ( rbPoint.z() / mCanvas->mapSettings()->terrainSettings()->verticalScale() );
    mRubberBand->addPoint( rbPoint );

    ++mCurrentFieldIdx;
    if ( mCurrentFieldIdx > mDialog->creationParamNumber() )
    {
      mCanvas->setCursor( Qt::WaitCursor );
      mDialog->hide();
      if ( mShowPrimitiveDialog )
      {
        mDialog->show();
        //mDialog->restorePosition();
        mDialog->focusCreateButton();
      }
    }
  }
}

void Qgs3DMapToolCreatePrimitive::mousePressEvent( QMouseEvent * /*event*/ )
{
}

double Qgs3DMapToolCreatePrimitive::constraintMapPoint( QgsPoint &pointMap, const Qt::KeyboardModifiers &stateKey )
{
  QgsPoint prevPointMap = mPointOnMap.last();
  double length;
  Qgs3DCreatePrimitiveDialog::ConstrainedAxis constraint = Qgs3DCreatePrimitiveDialog::NONE;
  if ( ( stateKey & Qt::Modifier::CTRL ) == 0 )
    constraint = Qgs3DCreatePrimitiveDialog::NONE;
  else
    constraint = mDialog->constrainedAxisForParam( mCurrentFieldIdx - 1 );

  switch ( constraint )
  {
    case Qgs3DCreatePrimitiveDialog::NONE:
      length = prevPointMap.distance3D( pointMap );
      break;
    case Qgs3DCreatePrimitiveDialog::X:
      length = std::abs( prevPointMap.x() - pointMap.x() );
      pointMap.setY( prevPointMap.y() );
      pointMap.setZ( prevPointMap.z() );
      break;
    case Qgs3DCreatePrimitiveDialog::Y:
      length = std::abs( prevPointMap.y() - pointMap.y() );
      pointMap.setX( prevPointMap.x() );
      pointMap.setZ( prevPointMap.z() );
      break;
    case Qgs3DCreatePrimitiveDialog::Z:
      length = std::abs( prevPointMap.z() - pointMap.z() );
      pointMap.setX( prevPointMap.x() );
      pointMap.setY( prevPointMap.y() );
      break;
    case Qgs3DCreatePrimitiveDialog::XY:
      length = std::sqrt( std::pow( prevPointMap.x() - pointMap.x(), 2 ) + std::pow( prevPointMap.y() - pointMap.y(), 2 ) );
      pointMap.setZ( prevPointMap.z() );
      break;
    case Qgs3DCreatePrimitiveDialog::XZ:
      length = std::sqrt( std::pow( prevPointMap.x() - pointMap.x(), 2 ) + std::pow( prevPointMap.z() - pointMap.z(), 2 ) );
      pointMap.setY( prevPointMap.y() );
      break;
    case Qgs3DCreatePrimitiveDialog::YZ:
      length = std::sqrt( std::pow( prevPointMap.z() - pointMap.z(), 2 ) + std::pow( prevPointMap.y() - pointMap.y(), 2 ) );
      pointMap.setX( prevPointMap.x() );
      break;
  }

  qDebug() << u"%1 #%2:"_s.arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "setting param" << mCurrentFieldIdx - 1 << "to value: " << length;
  return length;
}

void Qgs3DMapToolCreatePrimitive::mouseMoveEvent( QMouseEvent *event )
{
  if ( mDone )
  {
    restart();
  }

  if ( !mMouseHasMoved && ( event->pos() - mMouseClickPos ).manhattanLength() >= QApplication::startDragDistance() )
  {
    mMouseHasMoved = true;
  }

  QgsPoint pointMap = screenToMap( event->pos() );

  if ( mCurrentFieldIdx == 0 )
  {
    mDialog->setTranslation( pointMap );
  }
  else if ( mCurrentFieldIdx <= mDialog->creationParamNumber() )
  {
    if ( mCurrentFieldIdx == 1 && mType == Cube )
    {
      QgsPoint prevPointMap = mPointOnMap.last();
      double angle = -1.0 * QgsGeometryUtilsBase::lineAngle( pointMap.x(), pointMap.y(), prevPointMap.x(), prevPointMap.y() );
      angle *= 180.0 / M_PI;
      angle += 90.0; // TODO WHY??
      qDebug() << u"%1 #%2:"_s.arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "prim rotation: " << angle;

      mDialog->setRotation( mDialog->rotX(), mDialog->rotY(), ( angle < 0.0 ? 360.0 + angle : angle ) );
    }

    double length = constraintMapPoint( pointMap, event->modifiers() );
    mDialog->setParam( mCurrentFieldIdx - 1, length );

    QgsPoint rbPoint = pointMap;
    rbPoint.setZ( rbPoint.z() / mCanvas->mapSettings()->terrainSettings()->verticalScale() );
    mRubberBand->moveLastPoint( rbPoint );

    updatePrimitive();
  }
}

void Qgs3DMapToolCreatePrimitive::mouseReleaseEvent( QMouseEvent *event )
{
  if ( event->button() == Qt::LeftButton )
  {
    if ( mDone )
    {
      restart();
    }

    handleClick( event );
  }
  else if ( event->button() == Qt::RightButton )
  {
    if ( mCurrentFieldIdx > 0 )
    {
      mCanvas->setCursor( cursor() );
      --mCurrentFieldIdx;
      mRubberBand->removeLastPoint();
      mPointOnMap.removeLast();

      if ( mCurrentFieldIdx == 0 )
      {
        // Finish measurement
        finish();
      }
    }
  }
}

void Qgs3DMapToolCreatePrimitive::keyReleaseEvent( QKeyEvent *event )
{
  if ( event->key() == Qt::Key_Escape )
  {
    finish();
  }
  else if ( event->key() == Qt::Key_Enter )
  {
    createPrimitive();
  }
}

void Qgs3DMapToolCreatePrimitive::createPrimitive()
{
  QgsVectorLayer *vl = dynamic_cast<QgsVectorLayer *>( QgisApp::instance()->activeLayer() );
  if ( vl != nullptr && QgsWkbTypes::flatType( vl->wkbType() ) == Qgis::WkbType::PolyhedralSurface )
  {
    Qgs3DRenderContext renderCtx = Qgs3DRenderContext::fromMapSettings( this->mCanvas->mapSettings() );
    QgsCoordinateTransform coordTrans = QgsCoordinateTransform( vl->crs3D(), renderCtx.crs(), renderCtx.transformContext() );

    QgsVector3D translate( mDialog->transX(), mDialog->transY(), mDialog->transZ() );
    translate.setZ( translate.z() / mCanvas->mapSettings()->terrainSettings()->verticalScale() );

    QgsVector3D scale( mDialog->scaleX(), mDialog->scaleY(), mDialog->scaleZ() );

    std::unique_ptr<QgsSfcgalGeometry> geom;
    switch ( mType )
    {
      case Cube:
      case Box:
        geom = QgsSfcgalGeometry::createBox( mDialog->getParam( 0 ), mDialog->getParam( 1 ), mDialog->getParam( 2 ) /** 0.5*/ );
        translate.setX( translate.x() - 0.5 * mDialog->getParam( 0 ) );
        translate.setY( translate.y() - 0.5 * mDialog->getParam( 1 ) );
        break;
      case Sphere:
        geom = QgsSfcgalGeometry::createSphere( mDialog->getParam( 0 ), mDialog->getParam( 1 ), mDialog->getParam( 2 ) );
        break;
      case Cylinder:
        geom = QgsSfcgalGeometry::createCylinder( mDialog->getParam( 0 ), mDialog->getParam( 1 ) /* * 0.25*/, mDialog->getParam( 2 ) );
        break;
      case Torus:
        geom = QgsSfcgalGeometry::createTorus( mDialog->getParam( 0 ), mDialog->getParam( 1 ), mDialog->getParam( 2 ), mDialog->getParam( 3 ) );
        break;
      case Cone:
        geom = QgsSfcgalGeometry::createCone( mDialog->getParam( 0 ), mDialog->getParam( 1 ) /* * 0.25*/, mDialog->getParam( 2 ), mDialog->getParam( 3 ) );
        break;
    }

    QgsVector3D newTrans = coordTrans.transform( translate, Qgis::TransformDirection::Reverse );
    newTrans.setZ( newTrans.z() / mCanvas->mapSettings()->terrainSettings()->verticalScale() );

    geom = geom->translate( newTrans );
    geom = geom->scale( scale );
    //geom = geom->rotate3D( ??? );

    QgsFeature feat;
    feat.setGeometry( geom->asQgisGeometry() );
    qDebug() << u"%1 #%2:"_s.arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "Will save geom:" << feat.geometry().get()->asWkt( 1 );

    if ( mShowAttributeValuesDlg )
    {
      QgsExpressionContext expContext = vl->createExpressionContext();
      QgsFeature newFeature = QgsAttributeForm::createFeature( vl, feat.geometry(), QgsAttributeMap(), expContext );
      QgsAttributeEditorContext context( QgisApp::instance()->createAttributeEditorContext() );
      context.setFormMode( QgsAttributeEditorContext::StandaloneDialog );
      QgsAttributeDialog *dialog = new QgsAttributeDialog( vl, &newFeature, false, nullptr, true, context );

      // Skip this code on windows, because the Qt::Tool flag prevents the maximize button to be shown
#ifndef Q_OS_WIN
      dialog->setWindowFlags( dialog->windowFlags() | Qt::Tool );
#else
      dialog->setWindowFlags( dialog->windowFlags() | Qt::CustomizeWindowHint | Qt::WindowMaximizeButtonHint );
      if ( !dialog->parent() )
        dialog->setWindowFlag( Qt::WindowStaysOnTopHint );
#endif

      dialog->setObjectName( u"featureactiondlg:%1:%2"_s.arg( vl->id() ).arg( newFeature.id() ) );

      const QList<QgsAction> actions = vl->actions()->actions( u"Feature"_s );
      if ( !actions.isEmpty() )
      {
        dialog->setContextMenuPolicy( Qt::ActionsContextMenu );

        QAction *a = new QAction( tr( "Run Actions" ), dialog );
        a->setEnabled( false );
        dialog->addAction( a );

        for ( const QgsAction &action : actions )
        {
          if ( !action.runable() )
            continue;

          if ( !vl->isEditable() && action.isEnabledOnlyWhenEditable() )
            continue;

          QgsFeature &feat = const_cast<QgsFeature &>( *dialog->feature() );
          QgsFeatureAction *a = new QgsFeatureAction( action.name(), feat, vl, action.id(), -1, dialog );
          dialog->addAction( a );
          connect( a, &QAction::triggered, a, &QgsFeatureAction::execute );

          QAbstractButton *pb = dialog->findChild<QAbstractButton *>( action.name() );
          if ( pb )
            connect( pb, &QAbstractButton::clicked, a, &QgsFeatureAction::execute );
        }
      }

      // delete the dialog when it is closed
      dialog->setAttribute( Qt::WA_DeleteOnClose );
      dialog->setMode( QgsAttributeEditorContext::AddFeatureMode );
      dialog->setEditCommandMessage( QObject::tr( "Add new primitive" ) );

      connect( dialog->attributeForm(), &QgsAttributeForm::featureSaved, this, [vl]() {
        vl->triggerRepaint();
      } );

      dialog->show();
      dialog->exec();
    }
    else
    {
      vl->beginEditCommand( QObject::tr( "Add new primitive" ) );
      bool featureSaved = vl->addFeature( feat, QgsFeatureSink::FastInsert );
      if ( featureSaved )
      {
        vl->endEditCommand();
        vl->triggerRepaint();
      }
      else
      {
        vl->destroyEditCommand();
      }
    }
  }
  finish();
}
