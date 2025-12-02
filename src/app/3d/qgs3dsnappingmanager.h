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
      Off,
      Vertex,
      MiddleEdge,
      CenterFace,
    };

    Qgs3DSnappingManager( Qgs3DMapCanvasWidget *parentWidget );
    virtual ~Qgs3DSnappingManager();

    void restart();
    void finish();

    void setSnappingMode( SnappingMode mode );

    QVector3D screenToWorld( const QPoint &screenPos, bool *ok = nullptr, QString *layerId = nullptr, QgsFeatureId *nearestFid = nullptr, QVector3D ( *facePoints )[3] = nullptr ) const;

    QgsPoint screenToMap( const QPoint &screenPos, bool *ok = nullptr );

    void updateHighlighted( QgsMapLayer *layer, const QgsFeature &feat, const QVector3D &highlightedPoint );

    void clearHighlightedPointEntity();
    bool setEnableOnNode( Qt3DCore::QNode *currEnt, const QString &name, bool enabled );

  private:
    SnappingMode mMode;
    Qgs3DMapCanvasWidget *mParentWidget;
    Qgs3DMapCanvas *mCanvas;

    std::unique_ptr<Qt3DCore::QEntity> mHighlightedPointEntity = nullptr;
    QgsFeatureId mHighlightedFeatureId = -1;
    QRecursiveMutex mHighlightedMutex;
};

#endif // QGS3DSNAPPINGMANAGER_H
