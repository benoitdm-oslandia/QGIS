/***************************************************************************
                        qgssfcgal.h
  -------------------------------------------------------------------
Date                 : 22 Sept 2014
Copyright            : (C) 2014 by Marco Hugentobler
email                : marco.hugentobler at sourcepole dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSSFCGALENGINE_H
#define QGSSFCGALENGINE_H

#define SIP_NO_FILE

#include <SFCGAL/capi/sfcgal_c.h>
#include "qgis_core.h"
#include "qgsgeometryengine.h"
#include "qgsgeometry.h"

class QgsLineString;
class QgsPolygon;
class QgsGeometry;
class QgsGeometryCollection;

/**
 * Contains sfcgal related utilities and functions.
 * \note not available in Python bindings.
 */
namespace sfcgal
{

  /**
   * Destroys the SFCGAL geometry \a geom, using the static QGIS
   * sfcgal context.
   */
  struct Deleter
  {

    /**
     * Destroys the SFCGAL geometry \a geom, using the static QGIS
     * sfcgal context.
     */
    void CORE_EXPORT operator()( sfcgal_geometry_t *geom ) const;

    /**
     * Destroys the SFCGAL prepared geometry \a geom, using the static QGIS
     * sfcgal context.
     */
    void CORE_EXPORT operator()( const sfcgal_prepared_geometry_t *geom ) const;

  };

  /**
   * Scoped SFCGAL pointer.
   */
  using unique_ptr = std::unique_ptr< sfcgal_geometry_t, Deleter >;
  using shared_ptr = std::shared_ptr< sfcgal_geometry_t >; // NO DELETER ==> added with function make_shared_ptr!!!!!

  sfcgal::shared_ptr make_shared_ptr( sfcgal_geometry_t *geom );

  /**
   * Scoped SFCGAL prepared geometry pointer.
   */
  using prepared_unique_ptr = std::unique_ptr< const sfcgal_prepared_geometry_t, Deleter>;

  using geometry  = sfcgal_geometry_t;

  int errorCallback( const char *, ... );
  int warningCallback( const char *, ... );

  unsigned char *hex2bytes( const char *hex, int *size );
  QString bytes2hex( const char *bytes, int size );

  class ErrorHandler
  {
    public:
      ErrorHandler();

      bool hasFailed( QString *errorMsg = nullptr );
      void clearText();
      QString getText() const;
      void addText( const QString &msg );

    private:
      QString errorMessage;
  };

  ErrorHandler *errorHandler();
}

/**
 * \ingroup core
 * \brief Does vector analysis using the sfcgal library and handles import, export, exception handling*
 * \note not available in Python bindings
 */
class CORE_EXPORT QgsSfcgalEngine: public QgsGeometryEngine
{
  public:

    /**
     * SFCGAL geometry engine constructor
     * \param geometry The geometry
     * \param precision The precision of the grid to which to snap the geometry vertices. If 0, no snapping is performed.
     * \param allowInvalidSubGeom Whether invalid sub-geometries should be skipped without error (since QGIS 3.38)
     */
    QgsSfcgalEngine( const QgsAbstractGeometry *geometry, double precision = 0 );
    QgsSfcgalEngine( sfcgal::shared_ptr geometry, double precision = 0 );

    static sfcgal::shared_ptr cloneGeometry( const sfcgal_geometry_t *geom );
    static sfcgal::shared_ptr fromWkb( const QgsConstWkbPtr &wkbPtr, QString *errorMsg = nullptr );
    static QgsConstWkbPtr toWkb( const sfcgal_geometry_t *geom, QString *errorMsg = nullptr );
    static sfcgal::shared_ptr fromWkt( const QString &wkt, QString *errorMsg = nullptr );
    static QString toWkt( const sfcgal_geometry_t *geom, int numDecimals = -1, QString *errorMsg = nullptr );

    void geometryChanged() override;
    void prepareGeometry() override;

    QgsAbstractGeometry *intersection( const QgsAbstractGeometry *geom, QString *errorMsg = nullptr, const QgsGeometryParameters &parameters = QgsGeometryParameters() ) const override;
    static sfcgal::shared_ptr intersection( const sfcgal_geometry_t *geomA, const sfcgal_geometry_t *geomB, QString *errorMsg = nullptr, const QgsGeometryParameters &parameters = QgsGeometryParameters() );

    QgsAbstractGeometry *difference( const QgsAbstractGeometry *geom, QString *errorMsg = nullptr, const QgsGeometryParameters &parameters = QgsGeometryParameters() ) const override;
    static sfcgal::shared_ptr difference( const sfcgal_geometry_t *geomA, const sfcgal_geometry_t *geomB, QString *errorMsg = nullptr, const QgsGeometryParameters &parameters = QgsGeometryParameters() );


    QgsAbstractGeometry *combine( const QgsAbstractGeometry *geom, QString *errorMsg = nullptr, const QgsGeometryParameters &parameters = QgsGeometryParameters() ) const override;
    QgsAbstractGeometry *combine( const QVector<QgsAbstractGeometry *> &geomList, QString *errorMsg, const QgsGeometryParameters &parameters = QgsGeometryParameters() ) const override;
    QgsAbstractGeometry *combine( const QVector< QgsGeometry > &, QString *errorMsg = nullptr, const QgsGeometryParameters &parameters = QgsGeometryParameters() ) const override;
    QgsAbstractGeometry *symDifference( const QgsAbstractGeometry *geom, QString *errorMsg = nullptr, const QgsGeometryParameters &parameters = QgsGeometryParameters() ) const override;
    QgsAbstractGeometry *buffer( double distance, int segments, QString *errorMsg = nullptr ) const override;
    QgsAbstractGeometry *buffer( double distance, int segments, Qgis::EndCapStyle endCapStyle, Qgis::JoinStyle joinStyle, double miterLimit, QString *errorMsg = nullptr ) const override;
    QgsAbstractGeometry *simplify( double tolerance, QString *errorMsg = nullptr ) const override;
    QgsAbstractGeometry *interpolate( double distance, QString *errorMsg = nullptr ) const override;
    QgsAbstractGeometry *envelope( QString *errorMsg = nullptr ) const override;
    QgsPoint *centroid( QString *errorMsg = nullptr ) const override;
    QgsPoint *pointOnSurface( QString *errorMsg = nullptr ) const override;
    QgsAbstractGeometry *convexHull( QString *errorMsg = nullptr ) const override;
    double distance( const QgsAbstractGeometry *geom, QString *errorMsg = nullptr ) const override;
    bool distanceWithin( const QgsAbstractGeometry *geom, double maxdistance, QString *errorMsg = nullptr ) const override;

    bool intersects( const QgsAbstractGeometry *geom, QString *errorMsg = nullptr ) const override;
    static bool intersects( const sfcgal_geometry_t *geomA, const sfcgal_geometry_t *geomB, QString *errorMsg = nullptr );

    bool touches( const QgsAbstractGeometry *geom, QString *errorMsg = nullptr ) const override;
    bool crosses( const QgsAbstractGeometry *geom, QString *errorMsg = nullptr ) const override;
    bool within( const QgsAbstractGeometry *geom, QString *errorMsg = nullptr ) const override;
    bool overlaps( const QgsAbstractGeometry *geom, QString *errorMsg = nullptr ) const override;
    bool contains( const QgsAbstractGeometry *geom, QString *errorMsg = nullptr ) const override;
    bool disjoint( const QgsAbstractGeometry *geom, QString *errorMsg = nullptr ) const override;
    QString relate( const QgsAbstractGeometry *geom, QString *errorMsg = nullptr ) const override;
    bool relatePattern( const QgsAbstractGeometry *geom, const QString &pattern, QString *errorMsg = nullptr ) const override;
    double area( QString *errorMsg = nullptr ) const override;
    double length( QString *errorMsg = nullptr ) const override;
    bool isValid( QString *errorMsg = nullptr, bool allowSelfTouchingHoles = false, QgsGeometry *errorLoc = nullptr ) const override;
    bool isEqual( const QgsAbstractGeometry *geom, QString *errorMsg = nullptr ) const override;
    bool isEmpty( QString *errorMsg = nullptr ) const override;
    bool isSimple( QString *errorMsg = nullptr ) const override;

    EngineOperationResult splitGeometry( const QgsLineString &splitLine,
                                         QVector<QgsGeometry> &newGeometries,
                                         bool topological,
                                         QgsPointSequence &topologyTestPoints,
                                         QString *errorMsg = nullptr, bool skipIntersectionCheck = false ) const override;

    QgsAbstractGeometry *offsetCurve( double distance, int segments, Qgis::JoinStyle joinStyle, double miterLimit, QString *errorMsg = nullptr ) const override;

    QgsAbstractGeometry *triangulate( QString *errorMsg = nullptr ) const;

    /**
     * Create a geometry from a sfcgal::geometry
     * \param sfcgal sfcgal::geometry. Ownership is NOT transferred.
     */
    static std::unique_ptr< QgsAbstractGeometry > toAbsGeometry( const sfcgal::geometry *sfcgal );
    static std::unique_ptr< QgsPolygon > toPolygon( const sfcgal::geometry *sfcgal );
    static std::unique_ptr<QgsAbstractGeometry> toSfcgalGeometry( sfcgal::shared_ptr &geom );

    /**
     * Returns a sfcgal geometry - caller takes ownership of the object (should be deleted with SFCGALGeom_destroy_r)
     * \param geometry geometry to convert to SFCGAL representation
     * \param errorMsg pointer to QString to receive the error message if any
     */
    static sfcgal::shared_ptr fromAbsGeometry( const QgsAbstractGeometry *geometry, QString *errorMsg = nullptr );

    static Qgis::WkbType wkbType( const sfcgal_geometry_t *geom );

    const QgsAbstractGeometry *qgsGeometry() const { return mGeometry; }
    const sfcgal::shared_ptr sfcgalGeometry() const { return mSfcgalGeom; }

  private:
    int errorCallback( const char *, ... );
    int warningCallback( const char *, ... );


    //mutable QString errorMessage;
    mutable sfcgal::shared_ptr mSfcgalGeom;
    sfcgal::prepared_unique_ptr mSfcgalPrepared;
    double mPrecision = 0.0;

};

/// @cond PRIVATE


class SFCGALException : public std::runtime_error
{
  public:
    explicit SFCGALException( const QString &message )
      : std::runtime_error( message.toUtf8().constData() )
    {
    }
};

/// @endcond

#endif // QGSSFCGALENGINE_H
