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

#include "qgs3dcreateprimitivedialog.h"
#include "qgs3dutils.h"
#include "qgswindow3dengine.h"
#include "qgsframegraph.h"
#include "qgsrubberband3d.h"
#include "qgsraycastcontext.h"
#include "qgsraycasthit.h"
#include "qgsraycastingutils.h"
#include "qgscameracontroller.h"
#include "qgs3drendercontext.h"
#include "qgsgeotransform.h"
#include "qgs3dcreateprimitivecubedialog.h"
#include "qgs3dcreateprimitivespheredialog.h"
#include "qgs3dcreateprimitivecylinderdialog.h"
#include "qgs3dcreateprimitivetorusdialog.h"
#include "qgs3dcreateprimitiveconedialog.h"

#include <QMouseEvent>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QTorusMesh>
#include <Qt3DExtras/QConeMesh>
#include <Qt3DRender/QRenderSettings>

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
}

Qgs3DMapToolCreatePrimitive::~Qgs3DMapToolCreatePrimitive() = default;

void Qgs3DMapToolCreatePrimitive::activate()
{
  qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString();
  restart();

  // Show dialog
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
  qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString();
  mCanvas->setCursor( cursor() );
  mDialog->unfocusCreateButton();

  mPrimitiveLineEntity.reset();

  mRubberBand.reset();
  mPointOnMap.clear();
  mNbMouseClick = 0;
  mDone = true;
}

QCursor Qgs3DMapToolCreatePrimitive::cursor() const
{
  return Qt::CrossCursor;
}

void Qgs3DMapToolCreatePrimitive::restart()
{
  qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString();

  mDone = false;
  mNbMouseClick = 0;
  mDialog->resetData();
  mMouseClickPos = QPoint();

  mRubberBand.reset( new QgsRubberBand3D( *mCanvas->mapSettings(), mCanvas->engine(), mCanvas->engine()->frameGraph()->rubberBandsRootEntity() ) );
  const QgsSettings settings;
  const int myRed = settings.value( QStringLiteral( "qgis/default_measure_color_red" ), 222 ).toInt();
  const int myGreen = settings.value( QStringLiteral( "qgis/default_measure_color_green" ), 155 ).toInt();
  const int myBlue = settings.value( QStringLiteral( "qgis/default_measure_color_blue" ), 67 ).toInt();
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
        break;
      }
    }
  }


  transform->setOrigin( mCanvas->mapSettings()->origin() );
  transform->setRotationX( static_cast<float>( mDialog->rotX() ) );
  transform->setRotationY( static_cast<float>( mDialog->rotY() ) );
  transform->setRotationZ( static_cast<float>( mDialog->rotZ() ) );
  transform->setGeoTranslation( { static_cast<float>( mDialog->transX() ), static_cast<float>( mDialog->transY() ), static_cast<float>( mDialog->transZ() ) } );
  transform->setScale3D( { static_cast<float>( mDialog->scaleX() * sX ), static_cast<float>( mDialog->scaleY() * sY ), static_cast<float>( mDialog->scaleZ() * sZ ) } );
}

void Qgs3DMapToolCreatePrimitive::handleClick( const QPoint &screenPos )
{
  qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString();
  if ( mDone )
  {
    restart();
  }

  if ( mNbMouseClick == 0 )
  {
    qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "First click";
    mMouseClickPos = screenPos;

    mPointOnMap << screenToMap( screenPos );
    updatePrimitive();

    QgsPoint rbPoint( mPointOnMap[mNbMouseClick] );
    rbPoint.setZ( rbPoint.z() / mCanvas->mapSettings()->terrainSettings()->verticalScale() );
    mRubberBand->addPoint( rbPoint );
    mRubberBand->addPoint( rbPoint );
  }
  else if ( mNbMouseClick < mDialog->paramNumber() )
  {
    QgsPoint rbPoint( mPointOnMap[mNbMouseClick - 1] );
    rbPoint.setZ( rbPoint.z() / mCanvas->mapSettings()->terrainSettings()->verticalScale() );
    mRubberBand->addPoint( rbPoint );
    mPointOnMap << screenToMap( screenPos );
  }
  else
  {
    qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "Second click";
    finish();
  }

  ++mNbMouseClick;
}

void Qgs3DMapToolCreatePrimitive::mousePressEvent( QMouseEvent * /*event*/ )
{
}

void Qgs3DMapToolCreatePrimitive::mouseMoveEvent( QMouseEvent *event )
{
  if ( !mMouseHasMoved && ( event->pos() - mMouseClickPos ).manhattanLength() >= QApplication::startDragDistance() )
  {
    mMouseHasMoved = true;
  }

  QgsPoint pointMap = screenToMap( event->pos() );

  QgsPoint rbPoint( pointMap );
  rbPoint.setZ( rbPoint.z() / mCanvas->mapSettings()->terrainSettings()->verticalScale() );

  if ( mNbMouseClick == 0 )
  {
    mDialog->setTranslation( pointMap );
  }
  else if ( mNbMouseClick < mDialog->paramNumber() )
  {
    QgsPoint rbPoint( pointMap );
    QgsPoint prevPointMap = mPointOnMap.last();
    if ( mNbMouseClick == 1 )
    {
      double angle = -1.0 * QgsGeometryUtilsBase::lineAngle( pointMap.x(), pointMap.y(), prevPointMap.x(), prevPointMap.y() );
      angle *= 180.0 / M_PI;
      qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "prim rotation: " << angle;

      mDialog->setRotation( mDialog->rotX(), mDialog->rotY(), ( angle < 0.0 ? 360.0 + angle : angle ) );
    }

    double length;
    Qgs3DCreatePrimitiveDialog::ConstrainedAxis constraint = Qgs3DCreatePrimitiveDialog::NONE;
    if ( ( event->modifiers() & Qt::Modifier::CTRL ) == 0 )
      constraint = Qgs3DCreatePrimitiveDialog::NONE;
    else
      constraint = mDialog->constrainedAxisForParam( mNbMouseClick - 1 );

    switch ( constraint )
    {
      case Qgs3DCreatePrimitiveDialog::NONE:
        length = prevPointMap.distance3D( pointMap );
        break;
      case Qgs3DCreatePrimitiveDialog::X:
        length = std::abs( prevPointMap.x() - pointMap.x() );
        rbPoint.setY( prevPointMap.y() );
        rbPoint.setZ( prevPointMap.z() );
        break;
      case Qgs3DCreatePrimitiveDialog::Y:
        length = std::abs( prevPointMap.y() - pointMap.y() );
        rbPoint.setX( prevPointMap.x() );
        rbPoint.setZ( prevPointMap.z() );
        break;
      case Qgs3DCreatePrimitiveDialog::Z:
        length = std::abs( prevPointMap.z() - pointMap.z() );
        rbPoint.setX( prevPointMap.x() );
        rbPoint.setY( prevPointMap.y() );
        break;
      case Qgs3DCreatePrimitiveDialog::XY:
        length = std::sqrt( std::pow( prevPointMap.x() - pointMap.x(), 2 ) + std::pow( prevPointMap.y() - pointMap.y(), 2 ) );
        rbPoint.setZ( prevPointMap.z() );
        break;
      case Qgs3DCreatePrimitiveDialog::XZ:
        length = std::sqrt( std::pow( prevPointMap.x() - pointMap.x(), 2 ) + std::pow( prevPointMap.z() - pointMap.z(), 2 ) );
        rbPoint.setY( prevPointMap.y() );
        break;
      case Qgs3DCreatePrimitiveDialog::YZ:
        length = std::sqrt( std::pow( prevPointMap.z() - pointMap.z(), 2 ) + std::pow( prevPointMap.y() - pointMap.y(), 2 ) );
        rbPoint.setX( prevPointMap.x() );
        break;
    }

    rbPoint.setZ( rbPoint.z() / mCanvas->mapSettings()->terrainSettings()->verticalScale() );
    mRubberBand->moveLastPoint( rbPoint );

    mDialog->setParam( mNbMouseClick - 1, length );

    updatePrimitive();
  }
}

void Qgs3DMapToolCreatePrimitive::mouseReleaseEvent( QMouseEvent *event )
{
  if ( event->button() == Qt::LeftButton )
  {
    handleClick( event->pos() );
  }
  else if ( event->button() == Qt::RightButton )
  {
    // TODO revertLastClick();

    if ( mNbMouseClick == 0 )
    {
      // Finish measurement
      finish();
    }
    else
    {
      --mNbMouseClick;
      mRubberBand->removeLastPoint();
    }
    // if ( mDone )
    // {
    //   restart();
    //   return;
    // }

    // // Finish measurement
    // finish();
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

void Qgs3DMapToolCreatePrimitive::createPrimitive( bool enabledAttributeValuesDlg )
{
}
