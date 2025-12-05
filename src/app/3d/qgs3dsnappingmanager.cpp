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

#include "qgspoint3dsymbol.h"
#include "qgsmarkersymbol.h"
#include "qgsmarkersymbollayer.h"

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
  mHighlightedPointEntity->setObjectName( QStringLiteral( "ROOT_HL_OBJECT" ) );
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

QgsPoint Qgs3DSnappingManager::screenToMap( const QPoint &screenPos, bool *ok, bool highlightEntity, bool highlightSnappedPoint )
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

  if ( highlightSnappedPoint || highlightEntity )
  {
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
          QgsFeature feature;
          if ( ite.nextFeature( feature ) )
          {
            const QVector3D highlightPoint = snapFound != SnappingMode::Off ? worldPoint : QVector3D();
            updateHighlightedEntities( layer, feature, highlightPoint, snapFound, highlightEntity, highlightSnappedPoint );
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
  }
  else
  {
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
  QgsRayCastHit hitLayer;
  QgsRayCastHit hitTerrain;
  bool hitLayerFound = false;
  for ( const QgsRayCastHit &hit : allHits )
  {
    const double resDist = hit.distance();
    const QString layer = hit.properties().value( QStringLiteral( "layerId" ), QString() ).toString();
    if ( layer == QStringLiteral( "terrain" ) )
    {
      hitTerrain = hit;
    }
    else if ( minDist < 0 || resDist < minDist )
    {
      minDist = resDist;
      hitLayer = hit;
      hitLayerFound = true;
    }
  }

  const QgsRayCastHit bestHit = hitLayerFound ? hitLayer : hitTerrain;

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
  if ( mMode & SnappingMode::AlongEdge )
  {
    const QVector3D origin = ( *facePoints )[0];
    const QVector3D dest = ( *facePoints )[1];
    const QVector3D direction = ( dest - origin ).normalized();
    const QVector3D snapPoint = origin + QVector3D::dotProduct( worldPoint - origin, direction ) * direction;

    if ( QVector3D::dotProduct( dest - origin, snapPoint - origin ) >= 0 && QVector3D::dotProduct( origin - dest, snapPoint - dest ) >= 0 )
    {
      const float snapDistance = ( snapPoint - worldPoint ).length();
      if ( snapDistance < minSnapDistance )
      {
        if ( snapFound )
        {
          *snapFound = SnappingMode::AlongEdge;
        }
        minSnapDistance = snapDistance;
        outPoint = snapPoint;
      }
    }
    // else outside segment
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

void Qgs3DSnappingManager::updateHighlightedEntities( QgsMapLayer *layer, const QgsFeature &feature, const QVector3D &highlightedPoint, SnappingMode snapFound, bool highlightEntity, bool highlightSnappedPoint )
{
  QMutexLocker locker( &mHighlightedMutex );
  if ( !mHighlightedPointEntity )
    return;

  if ( highlightEntity && mHighlightedFeatureId != feature.id() )
  {
    qDebug() << QStringLiteral( "%1 #%2:" ).arg( __FUNCTION__ ).arg( __LINE__ ).toStdString() << "Switching from:" << mHighlightedFeatureId << "to:" << feature.id();

    clearAllHighlightedEntities();

    mHighlightedFeatureId = feature.id();

    QgsVectorLayer *vLayer = dynamic_cast<QgsVectorLayer *>( layer );
    if ( vLayer )
    {
      mCanvas->scene()->highlightEntity( mHighlightedPointEntity.get(), vLayer, feature );
    }
  } // end if feature id changed

  if ( highlightSnappedPoint && mMode != SnappingMode::Off && mPreviousHighlightedPoint != highlightedPoint )
  {
    mPreviousHighlightedPoint = highlightedPoint;
    // Remove the snap billboard entity if necessary
    clearHighlightedEntityByName( QStringLiteral( "HL_point" ) );
    if ( !highlightedPoint.isNull() )
    {
      // HL nearest vertex

      QgsFeature hlPointFeat;
      QgsPoint hlPointGeom( 0.0, 0.0, 0.0 );
      hlPointFeat.setGeometry( QgsGeometry::fromPoint( hlPointGeom ) );

      QgsMarkerSymbol *markerSymbol = static_cast<QgsMarkerSymbol *>( QgsSymbol::defaultSymbol( Qgis::GeometryType::Point ) );
      markerSymbol->setColor( QColor( 255, 0, 0, 150 ) );
      markerSymbol->setSize( 2 );
      QgsSimpleMarkerSymbolLayer *sl = static_cast<QgsSimpleMarkerSymbolLayer *>( markerSymbol->symbolLayer( 0 ) );
      switch ( snapFound )
      {
        case SnappingMode::Vertex:
          sl->setShape( Qgis::MarkerShape::Square );
          break;
        case SnappingMode::MiddleEdge:
          sl->setShape( Qgis::MarkerShape::Triangle );
          break;
        case SnappingMode::AlongEdge:
          sl->setShape( Qgis::MarkerShape::Cross );
          break;
        case SnappingMode::CenterFace:
          sl->setShape( Qgis::MarkerShape::Circle );
          break;
        default:
          sl->setShape( Qgis::MarkerShape::Heart );
          break;
      }

      sl->setStrokeColor( QColor( 0, 255, 255 ) );
      sl->setStrokeWidth( 0.5 );
      QgsPoint3DSymbol *point3DSymbol = new QgsPoint3DSymbol();
      point3DSymbol->setBillboardSymbol( markerSymbol );
      point3DSymbol->setShape( Qgis::Point3DShape::Billboard );

      Qgs3DRenderContext renderContext = Qgs3DRenderContext::fromMapSettings( mCanvas->mapSettings() );
      QSet<QString> attributeNames;

      // TODO do as rubberband

      QgsFeature3DHandler *feat3DHandler = QgsApplication::symbol3DRegistry()->createHandlerForSymbol( dynamic_cast<QgsVectorLayer *>( layer ), point3DSymbol );
      feat3DHandler->prepare( renderContext, attributeNames, QgsVector3D() );
      feat3DHandler->processFeature( hlPointFeat, renderContext );
      QList<Qt3DCore::QEntity *> billboardEntities = feat3DHandler->finalize( mHighlightedPointEntity.get(), renderContext );

      // Set billboard entity transform
      for ( auto billboardEntity : billboardEntities )
      {
        billboardEntity->setObjectName( QStringLiteral( "HL_point" ) );
        for ( auto transform : billboardEntity->componentsOfType<QgsGeoTransform>() )
        {
          // QgsVector3D mapPoint = Qgs3DUtils::worldToMapCoordinates( highlightedPoint, mCanvas->mapSettings()->origin() );
          transform->setGeoTranslation( QgsVector3D( highlightedPoint.x(), highlightedPoint.y(), highlightedPoint.z() ) );
          transform->setOrigin( QgsVector3D() );
        }
      }
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
