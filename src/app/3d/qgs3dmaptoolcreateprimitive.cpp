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

#include "qgs3dmapscene.h"
#include "qgs3dmaptoolcreateprimitive.h"
#include "qgs3dcreateprimitivedialog.h"
#include "qgs3dutils.h"
#include "qgsabstract3drenderer.h"
#include "qgspolygon3dsymbol.h"
#include "qgsvectorlayer.h"
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
#include "qgsfeaturesource.h"
#include "qgsfeatureiterator.h"
#include "qgsvectorlayer3drenderer.h"
#include "qgspolygon3dsymbol.h"
#include "qgspoint3dsymbol.h"
#include "qgsline3dsymbol.h"
#include "qgs3dsymbolregistry.h"
#include "qgsfeature3dhandler_p.h"
#include "qgsapplication.h"

#include <QMutexLocker>
#include <QMouseEvent>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/QCylinderMesh>

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

  {
    QMutexLocker locker( &mHighlightedMutex );
    if ( mHighlightedPointEntity.get() != nullptr )
    {
      mHighlightedPointEntity->setParent( ( Qt3DCore::QNode * ) nullptr );
      mHighlightedPointEntity->deleteLater();
    }
    mHighlightedPointEntity.reset();
  }

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

  //mScreenRayCaster.reset( new Qt3DRender::QScreenRayCaster( mCanvas->engine()->root() ) );
  if ( mScreenRayCaster != nullptr )
  {
    mScreenRayCaster->addLayer( mCanvas->engine()->frameGraph()->forwardRenderView().renderLayer() );
    mScreenRayCaster->setFilterMode( Qt3DRender::QScreenRayCaster::AcceptAllMatchingLayers );
    mScreenRayCaster->setRunMode( Qt3DRender::QAbstractRayCaster::SingleShot );

    mCanvas->engine()->root()->addComponent( mScreenRayCaster.get() );
    mCanvas->engine()->renderSettings()->pickingSettings()->setPickMethod( Qt3DRender::QPickingSettings::TrianglePicking );
    mCanvas->engine()->renderSettings()->pickingSettings()->setPickResultMode( Qt3DRender::QPickingSettings::NearestPick );
    mCanvas->engine()->renderSettings()->pickingSettings()->setWorldSpaceTolerance( 0.05 );

    mHighlightedMaterial.reset( new Qt3DExtras::QPhongMaterial );
    mHighlightedMaterial->setAmbient( Qt::blue );
  }
  else
  {
    QMutexLocker locker( &mHighlightedMutex );
    mHighlightedPointEntity.reset( new Qt3DCore::QEntity( mCanvas->engine()->frameGraph()->rubberBandsRootEntity() ) );
    mHighlightedPointEntity->setObjectName( "ROOT_HL_OBJECT" );
  }

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
  }
}

QgsPoint Qgs3DMapToolCreatePrimitive::screenToMap( const QPoint &screenPos, QString *layerId, QgsFeatureId *nearestFid, QVector3D ( *facePoints )[3] ) const
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
  QgsRayCastHit bestHit;
  for ( const QgsRayCastHit &hit : allHits )
  {
    const double resDist = hit.distance();
    if ( minDist < 0 || resDist < minDist )
    {
      minDist = resDist;
      bestHit = hit;
    }
  }

  if ( layerId != nullptr )
    *layerId = bestHit.properties().value( QStringLiteral( "layerId" ), QString() ).toString();
  if ( nearestFid != nullptr )
    *nearestFid = bestHit.properties().value( QStringLiteral( "fid" ), -1 ).toLongLong();
  if ( facePoints != nullptr && bestHit.properties().contains( QStringLiteral( "facePoint0" ) ) )
  {
    ( *facePoints )[0] = qvariant_cast<QVector3D>( bestHit.properties().value( QStringLiteral( "facePoint0" ) ) );
    ( *facePoints )[1] = qvariant_cast<QVector3D>( bestHit.properties().value( QStringLiteral( "facePoint1" ) ) );
    ( *facePoints )[2] = qvariant_cast<QVector3D>( bestHit.properties().value( QStringLiteral( "facePoint2" ) ) );
    //qDebug() << "Hit face: " << ( *facePoints )[0] << "/" << ( *facePoints )[1] << "/" << ( *facePoints )[2];
  }

  mapCoords = bestHit.mapCoordinates();
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

void Qgs3DMapToolCreatePrimitive::updateHighlighted( const QgsPoint &mapPos, const QPoint &screenPos, QgsMapLayer *layer, const QgsFeature &feat, const QVector3D ( &facePoints )[3] )
{
  QMutexLocker locker( &mHighlightedMutex );
  if ( !mHighlightedPointEntity )
    return;

  if ( mHighlightedFeatureId != feat.id() )
  {
    qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "Switching from:" << mHighlightedFeatureId << "to:" << feat.id();

    clearHighlightedPointEntity();

    mHighlightedFeatureId = feat.id();

    QgsVectorLayer3DRenderer *vectorRenderer = dynamic_cast<QgsVectorLayer3DRenderer *>( layer->renderer3D() );
    if ( vectorRenderer )
    {
      QgsPhongMaterialSettings *phong = dynamic_cast<QgsPhongMaterialSettings *>( QgsPhongMaterialSettings::create() );
      phong->setAmbient( Qt::darkRed );
      phong->setDiffuse( Qt::darkGray );
      phong->setOpacity( 0.4f );
      QgsAbstract3DSymbol *sym = nullptr;
      if ( QgsPolygon3DSymbol *clonedSymb = dynamic_cast<QgsPolygon3DSymbol *>( vectorRenderer->symbol()->clone() ) )
      {
        clonedSymb->setMaterialSettings( phong );
        clonedSymb->setAddBackFaces( false );
        clonedSymb->setCullingMode( Qgs3DTypes::CullingMode::NoCulling );
        clonedSymb->setEdgesEnabled( true );
        clonedSymb->setEdgeColor( Qt::green );
        clonedSymb->setEdgeWidth( 1.5f );
        sym = clonedSymb;
      }
      else if ( QgsPoint3DSymbol *clonedSymb = dynamic_cast<QgsPoint3DSymbol *>( vectorRenderer->symbol()->clone() ) )
      {
        clonedSymb->setMaterialSettings( phong );
        sym = clonedSymb;
      }
      else if ( QgsLine3DSymbol *clonedSymb = dynamic_cast<QgsLine3DSymbol *>( vectorRenderer->symbol()->clone() ) )
      {
        clonedSymb->setMaterialSettings( phong );
        sym = clonedSymb;
      }
      else
      {
        delete phong;
      }

      if ( sym != nullptr )
      {
        Qgs3DRenderContext renderContext = Qgs3DRenderContext::fromMapSettings( mCanvas->mapSettings() );
        QSet<QString> attributeNames;

        QgsFeature3DHandler *feat3DHandler = QgsApplication::symbol3DRegistry()->createHandlerForSymbol( dynamic_cast<QgsVectorLayer *>( layer ), sym );
        feat3DHandler->prepare( renderContext, attributeNames, QgsVector3D() );
        feat3DHandler->processFeature( feat, renderContext );
        feat3DHandler->finalize( mHighlightedPointEntity.get(), renderContext );

        // retrieve created entity
        for ( auto child : mHighlightedPointEntity->childNodes() )
        {
          if ( Qt3DCore::QEntity *ent = dynamic_cast<Qt3DCore::QEntity *>( child ) )
          {
            for ( auto trans : ent->componentsOfType<QgsGeoTransform>() )
            {
              trans->setGeoTranslation( { -mCanvas->mapSettings()->origin().x(), -mCanvas->mapSettings()->origin().y(), -mCanvas->mapSettings()->origin().z() } );
              trans->setOrigin( QgsVector3D() );
            }
            break;
          }
        }

        setEnableOnNode( mHighlightedPointEntity->parentNode()->parentNode(), "Drapé_2/3/1_SINGLE_10_TESSELLATED", false );
      }
    }
  } // end if feature id changed

  { // HL nearest vertex
    QgsGeoTransform *transform = nullptr;
    // search for existing HL vertex
    for ( auto child : mHighlightedPointEntity->childNodes() )
    {
      if ( Qt3DCore::QEntity *childEnt = dynamic_cast<Qt3DCore::QEntity *>( child ) )
        if ( childEnt->objectName() == "HL_point" )
        {
          for ( auto childComp : childEnt->componentsOfType<QgsGeoTransform>() )
            transform = childComp;
          break;
        }
    }

    if ( transform == nullptr )
    {
      Qt3DCore::QEntity *ent = new Qt3DCore::QEntity( mHighlightedPointEntity.get() );
      ent->setObjectName( "HL_point" );

      Qt3DExtras::QSphereMesh *mesh = new Qt3DExtras::QSphereMesh;
      mesh->setRadius( 1.0 );
      mesh->setSlices( 4 );
      mesh->setRings( 4 );
      ent->addComponent( mesh );

      Qt3DExtras::QPhongMaterial *material = new Qt3DExtras::QPhongMaterial;
      material->setAmbient( Qt::green );
      ent->addComponent( material );

      transform = new QgsGeoTransform;
      ent->addComponent( transform );
    }

    QgsVector3D mapPoint;
    mapPoint = Qgs3DUtils::worldToMapCoordinates( QVector3D( facePoints[0].x(), facePoints[0].y(), facePoints[0].z() ), QgsVector3D() /*mCanvas->mapSettings()->origin()*/ );
    transform->setGeoTranslation( mapPoint );
    // qDebug() << "facepoint:" << facePoints[0];
    // qDebug() << "mapPoint:" << mapPoint.toVector3D();
  }
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

bool Qgs3DMapToolCreatePrimitive::setEnableOnNode( Qt3DCore::QNode *currEnt, const QString &name, bool enabled )
{
  for ( auto child : currEnt->childNodes() )
  {
    if ( Qt3DCore::QEntity *ent = dynamic_cast<Qt3DCore::QEntity *>( child ) )
      if ( ent->objectName() == name )
      {
        ent->setEnabled( enabled );
        ent->parentNode()->setEnabled( enabled );
        ent->parentNode()->parentNode()->setEnabled( enabled );
        qDebug() << "========= Found entity: " << ent->objectName() << "/enabled:" << ent->isEnabled();
        return true;
      }

    if ( setEnableOnNode( child, name, enabled ) )
      return true;
  }
  return false;
}

void Qgs3DMapToolCreatePrimitive::clearHighlightedPointEntity()
{
  QMutexLocker locker( &mHighlightedMutex );
  if ( mCanvas->engine() && mCanvas->engine()->frameGraph() && mCanvas->scene()->engine() && mHighlightedPointEntity )
  {
    qDebug() << "mHighlightedPointEntity cleared!";

    for ( auto child : mHighlightedPointEntity->childNodes() )
    {
      if ( Qt3DCore::QEntity *ent = dynamic_cast<Qt3DCore::QEntity *>( child ) )
      {
        ent->setParent( ( Qt3DCore::QNode * ) nullptr );
        ent->deleteLater();
      }
    }
  }
  mHighlightedFeatureId = -1;

  setEnableOnNode( mHighlightedPointEntity->parentNode()->parentNode(), "Drapé_2/3/1_SINGLE_10_TESSELLATED", true );
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
  QString layerId;
  QgsFeatureId nearestFid;
  QVector3D facePoints[3];
  QgsPoint pointMap = screenToMap( mMouseHoverPos, &layerId, &nearestFid, &facePoints );

  QgsPoint rbPoint( pointMap );
  rbPoint.setZ( rbPoint.z() / mCanvas->mapSettings()->terrainSettings()->verticalScale() );

  if ( mScreenRayCaster.get() != nullptr )
  {
    mScreenRayCaster->setEnabled( true );
    onTouchedByRay( mScreenRayCaster->pick( mMouseHoverPos ) );
  }
  else
  {
    if ( mHighlightedFeatureId != nearestFid && nearestFid < std::numeric_limits<int>::max() )
      qDebug() << "HL changed layerId:" << layerId << "/ nearestFid:" << nearestFid;
    if ( !layerId.isEmpty() && nearestFid > 0 && nearestFid < std::numeric_limits<int>::max() )
    {
      const QList<QgsMapLayer *> layers = mCanvas->scene()->layers();
      for ( QgsMapLayer *layer : layers )
      {
        QgsFeatureSource *featureLayer = dynamic_cast<QgsFeatureSource *>( layer );
        if ( featureLayer && layer->id() == layerId )
        {
          QgsFeatureRequest req( nearestFid );
          QgsFeatureIterator ite = featureLayer->getFeatures( req );
          QgsFeature feat;
          if ( ite.nextFeature( feat ) )
          {
            updateHighlighted( pointMap, mMouseHoverPos, layer, feat, facePoints );
          }
          break;
        }
      }
    }
    else if ( mHighlightedFeatureId != -1 )
    {
      clearHighlightedPointEntity();
    }
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
