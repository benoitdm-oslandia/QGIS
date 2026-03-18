#ifndef QGSDEMHEIGHTMAPCACHE_P_H
#define QGSDEMHEIGHTMAPCACHE_P_H

#include "qgis_3d.h"

#include <QMap>
#include <QMutex>
#include <QObject>

#define SIP_NO_FILE

class QgsChunkNode;
struct QgsChunkNodeId;
class QgsRectangle;
class QgsDemHeightMapGenerator;
class Qgs3DRenderContext;


class _3D_EXPORT QgsDemHeightMapCache : public QObject
{
    Q_OBJECT
  public:
    QgsDemHeightMapCache( QgsDemHeightMapGenerator *generator, int resolution, int emitLevel, QgsChunkNode *rootNode );

    //! Returns height map cache size
    size_t size() const;

    /**
     * Checks if the tile is in the cache
     * \param tileText text version of the tile
     * \return true if the tile is in the cache
     */
    bool containsTile( const QString &tileText );

    void cleanup( const QgsChunkNode *currentNode ) const;
    void heightAndQualityAt( double x, double y, float &height, int &quality ) const;

    void setTerrainRootNode( QgsChunkNode *rootNode );

    void heightMinMax( float &zMin, float &zMax ) const;

  signals:
    //! emitted when an hi-res DEM tile has been received
    void maxResTileReceived( const QgsChunkNodeId &tileId, const QgsRectangle &extent );

  public slots:
    void onHeightMapReceived( int jobId, const QgsChunkNode *node, const QgsRectangle &extent, const QByteArray &heightMap );

  private:
    //! map used as height map data cache
    mutable QMap<QString, QByteArray> mLoaderMap;
    //! protect cache and mRootNode for concurrent updates
    mutable QRecursiveMutex mRootNodeMutex;

    QgsDemHeightMapGenerator *mHeightMapGenerator = nullptr;

    //! how many vertices to place on one side of the tile
    int mResolution = 16;
    //! min node level to start signal emittion
    int mEmitLevel;
    //! root node used to search best height map data according to x/y
    QgsChunkNode *mRootNode = nullptr;

    float mZMin;
    float mZMax;
};

class QgsTerrainGeneratorWithCache
{
  public:
    QgsTerrainGeneratorWithCache() = default;
    virtual ~QgsTerrainGeneratorWithCache() = default;

    /**
     * Returns height quality at point, the higher the better (max is the mMaxLevel). -1 = no data, 0 = coarse data.
     * \param x coordinate in map's CRS
     * \param y coordinate in map's CRS
     * \param context 3d render context
     * \return height quality at (x,y) in map's CRS
     */
    virtual int qualityAt( double x, double y, const Qgs3DRenderContext &context ) const = 0;

    virtual QgsDemHeightMapCache *heightMapCache() const = 0;
};

#endif // QGSDEMHEIGHTMAPCACHE_P_H
