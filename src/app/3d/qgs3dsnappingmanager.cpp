#include "qgs3dsnappingmanager.h"

#include "qgs3dmapcanvas.h"
#include "qgs3dmapcanvaswidget.h"
#include "qgs3dmapscene.h"
#include "qgs3dsymbolregistry.h"
#include "qgs3dutils.h"
#include "qgscameracontroller.h"
#include "qgsfeature3dhandler_p.h"
#include "qgsfeatureiterator.h"
#include "qgsfeaturesource.h"
#include "qgsframegraph.h"
#include "qgsmaplayer.h"
#include "qgsraycastcontext.h"
#include "qgsraycasthit.h"
#include "qgsrubberband3d.h"
#include "qgsvectorlayer.h"
#include "qgsvectorlayer3drenderer.h"
#include "qgswindow3dengine.h"

#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DRender/QCamera>

Qgs3DSnappingManager::Qgs3DSnappingManager( Qgs3DMapCanvasWidget *parentWidget, float tolerance )
  : mType( SnappingType3D::Off )
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
  mHighlightedRootEntity.reset( new Qt3DCore::QEntity( mCanvas->engine()->frameGraph()->rubberBandsRootEntity() ) );
  mHighlightedRootEntity->setObjectName( QStringLiteral( "ROOT_HL_OBJECT" ) );
}

void Qgs3DSnappingManager::finish()
{
  QMutexLocker locker( &mHighlightedMutex );

  clearAllHighlightedEntities();

  if ( mHighlightedRootEntity.get() != nullptr )
  {
    mHighlightedRootEntity->setParent( ( Qt3DCore::QNode * ) nullptr );
    mHighlightedRootEntity->deleteLater();
  }
  mHighlightedRootEntity.reset();
}

void Qgs3DSnappingManager::setSnappingType( SnappingType3D mode )
{
  mType = mode;
}

void Qgs3DSnappingManager::setTolerance( double tolerance )
{
  mTolerance = tolerance;
}

QgsPoint Qgs3DSnappingManager::screenToMap( const QPoint &screenPos, bool *ok, bool highlightEntity, bool highlightSnappedPoint )
{
  QString layerId;
  QgsFeatureId nearestFid;
  SnappingType3D snapFound;

  QVector3D worldPoint = screenToWorld( screenPos, ok, &snapFound, &layerId, &nearestFid );
  // qDebug() << "Screen to world: success" << okWorld << "/ snapFound:" << snapFound << "/ layerId" << layerId << "/ nearestFid" << nearestFid;
  if ( !ok )
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
            const QVector3D highlightedPointInWorld = snapFound != SnappingType3D::Off ? worldPoint : QVector3D();
            updateHighlightedEntities( layer, feature, highlightedPointInWorld, snapFound, highlightEntity, highlightSnappedPoint );
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

QVector3D Qgs3DSnappingManager::screenToWorld( const QPoint &screenPos, bool *success, SnappingType3D *snapFound, QString *layerId, QgsFeatureId *nearestFid ) const
{
  *success = false;
  *snapFound = SnappingType3D::Off;
  *layerId = QString();
  *nearestFid = QgsFeatureId();

  QgsRayCastContext context;
  context.setSingleResult( false );
  context.setMaximumDistance( mCanvas->cameraController()->camera()->farPlane() );
  context.setAngleThreshold( 0.5f );
  const QgsRayCastResult results = mCanvas->castRay( screenPos, context );

  if ( results.isEmpty() )
  {
    return QVector3D();
  }

  QVector3D facePoints[3];
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

  *layerId = bestHit.properties().value( QStringLiteral( "layerId" ), QString() ).toString();
  *nearestFid = bestHit.properties().value( QStringLiteral( "fid" ), -1 ).toLongLong();
  if ( bestHit.properties().contains( QStringLiteral( "facePoint0" ) ) )
  {
    facePoints[0] = qvariant_cast<QVector3D>( bestHit.properties().value( QStringLiteral( "facePoint0" ) ) );
    facePoints[1] = qvariant_cast<QVector3D>( bestHit.properties().value( QStringLiteral( "facePoint1" ) ) );
    facePoints[2] = qvariant_cast<QVector3D>( bestHit.properties().value( QStringLiteral( "facePoint2" ) ) );
    //qDebug() << "Hit face: " << facePoints[0] << "/" << facePoints[1] << "/" << facePoints[2];
  }

  mapCoords = bestHit.mapCoordinates();
  if ( std::isnan( mapCoords.z() ) )
  {
    mapCoords.setZ( 0.0 );
  }

  QVector3D worldPoint = Qgs3DUtils::mapToWorldCoordinates( mapCoords, mCanvas->mapSettings()->origin() ).toVector3D();
  QVector3D outPoint = worldPoint;
  float minSnapDistance = static_cast<float>( mTolerance );

  if ( mType & SnappingType3D::Vertex )
  {
    const QVector3D snapPoint = facePoints[0];
    const float snapDistance = ( snapPoint - worldPoint ).length();
    if ( snapDistance < minSnapDistance )
    {
      *snapFound = SnappingType3D::Vertex;
      minSnapDistance = snapDistance;
      outPoint = snapPoint;
    }
  }
  if ( mType & SnappingType3D::AlongEdge )
  {
    const QVector3D origin = facePoints[0];
    const QVector3D dest = facePoints[1];
    const QVector3D direction = ( dest - origin ).normalized();
    const QVector3D snapPoint = origin + QVector3D::dotProduct( worldPoint - origin, direction ) * direction;

    if ( QVector3D::dotProduct( dest - origin, snapPoint - origin ) >= 0 && QVector3D::dotProduct( origin - dest, snapPoint - dest ) >= 0 )
    {
      const float snapDistance = ( snapPoint - worldPoint ).length();
      if ( snapDistance < minSnapDistance )
      {
        *snapFound = SnappingType3D::AlongEdge;
        minSnapDistance = snapDistance;
        outPoint = snapPoint;
      }
    }
    // else outside segment
  }
  if ( mType & SnappingType3D::MiddleEdge )
  {
    const QVector3D snapPoint = ( facePoints[0] + facePoints[1] ) / 2.0;
    const float snapDistance = ( snapPoint - worldPoint ).length();
    if ( snapDistance < minSnapDistance )
    {
      *snapFound = SnappingType3D::MiddleEdge;
      minSnapDistance = snapDistance;
      outPoint = snapPoint;
    }
  }
  if ( static_cast<int>( mType & Qgs3DSnappingManager::SnappingType3D::CenterFace ) != 0 )
  {
    const QVector3D snapPoint = ( facePoints[0] + facePoints[1] + facePoints[2] ) / 3.0;
    const float snapDistance = ( snapPoint - worldPoint ).length();
    if ( snapDistance < minSnapDistance )
    {
      *snapFound = SnappingType3D::CenterFace;
      minSnapDistance = snapDistance;
      outPoint = snapPoint;
    }
  }

  *success = true;

  return outPoint;
}

void Qgs3DSnappingManager::updateHighlightedEntities( QgsMapLayer *layer, const QgsFeature &feature, const QVector3D &highlightedPointInWorld, SnappingType3D snapFound, bool highlightEntity, bool highlightSnappedPoint )
{
  QMutexLocker locker( &mHighlightedMutex );
  if ( !mHighlightedRootEntity )
    return;

  if ( mHighlightedFeatureId != feature.id() )
  {
    clearAllHighlightedEntities();

    if ( highlightEntity )
    {
      mHighlightedFeatureId = feature.id();

      if ( QgsVectorLayer *vLayer = dynamic_cast<QgsVectorLayer *>( layer ) )
      {
        mCanvas->scene()->highlightEntity( mHighlightedRootEntity.get(), vLayer, feature );
      }
    }
  } // end if feature id changed

  if ( highlightSnappedPoint && mType != SnappingType3D::Off && mPreviousHighlightedPoint != highlightedPointInWorld )
  {
    updateHighlightedPoint( highlightedPointInWorld, snapFound );
  }
}


void Qgs3DSnappingManager::updateHighlightedPoint( const QVector3D &highlightedPointInWorld, SnappingType3D snapFound )
{
  if ( mType != SnappingType3D::Off && mPreviousHighlightedPoint != highlightedPointInWorld )
  {
    mPreviousHighlightedPoint = highlightedPointInWorld;
    // Remove the snap billboard entity if necessary
    if ( highlightedPointInWorld.isNull() )
      mHighlightedPointBB.reset();
    else
    {
      // HL nearest vertex

      mHighlightedPointBB.reset( new QgsRubberBand3D( *mCanvas->mapSettings(), mCanvas->engine(), mHighlightedRootEntity.get(), Qgis::GeometryType::Point ) );
      QgsVector3D mapPoint = Qgs3DUtils::worldToMapCoordinates( highlightedPointInWorld, mCanvas->mapSettings()->origin() );
      mHighlightedPointBB->addPoint( QgsPoint( mapPoint.x(), mapPoint.y(), mapPoint.z() ) );
      mHighlightedPointBB->setColor( QColor( 255, 0, 0, 150 ) );
      mHighlightedPointBB->setOutlineColor( QColor( 0, 255, 255 ) );
      mHighlightedPointBB->setWidth( 9.f );
      switch ( snapFound )
      {
        case SnappingType3D::Vertex:
          mHighlightedPointBB->setMarkerShape( Qgis::MarkerShape::Square );
          break;
        case SnappingType3D::MiddleEdge:
          mHighlightedPointBB->setMarkerShape( Qgis::MarkerShape::Triangle );
          break;
        case SnappingType3D::AlongEdge:
          mHighlightedPointBB->setMarkerShape( Qgis::MarkerShape::Cross2 );
          break;
        case SnappingType3D::CenterFace:
          mHighlightedPointBB->setMarkerShape( Qgis::MarkerShape::Circle );
          break;
        default:
          mHighlightedPointBB->setMarkerShape( Qgis::MarkerShape::Circle );
          break;
      }
    }
  }
}

void Qgs3DSnappingManager::clearAllHighlightedEntities()
{
  QMutexLocker locker( &mHighlightedMutex );
  if ( mHighlightedRootEntity )
  {
    mHighlightedPointBB.reset();

    if ( mHighlightedRootEntity )
    {
      if ( mCanvas && mCanvas->engine() && mCanvas->engine()->frameGraph() && mCanvas->scene()->engine() )
      {
        for ( auto child : mHighlightedRootEntity->childNodes() )
        {
          if ( Qt3DCore::QEntity *entity = dynamic_cast<Qt3DCore::QEntity *>( child ) )
          {
            entity->setParent( ( Qt3DCore::QNode * ) nullptr );
            entity->deleteLater();
          }
        }
      }
    }

    mHighlightedFeatureId = -1;
    qDebug() << "clearAllHighlightedEntities done!";
  }
}
