/***************************************************************************
  qgs3dmaptoolstreetview.cpp
  --------------------------------------
  Date                 : May 2026
  Copyright            : (C) 2026 by Benoit De Mezzo
  Email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgs3dmaptoolstreetview.h"

#include "qgs3dmapcanvas.h"
#include "qgs3dmapscene.h"
#include "qgs3drendercontext.h"
#include "qgs3dutils.h"
#include "qgsabstractterrainsettings.h"
#include "qgscameracontroller.h"
#include "qgsframegraph.h"
#include "qgsmaplayer.h"
#include "qgspoint.h"
#include "qgsraycastcontext.h"
#include "qgsrubberband3d.h"
#include "qgssettingsentryimpl.h"
#include "qgssettingsregistrygui.h"
#include "qgswindow3dengine.h"

#include <QKeyEvent>

#include "moc_qgs3dmaptoolstreetview.cpp"

Qgs3DMapToolStreetView::Qgs3DMapToolStreetView( Qgs3DMapCanvas *canvas )
  : Qgs3DMapTool( canvas )
  , mIsNavigating( false )
  , mSelectedPos( QgsPoint() )
  , mLastTime( QTime::currentTime() )
{}

Qgs3DMapToolStreetView::~Qgs3DMapToolStreetView() = default;

void Qgs3DMapToolStreetView::activate()
{
  mRubberBand.reset( new QgsRubberBand3D( *mCanvas->mapSettings(), mCanvas->engine(), mCanvas->engine()->frameGraph()->rubberBandsRootEntity() ) );

  restart();
  updateSettings();
}

void Qgs3DMapToolStreetView::deactivate()
{
  mRubberBand.reset();
}

QCursor Qgs3DMapToolStreetView::cursor() const
{
  return Qt::DragMoveCursor;
}

void Qgs3DMapToolStreetView::handleClick( const QPoint &screenPos )
{
  QgsRayCastContext context;
  context.setSingleResult( false );
  context.setMaximumDistance( mCanvas->cameraController()->camera()->farPlane() );
  context.setAngleThreshold( 0.5f );
  const QgsRayCastResult results = mCanvas->castRay( screenPos, context );

  if ( results.isEmpty() )
    return;

  QgsVector3D mapCoords;
  double minDist = -1;
  for ( const QgsRayCastHit &hit : results.allHits() )
  {
    const double resDist = hit.distance();
    if ( minDist < 0 || resDist < minDist )
    {
      minDist = resDist;
      mapCoords = hit.mapCoordinates();
    }
  }
  setupPinPoint( QgsPoint( mapCoords.x(), mapCoords.y(), 0.0f ) );
}

void Qgs3DMapToolStreetView::updateSettings()
{
  if ( mRubberBand )
  {
    mRubberBand->setWidth( 3 );
    mRubberBand->setColor( QgsSettingsRegistryGui::settingsDefaultMeasureColor->value() );
  }
}

void Qgs3DMapToolStreetView::updateNavigationCamera( const QgsPoint &newCamPosInMap )
{
  qDebug() << "center:" << mCanvas->cameraController()->cameraPose().centerPoint().toString( 1 );
  qDebug() << "dist:" << mCanvas->cameraController()->cameraPose().distanceFromCenterPoint();
  qDebug() << "headingAngle:" << mCanvas->cameraController()->cameraPose().headingAngle();
  qDebug() << "pitchAngle:" << mCanvas->cameraController()->cameraPose().pitchAngle();
  QgsVector3D curCamCenterInMap = Qgs3DUtils::worldToMapCoordinates( QgsVector3D( mCanvas->camera()->viewCenter() ), mCanvas->mapSettings()->origin() );
  QgsVector3D curCamPositionInMap = Qgs3DUtils::worldToMapCoordinates( QgsVector3D( mCanvas->camera()->position() ), mCanvas->mapSettings()->origin() );

  QgsVector3D viewVectorInMap( curCamCenterInMap - curCamPositionInMap );

  // TODO: fails when vertical scale is not 1.0
  QgsPoint camPosInMap( newCamPosInMap.x(), newCamPosInMap.y(), 0.0f );
  float zFromTerrain = canvas()->mapSettings()->terrainGenerator()->heightAt( camPosInMap.x(), camPosInMap.y(), Qgs3DRenderContext() );
  // apply vertical scale and offset
  zFromTerrain = ( zFromTerrain + canvas()->mapSettings()->terrainSettings()->elevationOffset() ) * canvas()->mapSettings()->terrainSettings()->verticalScale();
  camPosInMap.setZ( 2.0 + zFromTerrain );

  QgsPoint lookAtInMap( camPosInMap.x() + viewVectorInMap.x(), camPosInMap.y() + viewVectorInMap.y(), 0.0f );
  zFromTerrain = canvas()->mapSettings()->terrainGenerator()->heightAt( lookAtInMap.x(), lookAtInMap.y(), Qgs3DRenderContext() );
  // apply vertical scale and offset
  zFromTerrain = ( zFromTerrain + canvas()->mapSettings()->terrainSettings()->elevationOffset() ) * canvas()->mapSettings()->terrainSettings()->verticalScale();
  lookAtInMap.setZ( 0.5 + zFromTerrain );

  QgsVector3D lookAtInWorld = Qgs3DUtils::mapToWorldCoordinates( QgsVector3D( lookAtInMap.x(), lookAtInMap.y(), lookAtInMap.z() ), mCanvas->mapSettings()->origin() );
  //lookAtInWorld.setZ( zFromTerrain );
  QgsVector3D camPosInWorld = Qgs3DUtils::mapToWorldCoordinates( QgsVector3D( camPosInMap.x(), camPosInMap.y(), camPosInMap.z() ), mCanvas->mapSettings()->origin() );
  //lookAtInWorld.setZ( mCanvas->camera()->viewCenter().z() );

  QgsCameraPose camPose; // (mCanvas->cameraController()->cameraPose());
  float heading = -camPosInMap.azimuth( lookAtInMap );
  float pitch = 180 - camPosInMap.inclination( lookAtInMap );

  qDebug() << "2 center:" << lookAtInWorld.toString( 1 );
  qDebug() << "2 dist:" << camPosInWorld.distance( lookAtInWorld );
  qDebug() << "2 headingAngle:" << heading;
  qDebug() << "2 pitchAngle:" << pitch;

  camPose.setCenterPoint( lookAtInWorld );
  camPose.setDistanceFromCenterPoint( camPosInWorld.distance( lookAtInWorld ) );
  camPose.setPitchAngle( pitch );
  camPose.setHeadingAngle( heading );
  mCanvas->cameraController()->setCameraPose( camPose );
}

void Qgs3DMapToolStreetView::setupNavigation()
{
  mIsNavigating = true;
  mPreviousCameraPose = mCanvas->cameraController()->cameraPose();
  mCanvas->cameraController()->setInputHandlersEnabled( false );
  updateNavigationCamera( mSelectedPos );

  mRubberBand->reset();
  mRubberBand->setHideLastMarker( true );
}

void Qgs3DMapToolStreetView::setupPinPoint( const QgsPoint &point )
{
  mRubberBand->reset();

  mSelectedPos = point;
  float zFromTerrain = canvas()->mapSettings()->terrainGenerator()->heightAt( mSelectedPos.x(), mSelectedPos.y(), Qgs3DRenderContext() );
  mSelectedPos.setZ( zFromTerrain );

  QgsPoint newPoint( mSelectedPos );
  mRubberBand->addPoint( newPoint );

  newPoint.setZ( newPoint.z() + 30.0 );
  mRubberBand->addPoint( newPoint );
}

void Qgs3DMapToolStreetView::restart()
{
  mRubberBand->reset();
  mRubberBand->setHideLastMarker( true );
  mLastTime = QTime::currentTime();
  mSelectedPos = QgsPoint();
  mIsNavigating = false;
}

// void Qgs3DMapToolStreetView::mousePressEvent( QMouseEvent *event )
// {
// }

void Qgs3DMapToolStreetView::mouseMoveEvent( QMouseEvent *event )
{
  if ( mIsNavigating )
  {
    // QPoint middle( mCanvas->parentWidget()->width() / 2, mCanvas->parentWidget()->height() / 2 );
    // if ( event->pos() != middle )
    // {
    mCanvas->cameraController()->rotateCamera(
      0.1 * ( event->pos().y() - mMouseMovePos.y() ), //
      0.1 * ( event->pos().x() - mMouseMovePos.x() )
    );
    //   QCursor::setPos( middle.x(), middle.y() );
    // }
  }
  else
  {
    QTime ct = QTime::currentTime();
    if ( mLastTime.msecsTo( ct ) < 100 )
      return;

    mLastTime = ct;

    handleClick( event->pos() );
  }
  mMouseMovePos = event->pos();
}

void Qgs3DMapToolStreetView::quit()
{
  if ( mIsNavigating )
  {
    mCanvas->cameraController()->setInputHandlersEnabled( true );
    mCanvas->cameraController()->setCameraPose( mPreviousCameraPose );
  }
  restart();
  emit finished();
}

void Qgs3DMapToolStreetView::mouseReleaseEvent( QMouseEvent *event )
{
  if ( event->button() == Qt::RightButton )
  {
    quit();
  }
  else if ( event->button() == Qt::LeftButton && !mIsNavigating )
  {
    setupNavigation();
  }
}

void Qgs3DMapToolStreetView::navigateOnX( float steps )
{
  QgsVector3D curCamCenterInMap = Qgs3DUtils::worldToMapCoordinates( QgsVector3D( mCanvas->camera()->viewCenter() ), mCanvas->mapSettings()->origin() );
  QgsVector3D curCamPositionInMap = Qgs3DUtils::worldToMapCoordinates( QgsVector3D( mCanvas->camera()->position() ), mCanvas->mapSettings()->origin() );

  QgsVector3D viewVectorInMap( curCamCenterInMap - curCamPositionInMap );
  viewVectorInMap.normalize();

  QgsVector3D newCamCenterInMap( curCamPositionInMap - ( viewVectorInMap * steps ) );

  updateNavigationCamera( QgsPoint( newCamCenterInMap.x(), newCamCenterInMap.y(), 0.0f ) );
}

void Qgs3DMapToolStreetView::mouseWheelEvent( QWheelEvent *event )
{
  if ( mIsNavigating )
  {
    navigateOnX( 0.1 * event->angleDelta().y() );
  }
}

void Qgs3DMapToolStreetView::keyPressEvent( QKeyEvent *event )
{
  if ( event->key() == Qt::Key_Escape )
  {
    quit();
  }
  else if ( event->key() == Qt::Key_Enter && !mIsNavigating )
  {
    setupNavigation();
  }
  else if ( mIsNavigating )
  {
    if ( event->key() == Qt::Key_Up )
    {
      navigateOnX( +10 );
    }
    else if ( event->key() == Qt::Key_Down )
    {
      navigateOnX( -10 );
    }
  }
}
