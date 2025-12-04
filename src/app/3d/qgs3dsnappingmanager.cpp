#include "qgs3dsnappingmanager.h"

#include "qgsraycastcontext.h"
#include "qgsraycasthit.h"
#include "qgs3dmapcanvas.h"
#include "qgscameracontroller.h"
#include "qgsfeaturesource.h"
#include "qgsfeatureiterator.h"
#include "qgsmaplayer.h"
#include "qgs3dmapscene.h"
#include "qgswindow3dengine.h"
#include "qgsframegraph.h"
#include "qgspolygon3dsymbol.h"
#include "qgsraycastcontext.h"
#include "qgsraycasthit.h"
#include "qgsfeaturesource.h"
#include "qgsfeatureiterator.h"
#include "qgsvectorlayer3drenderer.h"
#include "qgspolygon3dsymbol.h"
#include "qgspoint3dsymbol.h"
#include "qgsline3dsymbol.h"
#include "qgs3dsymbolregistry.h"
#include "qgsfeature3dhandler_p.h"
#include "qgsapplication.h"
#include "qgsvectorlayer.h"
#include "qgsgeotransform.h"
#include "qgs3dutils.h"
#include "qgs3dmapcanvaswidget.h"

#include <Qt3DRender/QCamera>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/QCylinderMesh>

Qgs3DSnappingManager::Qgs3DSnappingManager( Qgs3DMapCanvasWidget *parentWidget, float tolerance )
  : mMode( SnappingMode::Off )
  , mParentWidget( parentWidget )
  , mTolerance( tolerance )
{
}

Qgs3DSnappingManager::~Qgs3DSnappingManager()
{
  finish();
}

void Qgs3DSnappingManager::restart()
{
  mCanvas = mParentWidget->mapCanvas3D();
  Q_ASSERT( mCanvas != nullptr );

  QMutexLocker locker( &mHighlightedMutex );
  mHighlightedPointEntity.reset( new Qt3DCore::QEntity( mCanvas->engine()->frameGraph()->rubberBandsRootEntity() ) );
  mHighlightedPointEntity->setObjectName( "ROOT_HL_OBJECT" );
}

void Qgs3DSnappingManager::finish()
{
  QMutexLocker locker( &mHighlightedMutex );

  clearAllHighlightedEntities();

  if ( mHighlightedPointEntity.get() != nullptr )
  {
    mHighlightedPointEntity->setParent( ( Qt3DCore::QNode * ) nullptr );
    mHighlightedPointEntity->deleteLater();
  }
  mHighlightedPointEntity.reset();
}

void Qgs3DSnappingManager::setSnappingMode( SnappingMode mode )
{
  mMode = mode;
}

void Qgs3DSnappingManager::setTolerance( double tolerance )
{
  mTolerance = tolerance;
}

QgsPoint Qgs3DSnappingManager::screenToMap( const QPoint &screenPos, bool *ok )
{
  QString layerId;
  QgsFeatureId nearestFid;
  QVector3D facePoints[3];

  bool okWorld;
  SnappingMode snapFound;
  QVector3D worldPoint = screenToWorld( screenPos, &okWorld, &snapFound, &layerId, &nearestFid, &facePoints );
  // qDebug() << "Screen to world: success" << okWorld << "/ snapFound:" << snapFound << "/ layerId" << layerId << "/ nearestFid" << nearestFid;
  if ( ok )
  {
    *ok = okWorld;
  }
  if ( !okWorld )
  {
    // Unable to compute position
    qDebug() << "Unable to compute position";
    clearAllHighlightedEntities();
    return QgsPoint();
  }

  if ( mHighlightedFeatureId != nearestFid && nearestFid < std::numeric_limits<int>::max() )
    qDebug() << "HL changed layerId:" << layerId << "/ nearestFid:" << nearestFid;
  if ( !layerId.isEmpty() && nearestFid > 0 && nearestFid < std::numeric_limits<int>::max() )
  {
    // Inside a feature
    // Highlight the feature and display a sphere if a snapPoint was found
    const QList<QgsMapLayer *> layers = mCanvas->scene()->layers();
    for ( QgsMapLayer *layer : layers )
    {
      QgsFeatureSource *featureLayer = dynamic_cast<QgsFeatureSource *>( layer );
      if ( featureLayer && layer->id() == layerId )
      {
        QgsFeatureRequest req( nearestFid );
        req.setCoordinateTransform( QgsCoordinateTransform( layer->crs3D(), mCanvas->mapSettings()->crs(), mCanvas->mapSettings()->transformContext() ) );
        QgsFeatureIterator ite = featureLayer->getFeatures( req );
        QgsFeature feat;
        if ( ite.nextFeature( feat ) )
        {
          const QVector3D highlightPoint = snapFound != SnappingMode::Off ? worldPoint : QVector3D();
          updateHighlighted( layer, feat, highlightPoint, snapFound );
        }
        break;
      }
    }
  }
  else if ( mHighlightedFeatureId != -1 )
  {
    qDebug() << "on est sortie d'un batiment on clean";
    // Not Inside a feature anymore
    // clear all the highlight
    clearAllHighlightedEntities();
  }

  QgsVector3D mapPoint = Qgs3DUtils::worldToMapCoordinates( QgsVector3D( worldPoint ), mCanvas->mapSettings()->origin() );
  return QgsPoint( mapPoint.x(), mapPoint.y(), mapPoint.z() );
}

QVector3D Qgs3DSnappingManager::screenToWorld( const QPoint &screenPos, bool *success, SnappingMode *snapFound, QString *layerId, QgsFeatureId *nearestFid, QVector3D ( *facePoints )[3] ) const
{
  if ( success )
  {
    *success = false;
  }

  if ( snapFound )
  {
    *snapFound = SnappingMode::Off;
  }

  QgsRayCastContext context;
  context.setSingleResult( false );
  context.setMaximumDistance( mCanvas->cameraController()->camera()->farPlane() );
  context.setAngleThreshold( 0.5f );
  const QgsRayCastResult results = mCanvas->castRay( screenPos, context );

  if ( results.isEmpty() )
  {
    return QVector3D();
  }

  if ( success )
  {
    *success = true;
  }

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
  {
    mapCoords.setZ( 0.0 );
  }

  QVector3D worldPoint = Qgs3DUtils::mapToWorldCoordinates( mapCoords, mCanvas->mapSettings()->origin() ).toVector3D();
  QVector3D outPoint = worldPoint;
  float minSnapDistance = static_cast<float>( mTolerance );

  if ( mMode & SnappingMode::Vertex )
  {
    const QVector3D snapPoint = ( *facePoints )[0];
    const float snapDistance = ( snapPoint - worldPoint ).length();
    if ( snapDistance < minSnapDistance )
    {
      if ( snapFound )
      {
        *snapFound = SnappingMode::Vertex;
      }
      minSnapDistance = snapDistance;
      outPoint = snapPoint;
    }
  }
  if ( mMode & SnappingMode::MiddleEdge )
  {
    const QVector3D snapPoint = ( ( *facePoints )[0] + ( *facePoints )[1] ) / 2.0;
    const float snapDistance = ( snapPoint - worldPoint ).length();
    if ( snapDistance < minSnapDistance )
    {
      if ( snapFound )
      {
        *snapFound = SnappingMode::MiddleEdge;
      }
      minSnapDistance = snapDistance;
      outPoint = snapPoint;
    }
  }
  if ( static_cast<int>( mMode & Qgs3DSnappingManager::SnappingMode::CenterFace ) != 0 )
  {
    const QVector3D snapPoint = ( ( *facePoints )[0] + ( *facePoints )[1] + ( *facePoints )[2] ) / 3.0;
    const float snapDistance = ( snapPoint - worldPoint ).length();
    if ( snapDistance < minSnapDistance )
    {
      if ( snapFound )
      {
        *snapFound = SnappingMode::CenterFace;
      }
      minSnapDistance = snapDistance;
      outPoint = snapPoint;
    }
  }

  return outPoint;
}

void Qgs3DSnappingManager::updateHighlighted( QgsMapLayer *layer, const QgsFeature &feat, const QVector3D &highlightedPoint, SnappingMode snapFound )
{
  QMutexLocker locker( &mHighlightedMutex );
  if ( !mHighlightedPointEntity )
    return;

  if ( mHighlightedFeatureId != feat.id() )
  {
    qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "Switching from:" << mHighlightedFeatureId << "to:" << feat.id();

    clearAllHighlightedEntities();

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
              trans->setGeoTranslation( mCanvas->mapSettings()->origin() * -1.0 );
              trans->setOrigin( QgsVector3D() );
            }
            break;
          }
        }

        //setEnableOnNode( mHighlightedPointEntity->parentNode()->parentNode(), "Drapé_2/3/1_SINGLE_10_TESSELLATED", false );
      }
    }
  } // end if feature id changed

  if ( mMode != SnappingMode::Off )
  {
    if ( highlightedPoint.isNull() )
    {
      // no snap found
      // Remove the snap shpere entity if necessary
      clearHighlightedEntityByName( QStringLiteral( "HL_point" ) );
    }
    else
    {
      // HL nearest vertex
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
      mapPoint = Qgs3DUtils::worldToMapCoordinates( QVector3D( highlightedPoint.x(), highlightedPoint.y(), highlightedPoint.z() ), QgsVector3D() /*mCanvas->mapSettings()->origin()*/ );
      transform->setGeoTranslation( mapPoint );
      // qDebug() << "facepoint:" << facePoints[0];
      // qDebug() << "mapPoint:" << mapPoint.toVector3D();
    }
  }
}


void Qgs3DSnappingManager::clearHighlightedEntityByName( const QString &name )
{
  QMutexLocker locker( &mHighlightedMutex );
  if ( mHighlightedPointEntity )
  {
    if ( mCanvas && mCanvas->engine() && mCanvas->engine()->frameGraph() && mCanvas->scene()->engine() )
    {
      for ( auto child : mHighlightedPointEntity->childNodes() )
      {
        if ( Qt3DCore::QEntity *entity = dynamic_cast<Qt3DCore::QEntity *>( child ) )
        {
          if ( name.isEmpty() || entity->objectName() == name )
          {
            entity->setParent( ( Qt3DCore::QNode * ) nullptr );
            entity->deleteLater();
          }
        }
      }
    }
    //setEnableOnNode( mHighlightedPointEntity->parentNode()->parentNode(), "Drapé_2/3/1_SINGLE_10_TESSELLATED", true );
  }
}

void Qgs3DSnappingManager::clearAllHighlightedEntities()
{
  QMutexLocker locker( &mHighlightedMutex );
  if ( mHighlightedPointEntity )
  {
    clearHighlightedEntityByName();
    mHighlightedFeatureId = -1;
    qDebug() << "clearAllHighlightedEntities done!";
  }
}

bool Qgs3DSnappingManager::setEnableOnNode( Qt3DCore::QNode *currEnt, const QString &name, bool enabled )
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
