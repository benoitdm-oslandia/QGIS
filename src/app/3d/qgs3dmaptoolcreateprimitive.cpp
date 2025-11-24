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
#include "qgsforwardrenderview.h"
#include "qgs3dcreateprimitivecubedialog.h"
#include "qgs3dcreateprimitivespheredialog.h"

#include <QMouseEvent>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DRender/QScreenRayCaster>
#include <Qt3DRender/QRenderSettings>

Qgs3DMapToolCreatePrimitive::Qgs3DMapToolCreatePrimitive( Qgs3DMapCanvas *canvas, const QString &type )
  : Qgs3DMapTool( canvas ), mType( type )
{
  // Dialog
  if ( mType == "cube" )
    mDialog.reset( new Qgs3DCreatePrimitiveCubeDialog() );
  else if ( mType == "sphere" )
    mDialog.reset( new Qgs3DCreatePrimitiveSphereDialog() );

  connect( mDialog.get(), &Qgs3DCreatePrimitiveDialog::valueChanged, this, [this]() { updatePrimitive( mFirstPointOnMap, mDialog->defaultSize(), mDialog->rotZ() ); } );
  //mDialog->restorePosition();
}

Qgs3DMapToolCreatePrimitive::~Qgs3DMapToolCreatePrimitive() = default;

void Qgs3DMapToolCreatePrimitive::activate()
{
  qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString();
  restart();
  // updateSettings();

  // Show dialog
  //mDialog->updateSettings();
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
  mHighlightedPointEntity.reset();

  if ( mHighlightedEntity != nullptr )
  {
    mHighlightedEntity->removeComponent( mHighlightedMaterial.get() );
    if ( mPreviousHighlightedMaterial != nullptr )
      mHighlightedEntity->addComponent( mPreviousHighlightedMaterial );
    mHighlightedMaterial.reset();
  }

  if ( mScreenRayCaster != nullptr )
  {
    mScreenRayCaster->setEnabled( false );
    //disconnect( mScreenRayCaster.get(), &Qt3DRender::QScreenRayCaster::hitsChanged, this, &Qgs3DMapToolCreatePrimitive::onTouchedByRay );
    mCanvas->engine()->renderSettings()->pickingSettings()->setPickMethod( mDefaultPickingMethod );
    mCanvas->engine()->root()->removeComponent( mScreenRayCaster.get() );
    mScreenRayCaster.reset();
  }

  mRubberBand.reset();
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
  mDialog->resetData();
  mMouseClickPos = QPoint();

  mRubberBand.reset( new QgsRubberBand3D( *mCanvas->mapSettings(), mCanvas->engine(), mCanvas->engine()->frameGraph()->rubberBandsRootEntity() ) );
  const QgsSettings settings;
  const int myRed = settings.value( QStringLiteral( "qgis/default_measure_color_red" ), 222 ).toInt();
  const int myGreen = settings.value( QStringLiteral( "qgis/default_measure_color_green" ), 155 ).toInt();
  const int myBlue = settings.value( QStringLiteral( "qgis/default_measure_color_blue" ), 67 ).toInt();
  mRubberBand->setWidth( 3 );
  mRubberBand->setColor( QColor( myRed, myGreen, myBlue ) );

  mDefaultPickingMethod = mCanvas->engine()->renderSettings()->pickingSettings()->pickMethod();

  // Create screencaster to be used by EventFilter:
  //   1- Perform ray casting tests by specifying "touch" coordinates in screen space
  //   2- connect screencaster results to onTouchedByRay
  //   3- screencaster will be triggered by EventFilter
  mScreenRayCaster.reset( new Qt3DRender::QScreenRayCaster( mCanvas->engine()->root() ) );
  mScreenRayCaster->addLayer( mCanvas->engine()->frameGraph()->forwardRenderView().renderLayer() );
  mScreenRayCaster->setFilterMode( Qt3DRender::QScreenRayCaster::AcceptAllMatchingLayers );
  mScreenRayCaster->setRunMode( Qt3DRender::QAbstractRayCaster::SingleShot );

  mCanvas->engine()->root()->addComponent( mScreenRayCaster.get() );
  mCanvas->engine()->renderSettings()->pickingSettings()->setPickMethod( Qt3DRender::QPickingSettings::TrianglePicking );
  mCanvas->engine()->renderSettings()->pickingSettings()->setPickResultMode( Qt3DRender::QPickingSettings::NearestPick );
  mCanvas->engine()->renderSettings()->pickingSettings()->setWorldSpaceTolerance( 0.05 );

  mHighlightedMaterial.reset( new Qt3DExtras::QPhongMaterial );
  mHighlightedMaterial->setAmbient( Qt::blue );

  //connect( mScreenRayCaster.get(), &Qt3DRender::QScreenRayCaster::hitsChanged, this, &Qgs3DMapToolCreatePrimitive::onTouchedByRay );
}

void Qgs3DMapToolCreatePrimitive::onTouchedByRay( const Qt3DRender::QAbstractRayCaster::Hits &hits )
{
  mScreenRayCaster->setEnabled( false );
  // int hitFoundIdx = -1;
  if ( !hits.empty() )
  {
    if ( mHighlightedEntity != hits.at( 0 ).entity() )
    {
      qDebug() << "================================================";
      std::ostringstream os;
      os << "Qgs3DAxis::onTouchedByRay " << hits.length() << " hits at pos " << mMouseHoverPos << "\n";
      for ( int i = 0; i < hits.length(); ++i )
      {
        Qt3DCore::QEntity *hitEntity = hits.at( i ).entity();
        if ( hits.at( i ).distance() > 0 && hitEntity->isEnabled() )
        {
          os << "\tHit entity name: " << hitEntity->objectName().toStdString() << "\n";

          while ( hitEntity != nullptr && hitEntity->objectName().isEmpty() )
            hitEntity = hitEntity->parentEntity();
          if ( hitEntity != nullptr && hitEntity != hits.at( i ).entity() )
            os << "\t Hit parent entity name: " << hitEntity->objectName().toStdString() << "\n";
          else
            os << "\t Hit parent entity name: " << "<None>" << "\n";

          // os << "\t Hit Type: " << hits.at( i ).type() << "\n";
          // os << "\t Hit triangle id: " << hits.at( i ).primitiveIndex() << "\n";
          os << "\t Hit distance: " << hits.at( i ).distance() << "\n";
        }
      }
      qDebug() << os.str().c_str();


      // remove old HL
      if ( mPreviousHighlightedMaterial != nullptr )
      {
        qDebug() << "Removing HL mat from previous entity: " << mHighlightedEntityChild->objectName();
        mHighlightedEntityChild->removeComponent( mHighlightedMaterial.get() );
        mHighlightedEntityChild->addComponent( mPreviousHighlightedMaterial );
      }

      mHighlightedEntity = hits.at( 0 ).entity();
      Qt3DCore::QEntity *hitEntity = mHighlightedEntity;
      if ( hitEntity->objectName().isEmpty() )
      {
        while ( hitEntity != nullptr && hitEntity->objectName().isEmpty() )
          hitEntity = hitEntity->parentEntity();
        qDebug() << "Parent entity is: " << hitEntity->objectName();
      }
      else
      {
        qDebug() << "Main entity is: " << hitEntity->objectName();
      }

      // ===================== search all MAT
      for ( auto comp : hitEntity->components() )
      {
        if ( Qt3DRender::QMaterial *mat = dynamic_cast<Qt3DRender::QMaterial *>( comp ) )
        {
          qDebug() << "searching for mat from top '" << hitEntity->objectName() << "': " << mat;
        }
        // search for mat in entity children
        for ( auto child : hitEntity->findChildren<Qt3DCore::QEntity *>() )
        {
          for ( auto comp : child->components() )
          {
            if ( Qt3DRender::QMaterial *mat = dynamic_cast<Qt3DRender::QMaterial *>( comp ) )
            {
              qDebug() << "In sub entity '" << child->objectName() << "', found mat: " << mat;
            }
          }
        }
      }

      // search for mat in entity
      for ( auto comp : hitEntity->components() )
      {
        if ( Qt3DRender::QMaterial *mat = dynamic_cast<Qt3DRender::QMaterial *>( comp ) )
        {
          mPreviousHighlightedMaterial = mat;
          mHighlightedEntityChild = hitEntity;
          qDebug() << "In main entity, found mat: " << mat;
          break;
        }
      }
      if ( mPreviousHighlightedMaterial == nullptr )
      {
        // search for mat in entity children
        for ( auto child : hitEntity->findChildren<Qt3DCore::QEntity *>() )
        {
          for ( auto comp : child->components() )
          {
            if ( Qt3DRender::QMaterial *mat = dynamic_cast<Qt3DRender::QMaterial *>( comp ) )
            {
              mPreviousHighlightedMaterial = mat;
              mHighlightedEntityChild = child;
              qDebug() << "In sub entity, found mat: " << mat;
              break;
            }
          }
          if ( mPreviousHighlightedMaterial != nullptr )
            break;
        }
      }

      if ( mPreviousHighlightedMaterial == nullptr )
      {
        qDebug() << "Found NO mat in entity: " << mHighlightedEntity->objectName();
      }
      else
      {
        mHighlightedEntityChild->removeComponent( mPreviousHighlightedMaterial );

        mHighlightedMaterial.reset( new Qt3DExtras::QPhongMaterial );
        mHighlightedMaterial->setAmbient( Qt::blue );
        mHighlightedEntityChild->addComponent( mHighlightedMaterial.get() );
      }
    }

    // for ( int i = 0; i < hits.length() && hitFoundIdx == -1; ++i )
    // {
    //   Qt3DCore::QEntity *hitEntity = hits.at( i ).entity();
    //   // In Qt6, a Qt3DExtras::Text2DEntity contains a private entity: Qt3DExtras::DistanceFieldTextRenderer
    //   // The Text2DEntity needs to be retrieved to handle proper picking
    //   if ( hitEntity && qobject_cast<Qt3DExtras::QText2DEntity *>( hitEntity->parentEntity() ) )
    //   {
    //     hitEntity = hitEntity->parentEntity();
    //   }
    //   if ( hits.at( i ).distance() < 500.0f && hitEntity && ( hitEntity == mCubeRoot || hitEntity == mAxisRoot || hitEntity->parent() == mCubeRoot || hitEntity->parent() == mAxisRoot ) )
    //   {
    //     hitFoundIdx = i;
    //   }
    // }
  }
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

void Qgs3DMapToolCreatePrimitive::updatePrimitive( const QgsPoint &mapPos, double length, double zRotation )
{
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
    }
    else if ( mType == "sphere" )
    {
      Qt3DExtras::QSphereMesh *mesh = new Qt3DExtras::QSphereMesh();
      mesh->setRadius( 0.5 );
      mesh->setRings( 6 );
      mesh->setSlices( 6 );
      mPrimitiveLineEntity->addComponent( mesh );
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
  }

  transform->setOrigin( mCanvas->mapSettings()->origin() );
  transform->setRotationX( static_cast<float>( mDialog->rotX() ) );
  transform->setRotationY( static_cast<float>( mDialog->rotY() ) );
  transform->setRotationZ( static_cast<float>( zRotation ) );
  transform->setGeoTranslation( QgsVector3D( mapPos.x(), mapPos.y(), mapPos.z() ) );
  transform->setScale3D( { static_cast<float>( mDialog->scaleX() * length ), static_cast<float>( mDialog->scaleY() * length ), static_cast<float>( mDialog->scaleZ() * length ) } );
}

void Qgs3DMapToolCreatePrimitive::updateHLPoint( const QgsPoint &mapPos, const QPoint &screenPos )
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

  // QgsPoint mapPos2 = screenToMap( QPoint( screenPos.x() + 10, screenPos.y() + 10 ) );

  // QgsVector3D worldPos = Qgs3DUtils::mapToWorldCoordinates( QgsVector3D( mapPos.x(), mapPos.y(), mapPos.z() ), mCanvas->mapSettings()->origin() );
  // QgsVector3D worldPos2 = Qgs3DUtils::mapToWorldCoordinates( QgsVector3D( mapPos2.x(), mapPos2.y(), mapPos2.z() ), mCanvas->mapSettings()->origin() );

  double length = 10; //mapPos.distance3D( mapPos2 );

  // qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "screen / map:" << screenPos << "/" << mapPos.toQPointF() << worldPos.toVector3D();
  // qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "screen2 / map2:" << QPoint( screenPos.x() + 10, screenPos.y() + 10 ) << "/" << mapPos2.toQPointF() << worldPos2.toVector3D();
  qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "HL size:" << length;
  transform->setScale3D( { static_cast<float>( length ), static_cast<float>( length ), static_cast<float>( length ) } );
}

void Qgs3DMapToolCreatePrimitive::handleClick( const QPoint &screenPos )
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

void Qgs3DMapToolCreatePrimitive::mousePressEvent( QMouseEvent * /*event*/ )
{
}

void Qgs3DMapToolCreatePrimitive::mouseMoveEvent( QMouseEvent *event )
{
  if ( !mMouseHasMoved && ( event->pos() - mMouseClickPos ).manhattanLength() >= QApplication::startDragDistance() )
  {
    mMouseHasMoved = true;
  }

  if ( mMouseHoverPos == event->pos() )
    return;

  mMouseHoverPos = event->pos();
  QgsPoint pointMap = screenToMap( mMouseHoverPos );

  QgsPoint rbPoint( pointMap );
  rbPoint.setZ( rbPoint.z() / mCanvas->mapSettings()->terrainSettings()->verticalScale() );

  //updateHLPoint( pointMap, mMouseHoverPos );

  if ( mScreenRayCaster.get() != nullptr )
  {
    mScreenRayCaster->setEnabled( true );
    onTouchedByRay( mScreenRayCaster->pick( mMouseHoverPos ) );
  }

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
    qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "prim size:" << length << "/ rotation: " << angle;

    updatePrimitive( mFirstPointOnMap, length, angle );

    mDialog->setRotation( mDialog->rotX(), mDialog->rotY(), ( angle < 0.0 ? 360.0 + angle : angle ) );
    if ( mType == "cube" )
    {
      dynamic_cast<Qgs3DCreatePrimitiveCubeDialog *>( mDialog.get() )->setSize( length );
    }
    else if ( mType == "sphere" )
    {
      dynamic_cast<Qgs3DCreatePrimitiveSphereDialog *>( mDialog.get() )->setRadius( length );
    }
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
    if ( mDone )
    {
      restart();
      return;
    }

    // Finish measurement
    finish();
  }
}
