/***************************************************************************
                         qgssfcgalGeometry.h
                         -------------------
    copyright            : (C) 2024 by B. D.-M.
    email                : benoit dot de dotmezzo at oslandia dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSSGCGAL_GEOMETRY_H
#define QGSSGCGAL_GEOMETRY_H

#include "qgis_core.h"
#include "qgis_sip.h"
#include "qgsabstractgeometry.h"
#include "qgssfcgalengine.h"


/**
 * \ingroup core
 * \class QgsSfcgalGeometry
 * \brief SfcgalGeometry geometry type.
 */
class CORE_EXPORT QgsSfcgalGeometry: public QgsAbstractGeometry
{
  public:

    /**
     * Constructor for an empty sfcgalGeometry geometry.
     */
    QgsSfcgalGeometry() SIP_HOLDGIL;

    /**
     */
    QgsSfcgalGeometry( std::unique_ptr<QgsAbstractGeometry> &qgsGgeom, sfcgal::shared_ptr sfcgalGeom );

    QgsSfcgalGeometry( const QgsSfcgalGeometry &geom );

    QString geometryType() const override SIP_HOLDGIL;
    QgsAbstractGeometry *clone() const override;
    void clear() override;
    bool fromWkb( QgsConstWkbPtr &wkb ) override;
    int wkbSize( QgsAbstractGeometry::WkbFlags flags = QgsAbstractGeometry::WkbFlags() ) const override;
    QByteArray asWkb( QgsAbstractGeometry::WkbFlags flags = QgsAbstractGeometry::WkbFlags() ) const override;
    QString asWkt( int precision = -1 ) const override;

    QgsAbstractGeometry *boundary() const override SIP_FACTORY;

    QgsAbstractGeometry *createEmptyWithSameType() const override;

    bool operator ==( const QgsAbstractGeometry &other ) const override;
    bool operator !=( const QgsAbstractGeometry &other ) const override;
    bool fuzzyEqual( const QgsAbstractGeometry &other, double epsilon ) const override;
    bool fuzzyDistanceEqual( const QgsAbstractGeometry &other, double epsilon ) const override;
    QgsBox3D boundingBox3D() const override;
    int dimension() const override;
    void normalize() override;
    bool fromWkt( const QString &wkt ) override;
    QDomElement asGml2( QDomDocument &doc, int precision, const QString &ns, AxisOrder axisOrder ) const override;
    QDomElement asGml3( QDomDocument &doc, int precision, const QString &ns, AxisOrder axisOrder ) const override;
    QString asKml( int precision ) const override;
    void transform( const QgsCoordinateTransform &ct, Qgis::TransformDirection d, bool transformZ ) override;
    void transform( const QTransform &t, double zTranslate, double zScale, double mTranslate, double mScale ) override;
    void draw( QPainter &p ) const override;
    QPainterPath asQPainterPath() const override;
    int vertexNumberFromVertexId( QgsVertexId id ) const override;
    bool nextVertex( QgsVertexId &id, QgsPoint &vertex ) const override;
    void adjacentVertices( QgsVertexId vertex, QgsVertexId &previousVertex, QgsVertexId &nextVertex ) const override;
    QgsCoordinateSequence coordinateSequence() const override;
    QgsPoint vertexAt( QgsVertexId id ) const override;
    double closestSegment( const QgsPoint &pt, QgsPoint &segmentPt, QgsVertexId &vertexAfter, int *leftOf, double epsilon ) const override;
    bool insertVertex( QgsVertexId position, const QgsPoint &vertex ) override;
    bool moveVertex( QgsVertexId position, const QgsPoint &newPos ) override;
    bool deleteVertex( QgsVertexId position ) override;
    double segmentLength( QgsVertexId startVertex ) const override;
    QgsAbstractGeometry *toCurveType() const override;
    QgsAbstractGeometry *snappedToGrid( double hSpacing, double vSpacing, double dSpacing, double mSpacing, bool removeRedundantPoints = false ) const override;
    bool removeDuplicateNodes( double epsilon, bool useZValues ) override;
    double vertexAngle( QgsVertexId vertex ) const override;
    int vertexCount( int part, int ring ) const override;
    int ringCount( int part ) const override;
    int partCount() const override;
    bool addZValue( double zValue ) override;
    bool addMValue( double mValue ) override;
    bool dropZValue() override;
    bool dropMValue() override;
    void swapXy() override;
    bool isValid( QString &error, Qgis::GeometryValidityFlags flags ) const override;
    bool transform( QgsAbstractGeometryTransformer *transformer, QgsFeedback *feedback ) override;
    QgsAbstractGeometry *simplifyByDistance( double tolerance ) const override;


    QgsSfcgalGeometry *intersection( const QgsAbstractGeometry *geom, QString *errorMsg = nullptr, const QgsGeometryParameters &parameters = QgsGeometryParameters() ) const;
    bool intersects( const QgsAbstractGeometry *geom, QString *errorMsg = nullptr ) const;
    QgsSfcgalGeometry *triangulate( QString *errorMsg = nullptr ) const;
    QgsSfcgalGeometry *difference( const QgsAbstractGeometry *geom, QString *errorMsg = nullptr, const QgsGeometryParameters &parameters = QgsGeometryParameters() ) const;

    sfcgal::shared_ptr sfcgalGeometry() const {return mSfcgalGeom; }
    const QgsAbstractGeometry *delegatedGeometry()const {return mQgsGeom.get();}

  protected:
    sfcgal::shared_ptr mSfcgalGeom;
    std::unique_ptr<QgsAbstractGeometry> mQgsGeom;

  protected:
    int compareToSameClass( const QgsAbstractGeometry *other ) const override;
};


#endif // QGSSGCGAL_GEOMETRY_H
