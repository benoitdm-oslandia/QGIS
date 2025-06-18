/***************************************************************************
  qgs3dmaptoolstreetview.cpp
  --------------------------------------
  Date                 : Jun 2019
  Copyright            : (C) 2019 by Ismail Sunni
  Email                : imajimatika at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QKeyEvent>

#include "qgs3dmaptoolstreetview.h"
#include "moc_qgs3dmaptoolstreetview.cpp"
#include "qgs3dutils.h"
#include "qgs3dmapscene.h"
#include "qgs3dmapcanvas.h"
#include "qgspoint.h"
#include "qgsmaplayer.h"
#include "qgs3dmeasuredialog.h"
#include "qgsrubberband3d.h"
#include "qgswindow3dengine.h"
#include "qgsframegraph.h"
#include "qgsabstractterrainsettings.h"
#include "qgs3drendercontext.h"
#include <QTimer>
#include <QGuiApplication>
#ifdef Q_OS_MAC
#include <ApplicationServices/ApplicationServices.h>
#endif

Qgs3DMapToolStreetView::Qgs3DMapToolStreetView( Qgs3DMapCanvas *canvas )
  : Qgs3DMapTool( canvas )
  , mIsOptimal( false )
  , mIsNavigating( false )
  , mIsNavigationPaused( false )
  , mIsEnabled( false )
  , mIgnoreNextMouseMove( false )
  , mPreviousCameraPose()
  , mMarkerPos( QgsPoint() )
  , mLastMarkerTime( QTime::currentTime() )
  , mJumpTime( QTime::currentTime() )
{
#if defined( Q_OS_MAC )
  mIsOptimal = AXIsProcessTrusted();
#elif defined( Q_OS_LINUX )
  mIsOptimal = QString( getenv( "XDG_SESSION_TYPE" ) ) == "x11" && QString( getenv( "XRDP_SESSION" ) ) == "";
#elif defined( Q_OS_WINDOWS )
  mIsOptimal = true;
#else
  mIsOptimal = false;
#endif

  QgsDebugError( QString( "Qgs3DMapToolStreetView::Qgs3DMapToolStreetView mIsOptimal=%1" ).arg( mIsOptimal ) );
  mJumpTimer = new QTimer( this );
  connect( mJumpTimer, &QTimer::timeout, this, qOverload<>( &Qgs3DMapToolStreetView::refreshCameraForJump ) );
}

Qgs3DMapToolStreetView::~Qgs3DMapToolStreetView() = default;

void Qgs3DMapToolStreetView::activate()
{
  if ( !mIsEnabled )
  {
    reset();
    updateSettings();
    mIsEnabled = true;

    if ( mIsOptimal )
    {
      QPoint middle( mCanvas->width() / 2, mCanvas->height() / 2 );
      QPoint middleG = mCanvas->mapToGlobal( middle );
      QCursor::setPos( middleG.x(), middleG.y() );
    }
  }
}

void Qgs3DMapToolStreetView::deactivate()
{
  if ( mIsEnabled )
  {
    if ( mIsNavigating )
    {
      mCanvas->cameraController()->setInputHandlersEnabled( true );
      mCanvas->cameraController()->setCameraPose( mPreviousCameraPose );
    }

    mJumpTimer->stop();
    if ( mRubberBand.get() )
      mRubberBand->reset(); // clean remaining entities

    // to be done here to delete the children of rubberbond entity before the framegraph destructor
    // which will delete the rubberbond entity parent and all its children
    mRubberBand.reset();

    mIsEnabled = false;
    emit finished();
  }
}

QCursor Qgs3DMapToolStreetView::cursor() const
{
  return Qt::WhatsThisCursor;
}

void Qgs3DMapToolStreetView::handleMarkerMove( const QPoint &screenPos )
{
  QTime ct = QTime::currentTime();
  if ( mLastMarkerTime.msecsTo( ct ) < 100 )
    return;

  mLastMarkerTime = ct;

  // convert screen pos to map coordinates
  const QgsRay3D ray = Qgs3DUtils::rayFromScreenPoint( screenPos, mCanvas->size(), mCanvas->cameraController()->camera() );
  const QHash<QgsMapLayer *, QVector<QgsRayCastingUtils::RayHit>> allHits = Qgs3DUtils::castRay( mCanvas->scene(), ray, QgsRayCastingUtils::RayCastContext( true, mCanvas->size(), mCanvas->cameraController()->camera()->farPlane() ) );

  if ( allHits.isEmpty() )
    return;

  QgsVector3D worldIntersection;
  float minDist = -1;
  for ( const QVector<QgsRayCastingUtils::RayHit> &results : allHits )
  {
    const QgsRayCastingUtils::RayHit &result = results.first();
    const float resDist = result.distance;
    if ( minDist < 0 || resDist < minDist )
    {
      minDist = resDist;
      worldIntersection = QgsVector3D( result.pos.x(), result.pos.y(), result.pos.z() );
    }
  }
  const QgsVector3D mapCoords = Qgs3DUtils::worldToMapCoordinates( worldIntersection, mCanvas->mapSettings()->origin() );

  // update rubberband
  mRubberBand->reset();

  mMarkerPos = QgsPoint( mapCoords.x(), mapCoords.y(), 0.0f );
  float zFromTerrain = canvas()->mapSettings()->terrainGenerator()->heightAt( mMarkerPos.x(), mMarkerPos.y(), Qgs3DRenderContext() );
  mMarkerPos.setZ( zFromTerrain );

  QgsPoint newPoint( mMarkerPos );
  mRubberBand->addPoint( newPoint );

  newPoint.setZ( newPoint.z() + 30.0 );
  mRubberBand->addPoint( newPoint );
}

void Qgs3DMapToolStreetView::updateSettings()
{
  if ( mRubberBand )
  {
    const QgsSettings settings;
    const int myRed = settings.value( QStringLiteral( "qgis/default_measure_color_red" ), 222 ).toInt();
    const int myGreen = settings.value( QStringLiteral( "qgis/default_measure_color_green" ), 155 ).toInt();
    const int myBlue = settings.value( QStringLiteral( "qgis/default_measure_color_blue" ), 67 ).toInt();

    mRubberBand->setWidth( 3 );
    mRubberBand->setColor( QColor( myRed, myGreen, myBlue ) );
  }
}

void Qgs3DMapToolStreetView::updateNavigationCamera( const QgsPoint &newCamPosInMap )
{
  Qgs3DUtils::waitForFrame( *mCanvas->engine(), mCanvas->scene() );

  qDebug() << "updateNavigationCamera before center:" << mCanvas->cameraController()->cameraPose().centerPoint().toString( 1 );
  qDebug() << "updateNavigationCamera before dist:" << mCanvas->cameraController()->cameraPose().distanceFromCenterPoint();
  qDebug() << "updateNavigationCamera before headingAngle:" << mCanvas->cameraController()->cameraPose().headingAngle();
  qDebug() << "updateNavigationCamera before pitchAngle:" << mCanvas->cameraController()->cameraPose().pitchAngle();
  QgsVector3D curCamCenterInMap = Qgs3DUtils::worldToMapCoordinates( QgsVector3D( mCanvas->camera()->viewCenter() ), mCanvas->mapSettings()->origin() );
  QgsVector3D curCamPositionInMap = Qgs3DUtils::worldToMapCoordinates( QgsVector3D( mCanvas->camera()->position() ), mCanvas->mapSettings()->origin() );

  QgsVector3D viewVectorInMap( curCamCenterInMap - curCamPositionInMap );

  double jump = jumpHeight( mJumpTime.msecsTo( QTime::currentTime() ) );
  qDebug() << "updateNavigationCamera before jump:" << jump;

  // TODO: fails when vertical scale is not 1.0
  QgsPoint camPosInMap( newCamPosInMap.x(), newCamPosInMap.y(), 0.0f );
  float zFromTerrain = canvas()->mapSettings()->terrainGenerator()->heightAt( camPosInMap.x(), camPosInMap.y(), Qgs3DRenderContext() );
  // apply vertical scale and offset
  zFromTerrain = ( zFromTerrain + canvas()->mapSettings()->terrainSettings()->elevationOffset() ) * canvas()->mapSettings()->terrainSettings()->verticalScale();
  camPosInMap.setZ( 2.0 + jump + zFromTerrain );

  QgsPoint lookAtInMap( camPosInMap.x() + viewVectorInMap.x(), camPosInMap.y() + viewVectorInMap.y(), 0.0f );
  zFromTerrain = canvas()->mapSettings()->terrainGenerator()->heightAt( lookAtInMap.x(), lookAtInMap.y(), Qgs3DRenderContext() );
  // apply vertical scale and offset
  zFromTerrain = ( zFromTerrain + canvas()->mapSettings()->terrainSettings()->elevationOffset() ) * canvas()->mapSettings()->terrainSettings()->verticalScale();
  lookAtInMap.setZ( 0.5 + jump + zFromTerrain );

  QgsVector3D lookAtInWorld = Qgs3DUtils::mapToWorldCoordinates(
    QgsVector3D( lookAtInMap.x(), lookAtInMap.y(), lookAtInMap.z() ),
    mCanvas->mapSettings()->origin()
  );
  //lookAtInWorld.setZ( zFromTerrain );
  QgsVector3D camPosInWorld = Qgs3DUtils::mapToWorldCoordinates(
    QgsVector3D( camPosInMap.x(), camPosInMap.y(), camPosInMap.z() ),
    mCanvas->mapSettings()->origin()
  );
  //lookAtInWorld.setZ( mCanvas->camera()->viewCenter().z() );

  QgsCameraPose camPose; // (mCanvas->cameraController()->cameraPose());
  float heading = -camPosInMap.azimuth( lookAtInMap );
  float pitch = 180 - camPosInMap.inclination( lookAtInMap );

  qDebug() << "updateNavigationCamera after center:" << lookAtInWorld.toString( 1 );
  qDebug() << "updateNavigationCamera after dist:" << camPosInWorld.distance( lookAtInWorld );
  qDebug() << "updateNavigationCamera after headingAngle:" << heading;
  qDebug() << "updateNavigationCamera after pitchAngle:" << pitch;

  camPose.setCenterPoint( lookAtInWorld );
  camPose.setDistanceFromCenterPoint( camPosInWorld.distance( lookAtInWorld ) );
  camPose.setPitchAngle( pitch );
  camPose.setHeadingAngle( heading );
  mCanvas->cameraController()->setCameraPose( camPose );

  mLastCamPosInMap = camPosInMap;
}

void Qgs3DMapToolStreetView::refreshCameraForJump()
{
  double jump = jumpHeight( mJumpTime.msecsTo( QTime::currentTime() ) );
  if ( jump )
  {
    updateNavigationCamera( mLastCamPosInMap );
  }
  else
  {
    if ( mJumpTimer->isActive() ) // last one refresh
      updateNavigationCamera( mLastCamPosInMap );
    mJumpTimer->stop();
  }
}

void Qgs3DMapToolStreetView::setupNavigation()
{
  mIsNavigating = true;
  mIsNavigationPaused = false;
  mPreviousCameraPose = mCanvas->cameraController()->cameraPose();
  mCanvas->cameraController()->setInputHandlersEnabled( false );
  updateNavigationCamera( mMarkerPos );

  mRubberBand->reset();
  mRubberBand->setHideLastMarker( true );

  if ( mIsOptimal )
  {
    QPoint middle( mCanvas->width() / 2, mCanvas->height() / 2 );
    QPoint middleG = mCanvas->mapToGlobal( middle );
    mIgnoreNextMouseMove = true;
    QCursor::setPos( middleG.x(), middleG.y() );
  }

  mCanvas->setCursor( Qt::CrossCursor );
}

void Qgs3DMapToolStreetView::reset()
{
  mJumpTimer->stop();
  if ( mRubberBand.get() )
    mRubberBand->reset(); // clean remaining entities
  mRubberBand.reset( new QgsRubberBand3D( *mCanvas->mapSettings(), mCanvas->engine(), mCanvas->engine()->frameGraph()->rubberBandsRootEntity() ) );
  mRubberBand->setHideLastMarker( true );
  mLastMarkerTime = QTime::currentTime();
  mMarkerPos = QgsPoint();
  mIsNavigating = false;
}

void Qgs3DMapToolStreetView::navigateForward( float steps )
{
  QgsVector3D curCamCenterInMap = Qgs3DUtils::worldToMapCoordinates( QgsVector3D( mCanvas->camera()->viewCenter() ), mCanvas->mapSettings()->origin() );
  QgsVector3D curCamPositionInMap = Qgs3DUtils::worldToMapCoordinates( QgsVector3D( mCanvas->camera()->position() ), mCanvas->mapSettings()->origin() );

  QgsVector3D viewVectorInMap( curCamCenterInMap - curCamPositionInMap );
  viewVectorInMap.normalize();

  QgsVector3D newCamCenterInMap( curCamPositionInMap + ( viewVectorInMap * steps ) );

  updateNavigationCamera( QgsPoint( newCamCenterInMap.x(), newCamCenterInMap.y(), 0.0f ) );
}

void Qgs3DMapToolStreetView::navigateRightSide( float steps )
{
  QgsVector3D curCamCenterInMap = Qgs3DUtils::worldToMapCoordinates( QgsVector3D( mCanvas->camera()->viewCenter() ), mCanvas->mapSettings()->origin() );
  QgsVector3D curCamPositionInMap = Qgs3DUtils::worldToMapCoordinates( QgsVector3D( mCanvas->camera()->position() ), mCanvas->mapSettings()->origin() );

  QgsVector3D viewVectorInMap( curCamCenterInMap - curCamPositionInMap );
  viewVectorInMap.normalize();
  // perpendicular vector
  double y = viewVectorInMap.y();
  viewVectorInMap.setY( -viewVectorInMap.x() );
  viewVectorInMap.setX( y );
  viewVectorInMap.setZ( 0.0 );

  QgsVector3D newCamCenterInMap( curCamPositionInMap + ( viewVectorInMap * steps ) );

  updateNavigationCamera( QgsPoint( newCamCenterInMap.x(), newCamCenterInMap.y(), 0.0f ) );
}


void Qgs3DMapToolStreetView::mouseMoveEvent( QMouseEvent *event )
{
  if ( mIsNavigating )
  {
    if ( !mIsNavigationPaused )
    {
      QPoint middle;
      if ( mIsOptimal )
      {
        middle = QPoint( mCanvas->width() / 2, mCanvas->height() / 2 );
      }
      else
      {
        middle = mLasMousePos;
      }
      QPoint evPos = event->pos();

      if ( mIgnoreNextMouseMove )
      {
        mIgnoreNextMouseMove = false;
      }
      else
      {
        if ( evPos != middle )
        {
          evPos -= middle;
          if ( mIsOptimal )
            evPos *= 0.2;
          else
            evPos *= 0.5;

          mCanvas->cameraController()->rotateCamera( -evPos.y(), -evPos.x() );

          if ( mIsOptimal )
          {
            mIgnoreNextMouseMove = true;
            QPoint middleG = mCanvas->mapToGlobal( middle );
            QCursor::setPos( middleG.x(), middleG.y() );
          }
        }
      }
    }
  }
  else
  {
    handleMarkerMove( event->pos() );
  }

  mLasMousePos = event->pos();
}

void Qgs3DMapToolStreetView::mouseReleaseEvent( QMouseEvent *event )
{
  if ( event->button() == Qt::RightButton )
  {
    deactivate();
  }
  else if ( event->button() == Qt::LeftButton )
  {
    if ( mIsNavigating )
    {
      mIsNavigationPaused ^= true;
      if ( mIsNavigationPaused )
        mCanvas->setCursor( Qt::WaitCursor );
      else
        mCanvas->setCursor( Qt::CrossCursor );
    }
    else
    {
      setupNavigation();
    }
  }
}

void Qgs3DMapToolStreetView::mouseWheelEvent( QWheelEvent *event )
{
  if ( mIsNavigating && !mIsNavigationPaused )
  {
    double speed = 1.0;
    if ( event->modifiers() == Qt::ControlModifier )
      speed = 0.1;
    else if ( event->modifiers() == Qt::AltModifier )
      speed = 10.0;

    if ( event->angleDelta().y() != 0 )
      navigateForward( -0.1 * speed * event->angleDelta().y() );
    if ( event->angleDelta().x() != 0 )
      navigateRightSide( 0.1 * speed * event->angleDelta().x() );
  }
}

void Qgs3DMapToolStreetView::keyPressEvent( QKeyEvent *event )
{
  if ( event->key() == Qt::Key_Escape )
  {
    deactivate();
  }
  else if ( event->key() == Qt::Key_Enter )
  {
    if ( mIsNavigating )
    {
      mIsNavigationPaused ^= true;
      if ( mIsNavigationPaused )
        mCanvas->setCursor( Qt::WaitCursor );
      else
        mCanvas->setCursor( Qt::CrossCursor );
    }
    else
    {
      setupNavigation();
    }
  }
  else if ( mIsNavigating && !mIsNavigationPaused )
  {
    double speed = 1.0;
    if ( event->modifiers() == Qt::ControlModifier )
      speed = 0.1;
    else if ( event->modifiers() == Qt::AltModifier )
      speed = 10.0;

    if ( event->key() == Qt::Key_Space )
    {
      double t = mJumpTime.msecsTo( QTime::currentTime() );
      double h = jumpHeight( t );
      qDebug() << "keyPressEvent cur jump:" << h;
      if ( h == 0.0 ) // timeout, reset
      {
        mJumpAgainHeight = 0.0;
        mJumpAgainTime = 0.0;
        mJumpTime = QTime::currentTime();
      }
      else
      {
        mJumpAgainTime = t;
        mJumpAgainHeight = h;
      }
      qDebug() << "keyPressEvent mJumpAgainHeight:" << mJumpAgainHeight;
      qDebug() << "keyPressEvent mJumpAgainTime:" << mJumpAgainTime;
      qDebug() << "keyPressEvent mJumpTime:" << mJumpTime;
      mJumpTimer->start( 100 );
    }
    else if ( event->key() == Qt::Key_Up )
    {
      navigateForward( speed * 10 );
    }
    else if ( event->key() == Qt::Key_Down )
    {
      navigateForward( speed * -10 );
    }
    else if ( event->key() == Qt::Key_Left )
    {
      navigateRightSide( speed * -10 );
    }
    else if ( event->key() == Qt::Key_Right )
    {
      navigateRightSide( speed * 10 );
    }
  }
}

double Qgs3DMapToolStreetView::jumpHeight( double t )
{
  double h = mJumpDefaultHeight - std::pow( ( t - mJumpAgainTime ) / mJumpDefaultTime - std::sqrt( mJumpDefaultHeight ), 2.0 ) + mJumpAgainHeight;
  return std::max( 0.0, h );
}
