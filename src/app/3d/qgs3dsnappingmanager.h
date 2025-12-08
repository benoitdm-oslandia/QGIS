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
class QgsRubberBand3D;

namespace Qt3DCore
{
  class QNode;
} //namespace Qt3DCore

/**
 * 3D snapping class.
 * 
 * 4 snapping types are available and can be combined. The snapper can be used as a screen to map coordinate converter when handling mouse movements whatever the snapping type. 
 * 
 * Snapped entities and points can be highlighted. Highlighted entities are attached a dedicated root entity. This root entity is attached to the root rubberband entity and depend on the rubberband render pass.
 */
class Qgs3DSnappingManager
{
  public:
    /**
     * 3D snapping type
     */
    enum SnappingType3D
    {
      Off = 1 << 0,        //!< Snap anything
      Vertex = 1 << 1,     //!< Snap any face vertex
      AlongEdge = 1 << 2,  //!< Snap nearest point on an edge
      MiddleEdge = 1 << 3, //!< Snap the middle point on an edge
      CenterFace = 1 << 4, //!< Snap the face center
    };

    /**
     * Default constructor 
     * \param parentWidget 3d canvas widget
     * \param tolerance snapping tolerance in map unit
     */
    Qgs3DSnappingManager( Qgs3DMapCanvasWidget *parentWidget, float tolerance );

    /**
     * Default destructor
     */
    virtual ~Qgs3DSnappingManager();

    /**
     * Makes the snapper ready
     */
    void restart();

    /**
     * Ends the snapper. Remove the root entity and all highlighted entities if any.
     */
    void finish();

    /**
     * Update snapping type
     * \param type a binary combinaison of snapping types
     */
    void setSnappingType( SnappingType3D type );

    /**
     * \return current snapping type
     */
    SnappingType3D snappingType() const { return mType; }

    /**
     * Update tolerance
     * \param tolerance new tolerance in map unit
     */
    void setTolerance( double tolerance );

    /**
     * \return current tolerance
     */
    double tolerance() const { return mTolerance; }

    /**
     * Try to snap an object in the 3D canvas according to the snapping type.
     * 
     * We use ray casting from the \a screenPos to find the nearest triangle in the 3D scene matching the snapping types.
     * If the snapping type is Off we return the best hit found in the 3D map. 
     * 
     * \param screenPos screen position to start the search from
     * \param ok if not null, will be set to false when nothing is found, else true
     * \param highlightEntity if true, will highlight the underlying entity
     * \param highlightSnappedPoint if true, will highlight the snapped point
     * \return the snapped point in map coordinates or invalid point if nothing found
     */
    QgsPoint screenToMap( const QPoint &screenPos, bool *ok = nullptr, bool highlightEntity = true, bool highlightSnappedPoint = true );

  private:
    /// remove all created entities
    void clearAllHighlightedEntities();

    /**
     *
     * \param screenPos screen position to start the search from
     * \param success will be set to false when ray casting failed, else true
     * \param snapFound will be set to Off when nothing is found, else the best snapping type 
     * \param layerId will contain the layer id of the found entity, else empty
     * \param nearestFid will contain the feature id of the found entity, else empty
     * \return the snapped point in WORLD coordinates or invalid point if nothing found
     */
    QVector3D screenToWorld( const QPoint &screenPos, bool *success, SnappingType3D *snapFound, QString *layerId, QgsFeatureId *nearestFid ) const;

    void updateHighlightedEntities( QgsMapLayer *layer, const QgsFeature &feature, const QVector3D &highlightedPoint, SnappingType3D snapFound, bool highlightEntity, bool highlightSnappedPoint );

    void updateHighlightedPoint( const QVector3D &highlightedPoint, SnappingType3D snapFound );

  private:
    SnappingType3D mType = SnappingType3D::Off;
    Qgs3DMapCanvasWidget *mParentWidget;
    Qgs3DMapCanvas *mCanvas;
    double mTolerance = 0.0;

    std::unique_ptr<Qt3DCore::QEntity> mHighlightedRootEntity = nullptr;
    std::unique_ptr<QgsRubberBand3D> mHighlightedPointBB = nullptr;
    QgsFeatureId mHighlightedFeatureId = -1;
    QVector3D mPreviousHighlightedPoint;
    QRecursiveMutex mHighlightedMutex;
};

#endif // QGS3DSNAPPINGMANAGER_H
