#ifndef QGS3DSNAPPINGMANAGER_H
#define QGS3DSNAPPINGMANAGER_H

#include "qgspoint.h"
#include "qgsfeatureid.h"

#include <QRecursiveMutex>
#include <Qt3DCore/QEntity>

class Qgs3DMapCanvasWidget;
class Qgs3DMapCanvas;
class QgsMapLayer;
class QgsFeature;

namespace Qt3DCore
{
  class QNode;
} //namespace Qt3DCore

class Qgs3DSnappingManager
{
  public:
    enum SnappingMode
    {
      Off = 1 << 0,
      Vertex = 1 << 1,
      MiddleEdge = 1 << 2,
      CenterFace = 1 << 3,
    };

    Qgs3DSnappingManager( Qgs3DMapCanvasWidget *parentWidget, float tolerance );
    virtual ~Qgs3DSnappingManager();

    void restart();
    void finish();

    void setSnappingMode( SnappingMode mode );

    SnappingMode snappingMode() { return mMode; }

    void setTolerance( double tolerance );

    double tolerance() const { return mTolerance; }

    QVector3D screenToWorld( const QPoint &screenPos, bool *success = nullptr, SnappingMode *snapFound = nullptr, QString *layerId = nullptr, QgsFeatureId *nearestFid = nullptr, QVector3D ( *facePoints )[3] = nullptr ) const;

    QgsPoint screenToMap( const QPoint &screenPos, bool *ok = nullptr );

    bool setEnableOnNode( Qt3DCore::QNode *currEnt, const QString &name, bool enabled );

  private:
    void clearAllHighlightedEntities();
    void clearHighlightedEntityByName( const QString &name = QString() );
    void updateHighlighted( QgsMapLayer *layer, const QgsFeature &feature, const QVector3D &highlightedPoint, SnappingMode snapFound );

  private:
    SnappingMode mMode = SnappingMode::Off;
    Qgs3DMapCanvasWidget *mParentWidget;
    Qgs3DMapCanvas *mCanvas;
    double mTolerance = 0.0;

    std::unique_ptr<Qt3DCore::QEntity> mHighlightedPointEntity = nullptr;
    QgsFeatureId mHighlightedFeatureId = -1;
    QVector3D mPreviousHighlightedPoint;
    QRecursiveMutex mHighlightedMutex;
};

#endif // QGS3DSNAPPINGMANAGER_H
