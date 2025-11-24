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

//#include "qgs3dmaptoolcreatecube.moc"
//#include "moc_qgs3dmaptoolcreatecube.cpp"
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

Qgs3DMapToolCreatePrimitive::Qgs3DMapToolCreatePrimitive( Qgs3DMapCanvas *canvas, const QString &type )
  : Qgs3DMapTool( canvas ), mType( type )
{
  // Dialog
  if ( mType == "cube" )
    mDialog.reset( new Qgs3DCreatePrimitiveCubeDialog() );
  else if ( mType == "sphere" )
    mDialog.reset( new Qgs3DCreatePrimitiveSphereDialog() );
  else if ( mType == "cylinder" )
    mDialog.reset( new Qgs3DCreatePrimitiveCylinderDialog() );
  else if ( mType == "torus" )
    mDialog.reset( new Qgs3DCreatePrimitiveTorusDialog() );
  else if ( mType == "cone" )
    mDialog.reset( new Qgs3DCreatePrimitiveConeDialog() );

  connect( mDialog.get(), &Qgs3DCreatePrimitiveDialog::valueChanged, this, [this]() { updatePrimitive(); } );
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

    if ( mType == "cube" )
    {
      QgsPrivate::Qgs3DWiredMesh *mesh = new QgsPrivate::Qgs3DWiredMesh;
      QgsAABB box = QgsAABB( -0.5f, -0.5f, 0, //
                             0.5f, 0.5f, 1.0 );
      mesh->setVertices( box.verticesForLines() );
      mPrimitiveLineEntity->addComponent( mesh );
      mCurrentMesh = mesh;
    }
    else if ( mType == "sphere" )
    {
      Qt3DExtras::QSphereMesh *mesh = new Qt3DExtras::QSphereMesh();
      mesh->setRadius( 0.5 );
      mesh->setRings( 6 );
      mesh->setSlices( 6 );
      mPrimitiveLineEntity->addComponent( mesh );
      mCurrentMesh = mesh;
    }
    else if ( mType == "cylinder" )
    {
      Qt3DExtras::QCylinderMesh *mesh = new Qt3DExtras::QCylinderMesh();
      mesh->setRadius( 0.5 );
      mesh->setLength( 1.0 );
      mesh->setRings( 6 );
      mesh->setSlices( 6 );
      mPrimitiveLineEntity->addComponent( mesh );
      mCurrentMesh = mesh;
    }
    else if ( mType == "torus" )
    {
      Qt3DExtras::QTorusMesh *mesh = new Qt3DExtras::QTorusMesh();
      mesh->setRadius( 0.5 );
      mesh->setMinorRadius( 0.5 );
      mesh->setRings( 6 );
      mesh->setSlices( 6 );
      mPrimitiveLineEntity->addComponent( mesh );
      mCurrentMesh = mesh;
    }
    else if ( mType == "cone" )
    {
      Qt3DExtras::QConeMesh *mesh = new Qt3DExtras::QConeMesh();
      mesh->setBottomRadius( 0.5 );
      mesh->setLength( 1.0 );
      mesh->setTopRadius( 0.5 );
      mesh->setRings( 6 );
      mesh->setSlices( 6 );
      mPrimitiveLineEntity->addComponent( mesh );
      mCurrentMesh = mesh;
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

    if ( mType == "cube" )
    {
      sX = mDialog->getParam( 0 );
      sY = mDialog->getParam( 1 );
      sZ = mDialog->getParam( 2 );
    }
    else if ( mType == "sphere" )
    {
      // Qgs3DCreatePrimitiveSphereDialog * dialog = dynamic_cast<Qgs3DCreatePrimitiveSphereDialog *>( mDialog.get() );
      Qt3DExtras::QSphereMesh *mesh = dynamic_cast<Qt3DExtras::QSphereMesh *>( mCurrentMesh );
      mesh->setRadius( mDialog->getParam( 0 ) );
    }
    else if ( mType == "cylinder" )
    {
      Qt3DExtras::QCylinderMesh *mesh = dynamic_cast<Qt3DExtras::QCylinderMesh *>( mCurrentMesh );
      mesh->setRadius( mDialog->getParam( 0 ) );
      mesh->setLength( mDialog->getParam( 1 ) );
    }
    else if ( mType == "torus" )
    {
      Qt3DExtras::QTorusMesh *mesh = dynamic_cast<Qt3DExtras::QTorusMesh *>( mCurrentMesh );
      mesh->setRadius( mDialog->getParam( 0 ) );
      mesh->setMinorRadius( mDialog->getParam( 1 ) );
    }
    else if ( mType == "cone" )
    {
      Qt3DExtras::QConeMesh *mesh = dynamic_cast<Qt3DExtras::QConeMesh *>( mCurrentMesh );
      mesh->setBottomRadius( mDialog->getParam( 0 ) );
      mesh->setLength( mDialog->getParam( 1 ) );
      mesh->setTopRadius( mDialog->getParam( 2 ) );
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
    mRubberBand->moveLastPoint( rbPoint );

    QgsPoint prevPointMap = mPointOnMap[mNbMouseClick - 1];
    double length = prevPointMap.distance3D( pointMap );

    if ( mNbMouseClick == 1 )
    {
      double angle = -1.0 * QgsGeometryUtilsBase::lineAngle( pointMap.x(), pointMap.y(), prevPointMap.x(), prevPointMap.y() );
      angle *= 180.0 / M_PI;
      qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "prim size:" << length << "/ rotation: " << angle;

      mDialog->setRotation( mDialog->rotX(), mDialog->rotY(), ( angle < 0.0 ? 360.0 + angle : angle ) );
    }

    mDialog->setParam( mNbMouseClick - 1, length );

    updatePrimitive( /*length, 0.0*/ );
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
