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

#include "qgs3dmaptoolcreatecube.h"

//#include "qgs3dmaptoolcreatecube.moc"
//#include "moc_qgs3dmaptoolcreatecube.cpp"
#include "qgs3dcreatecubedialog.h"
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

#include <QMouseEvent>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QSphereMesh>


Qgs3DMapToolCreateCube::Qgs3DMapToolCreateCube( Qgs3DMapCanvas *canvas )
  : Qgs3DMapTool( canvas )
{
  // Dialog
  mDialog = std::make_unique<Qgs3DCreateCubeDialog>();
}

Qgs3DMapToolCreateCube::~Qgs3DMapToolCreateCube() = default;

void Qgs3DMapToolCreateCube::activate()
{
  qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString();
  restart();

  // Show dialog
  mDialog->show();
}

void Qgs3DMapToolCreateCube::deactivate()
{
  finish();

  // Hide dialog
  mDialog->hide();
}

void Qgs3DMapToolCreateCube::finish()
{
  qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString();
  mRubberBand.reset();
  mPrimitiveLineEntity.reset();
  mHighlightedPointEntity.reset();

  mDone = true;
}

QCursor Qgs3DMapToolCreateCube::cursor() const
{
  return Qt::CrossCursor;
}

void Qgs3DMapToolCreateCube::restart()
{
  qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString();

  mDone = false;
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


QgsPoint Qgs3DMapToolCreateCube::screenToMap( const QPoint &screenPos ) const
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

void Qgs3DMapToolCreateCube::updatePrimitive( const QgsPoint &mapPos, double length, double zRotation )
{
  QgsGeoTransform *transform;
  if ( mPrimitiveLineEntity.get() == nullptr )
  {
    mPrimitiveLineEntity.reset( new Qt3DCore::QEntity( mCanvas->engine()->frameGraph()->rubberBandsRootEntity() ) );
    mPrimitiveLineEntity->setObjectName( "new_primitive" );

    QgsPrivate::Qgs3DWiredMesh *mesh = new QgsPrivate::Qgs3DWiredMesh;
    QgsAABB box = QgsAABB( -0.5f, -0.5f, 0, //
                           0.5f, 0.5f, 1.0 );
    mesh->setVertices( box.verticesForLines() );
    mPrimitiveLineEntity->addComponent( mesh );

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
  }

  transform->setOrigin( mCanvas->mapSettings()->origin() );
  transform->setRotationZ( static_cast<float>( zRotation ) );
  transform->setGeoTranslation( QgsVector3D( mapPos.x(), mapPos.y(), mapPos.z() ) );
  transform->setScale3D( { static_cast<float>( length ), static_cast<float>( length ), static_cast<float>( length ) } );
}

void Qgs3DMapToolCreateCube::updateHLPoint( const QgsPoint &mapPos, const QPoint &screenPos )
{
  QgsGeoTransform *transform;
  if ( mHighlightedPointEntity.get() == nullptr )
  {
    mHighlightedPointEntity.reset( new Qt3DCore::QEntity( mCanvas->engine()->frameGraph()->rubberBandsRootEntity() ) );
    mHighlightedPointEntity->setObjectName( "HL_point" );

    Qt3DExtras::QSphereMesh *mesh = new Qt3DExtras::QSphereMesh( mCanvas->engine()->frameGraph()->rubberBandsRootEntity() );
    mesh->setRadius( 1.0f );
    mesh->setRings( 4 );
    mesh->setSlices( 4 );
    mHighlightedPointEntity->addComponent( mesh );

    Qt3DExtras::QPhongMaterial *material = new Qt3DExtras::QPhongMaterial;
    material->setAmbient( Qt::red );
    mHighlightedPointEntity->addComponent( material );

    transform = new QgsGeoTransform( mHighlightedPointEntity.get() );
    mHighlightedPointEntity->addComponent( transform );
  }
  else
  {
    for ( auto trans : mHighlightedPointEntity->findChildren<QgsGeoTransform *>() )
    {
      transform = trans;
      break;
    }
  }

  transform->setOrigin( mCanvas->mapSettings()->origin() );
  transform->setGeoTranslation( QgsVector3D( mapPos.x(), mapPos.y(), mapPos.z() ) );

  double length = 10;
  qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "HL size:" << length;
  transform->setScale3D( { static_cast<float>( length ), static_cast<float>( length ), static_cast<float>( length ) } );
}

void Qgs3DMapToolCreateCube::handleClick( const QPoint &screenPos )
{
  qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString();
  if ( mDone )
  {
    restart();
  }

  if ( mMouseClickPos == QPoint() )
  {
    qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "First click";
    mMouseClickPos = screenPos;

    mFirstPointOnMap = screenToMap( screenPos );
    updatePrimitive( mFirstPointOnMap, 10.0, 0.0 );

    QgsPoint rbPoint( mFirstPointOnMap );
    rbPoint.setZ( rbPoint.z() / mCanvas->mapSettings()->terrainSettings()->verticalScale() );
    mRubberBand->addPoint( rbPoint );
    mRubberBand->addPoint( rbPoint );
  }
  else
  {
    qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "Second click";
    finish();
  }
}

void Qgs3DMapToolCreateCube::mousePressEvent( QMouseEvent * /*event*/ )
{
}

void Qgs3DMapToolCreateCube::mouseMoveEvent( QMouseEvent *event )
{
  if ( !mMouseHasMoved && ( event->pos() - mMouseClickPos ).manhattanLength() >= QApplication::startDragDistance() )
  {
    mMouseHasMoved = true;
  }

  QgsPoint pointMap = screenToMap( event->pos() );

  QgsPoint rbPoint( pointMap );
  rbPoint.setZ( rbPoint.z() / mCanvas->mapSettings()->terrainSettings()->verticalScale() );

  updateHLPoint( pointMap, event->pos() );

  if ( mPrimitiveLineEntity.get() == nullptr )
  {
    mDialog->setTranslation( pointMap );
  }
  else
  {
    mRubberBand->moveLastPoint( rbPoint );

    double length = pointMap.distance3D( mFirstPointOnMap );
    double angle = -1.0 * QgsGeometryUtilsBase::lineAngle( pointMap.x(), pointMap.y(), mFirstPointOnMap.x(), mFirstPointOnMap.y() );
    angle *= 180.0 / M_PI;
    qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "cube size:" << length << "/ rotation: " << angle;
    updatePrimitive( mFirstPointOnMap, length, angle );
    mDialog->setRotation( 0.0, 0.0, ( angle < 0.0 ? 360.0 + angle : angle ) );
    mDialog->setSize( length );
  }
}

void Qgs3DMapToolCreateCube::mouseReleaseEvent( QMouseEvent *event )
{
  if ( event->button() == Qt::LeftButton )
  {
    handleClick( event->pos() );
  }
  else if ( event->button() == Qt::RightButton )
  {
    if ( mDone )
    {
      restart();
      return;
    }

    // Finish measurement
    finish();
  }
}
