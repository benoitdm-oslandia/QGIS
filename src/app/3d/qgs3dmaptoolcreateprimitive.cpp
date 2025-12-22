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

#include "qgs3dcreateprimitiveconedialog.h"
#include "qgs3dcreateprimitivecubedialog.h"
#include "qgs3dcreateprimitivecylinderdialog.h"
#include "qgs3dcreateprimitivedialog.h"
#include "qgs3dcreateprimitivespheredialog.h"
#include "qgs3dcreateprimitivetorusdialog.h"
#include "qgs3drendercontext.h"
#include "qgs3dutils.h"
#include "qgscameracontroller.h"
#include "qgsframegraph.h"
#include "qgsgeotransform.h"
#include "qgsraycastcontext.h"
#include "qgsraycasthit.h"
#include "qgsraycastingutils.h"
#include "qgsrubberband3d.h"
#include "qgswindow3dengine.h"

#include <QMouseEvent>
#include <QString>
#include <Qt3DExtras/QConeMesh>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/QTorusMesh>
#include <Qt3DRender/QRenderSettings>

using namespace Qt::StringLiterals;

Qgs3DMapToolCreatePrimitive::Qgs3DMapToolCreatePrimitive( Qgs3DMapCanvas *canvas, QgsMapLayer *activeLayer, PrimitiveType type )
  : Qgs3DMapTool( canvas )
  , mType( type )
  , mActiveLayer( activeLayer )
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

  // revert cursor to default
  mCanvas->setCursor( Qt::ArrowCursor );

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
        QgsAABB box = QgsAABB(
          -0.5f,
          -0.5f,
          0, //
          0.5f,
          0.5f,
          1.0
        );
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
  if ( mCurrentFieldIdx < 0 )
  {
    qDebug() << u"%1 #%2:"_s.arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "First click";
    mMouseClickPos = event->pos();

    mPointOnMap.clear();
    mPointOnMap << screenToMap( event->pos() );
    mDialog->setTranslation( mPointOnMap.last() );
  }
  else if ( mCurrentFieldIdx < mDialog->creationParamNumber() )
  {
    QgsPoint pointMap = screenToMap( event->pos() );
    double length = constraintMapPoint( pointMap, event->modifiers() );
    mDialog->setParam( mCurrentFieldIdx, length );
    mPointOnMap << pointMap;
  }
}

void Qgs3DMapToolCreatePrimitive::mousePressEvent( QMouseEvent * /*event*/ )
{}

double Qgs3DMapToolCreatePrimitive::constraintMapPoint( QgsPoint &pointMap, const Qt::KeyboardModifiers &stateKey )
{
  QgsPoint prevPointMap = mPointOnMap.last();
  double length;
  Qgs3DCreatePrimitiveDialog::ConstrainedAxis constraint = Qgs3DCreatePrimitiveDialog::NONE;
  if ( ( stateKey & Qt::Modifier::CTRL ) == 0 )
    constraint = Qgs3DCreatePrimitiveDialog::NONE;
  else
    constraint = mDialog->constrainedAxisForParam( mCurrentFieldIdx );

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

  qDebug() << u"%1 #%2:"_s.arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "setting param" << mCurrentFieldIdx << "to value: " << length;
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

  if ( mCurrentFieldIdx < 0 )
  {
    mDialog->setTranslation( pointMap );
  }
  else if ( mCurrentFieldIdx < mDialog->creationParamNumber() )
  {
    if ( mCurrentFieldIdx == 0 && mType == Cube )
    {
      QgsPoint prevPointMap = mPointOnMap.last();
      double angle = -1.0 * QgsGeometryUtilsBase::lineAngle( pointMap.x(), pointMap.y(), prevPointMap.x(), prevPointMap.y() );
      angle *= 180.0 / M_PI;
      angle += 90.0; // TODO WHY??
      qDebug() << u"%1 #%2:"_s.arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "prim rotation: " << angle;

      mDialog->setRotation( mDialog->rotX(), mDialog->rotY(), ( angle < 0.0 ? 360.0 + angle : angle ) );
    }

    double length = constraintMapPoint( pointMap, event->modifiers() );
    mDialog->setParam( mCurrentFieldIdx, length );

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
    // if ( mCurrentFieldIdx < 3 )
    // {
    //   mCurrentFieldIdx = 2; // if we left click with the mouse the 3 translation fields are set
    // }
    handleClick( event );

    handleNextParameter();
  }
  else if ( event->button() == Qt::RightButton )
  {
    // if ( mCurrentFieldIdx <= 3 )
    // {
    //   mCurrentFieldIdx = 1; // if we right click with the mouse the 3 translation fields are canceled
    // }
    handlePreviousParameter();
  }

  if ( mShowPrimitiveDialog && !mDone && mCurrentFieldIdx != mDialog->creationParamNumber() )
  {
    mDialog->show();
    qDebug() << u"%1 #%2:"_s.arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "focus on param:" << mCurrentFieldIdx;
    mDialog->focusOnParam( mCurrentFieldIdx );
  }
}

void Qgs3DMapToolCreatePrimitive::keyReleaseEvent( QKeyEvent *event )
{
  qDebug() << u"%1 #%2:"_s.arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "key:" << event;
  if ( event->key() == Qt::Key_Escape )
  {
    finish();
  }
  else if ( event->key() == Qt::Key_Enter )
  {
    createPrimitive();
  }
  else if ( event->key() == Qt::Key_Tab || event->key() == Qt::Key_Backtab )
  {
    if ( mShowPrimitiveDialog && !mDone && mCurrentFieldIdx != mDialog->creationParamNumber() )
    {
      mDialog->show();
      qDebug() << u"%1 #%2:"_s.arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "focus on param:" << mCurrentFieldIdx;
      mDialog->focusOnParam( mCurrentFieldIdx );
    }
  }
}

void Qgs3DMapToolCreatePrimitive::handleNextParameter()
{
  updatePrimitive();

  if ( mCurrentFieldIdx < 0 )
  {
    QgsPoint rbPoint( mPointOnMap.last() );
    rbPoint.setZ( rbPoint.z() / mCanvas->mapSettings()->terrainSettings()->verticalScale() );
    mRubberBand->addPoint( rbPoint );
    mRubberBand->addPoint( rbPoint );
  }
  else if ( mCurrentFieldIdx < mDialog->creationParamNumber() )
  {
    QgsPoint rbPoint( mPointOnMap.last() );
    rbPoint.setZ( rbPoint.z() / mCanvas->mapSettings()->terrainSettings()->verticalScale() );
    mRubberBand->addPoint( rbPoint );
  }

  ++mCurrentFieldIdx;

  if ( mCurrentFieldIdx == mDialog->creationParamNumber() )
  {
    mCanvas->setCursor( Qt::WaitCursor );
    mDialog->hide();
    if ( mShowPrimitiveDialog )
    {
      mDialog->show();
      mDialog->focusCreateButton();
    }
  }
}

void Qgs3DMapToolCreatePrimitive::handlePreviousParameter()
{
  // TODO should focus on the right field
  if ( mCurrentFieldIdx >= 0 )
  {
    mCanvas->setCursor( cursor() );
    --mCurrentFieldIdx;
    mRubberBand->removeLastPoint();
    mPointOnMap.removeLast();

    if ( mCurrentFieldIdx < 0 )
    {
      // Cancel
      finish();
    }
  }
}


void Qgs3DMapToolCreatePrimitive::createPrimitive()
{}
