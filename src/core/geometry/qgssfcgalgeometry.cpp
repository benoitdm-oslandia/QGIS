/***************************************************************************
                         qgssfcgalGeometry.cpp
                         ----------------
    begin                : September 2014
    copyright            : (C) 2014 by Marco Hugentobler
    email                : marco at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgssfcgalgeometry.h"
#include "qgswkbptr.h"


std::unique_ptr<QgsAbstractGeometry> QgsSfcgalEngine::toSfcgalGeometry( sfcgal::shared_ptr &sfcgalGeom )
{
  std::unique_ptr<QgsAbstractGeometry> qgsGeom = QgsSfcgalEngine::toAbsGeometry( sfcgalGeom.get() );
  return std::make_unique<QgsSfcgalGeometry>( qgsGeom, sfcgalGeom );
}


QgsSfcgalGeometry::QgsSfcgalGeometry()
  : QgsAbstractGeometry()
{
  mWkbType = Qgis::WkbType::NoGeometry;
}

QgsSfcgalGeometry::QgsSfcgalGeometry( std::unique_ptr<QgsAbstractGeometry> &qgsGgeom, sfcgal::shared_ptr sfcgalGeom )
  : QgsAbstractGeometry( )
{
  if ( qgsGgeom && qgsGgeom.get() )
  {
    mQgsGeom = std::move( qgsGgeom );
    mWkbType = mQgsGeom->wkbType();
  }
  else
  {
    mWkbType = QgsSfcgalEngine::wkbType( sfcgalGeom.get() );
  }
  mSfcgalGeom = sfcgalGeom;
}

QgsSfcgalGeometry::QgsSfcgalGeometry( const QgsSfcgalGeometry &geom )
  : QgsAbstractGeometry()
{
  mSfcgalGeom = QgsSfcgalEngine::cloneGeometry( geom.mSfcgalGeom.get() );
  mQgsGeom = QgsSfcgalEngine::toAbsGeometry( mSfcgalGeom.get() );

  mWkbType = geom.mWkbType;
}

QString QgsSfcgalGeometry::geometryType() const
{
  return mQgsGeom->geometryType();
}

QgsAbstractGeometry *QgsSfcgalGeometry::createEmptyWithSameType() const
{
  auto result = std::make_unique< QgsSfcgalGeometry >();
  result->mWkbType = mWkbType;
  return result.release();
}

QgsAbstractGeometry *QgsSfcgalGeometry::clone() const
{
  return new QgsSfcgalGeometry( *this );
}

void QgsSfcgalGeometry::clear()
{
  mQgsGeom.reset();
  mSfcgalGeom.reset();
  mWkbType = Qgis::WkbType::NoGeometry;
}

bool QgsSfcgalGeometry::fromWkb( QgsConstWkbPtr &wkbPtr )
{
  if ( !wkbPtr )
    return false;

  clear();

  mSfcgalGeom = QgsSfcgalEngine::fromWkb( wkbPtr );
  mQgsGeom = QgsSfcgalEngine::toAbsGeometry( mSfcgalGeom.get() );
  QString errorMsg;
  if ( sfcgal::errorHandler()->hasFailed( &errorMsg ) )
    QgsDebugError( QStringLiteral( "Unable to init sfcgal with qgs geom. Error: %1" ).arg( errorMsg ) );
  mWkbType = mQgsGeom->wkbType();

  return true;
}

int QgsSfcgalGeometry::wkbSize( QgsAbstractGeometry::WkbFlags flags ) const
{
  QByteArray array = asWkb( flags );
  return array.length();
}

QByteArray QgsSfcgalGeometry::asWkb( QgsAbstractGeometry::WkbFlags ) const
{
  QgsConstWkbPtr ptr = QgsSfcgalEngine::toWkb( mSfcgalGeom.get() );
  const unsigned char *wkbUnsignedPtr = ptr;
  return QByteArray( reinterpret_cast<const char *>( wkbUnsignedPtr ), ptr.remaining() );
}

QString QgsSfcgalGeometry::asWkt( int numDecim ) const
{
  QString out = QgsSfcgalEngine::toWkt( mSfcgalGeom.get(), numDecim );

  QString errorMsg;
  if ( sfcgal::errorHandler()->hasFailed( &errorMsg ) )
    QgsDebugError( QStringLiteral( "Unable to compute asWkt. Error: %1" ).arg( errorMsg ) );

  return out;
}


QgsAbstractGeometry *QgsSfcgalGeometry::boundary() const
{
  return mQgsGeom->boundary();
}

bool QgsSfcgalGeometry::operator ==( const QgsAbstractGeometry &other ) const
{
  return *mQgsGeom == other;
}

bool QgsSfcgalGeometry::operator !=( const QgsAbstractGeometry &other ) const
{
  return *mQgsGeom != other;
}

bool QgsSfcgalGeometry::fuzzyEqual( const QgsAbstractGeometry &other, double epsilon ) const
{
  return mQgsGeom->fuzzyEqual( other, epsilon );
}

bool QgsSfcgalGeometry::fuzzyDistanceEqual( const QgsAbstractGeometry &other, double epsilon ) const
{
  return mQgsGeom->fuzzyDistanceEqual( other, epsilon );
}

QgsBox3D QgsSfcgalGeometry::boundingBox3D() const
{
  return mQgsGeom->boundingBox3D();
}

int QgsSfcgalGeometry::dimension() const
{
  return mQgsGeom->dimension();
}

void QgsSfcgalGeometry::normalize()
{
  // nope
}

bool QgsSfcgalGeometry::fromWkt( const QString &wkt )
{
  clear();

  mSfcgalGeom = QgsSfcgalEngine::fromWkt( wkt );
  mQgsGeom = QgsSfcgalEngine::toAbsGeometry( mSfcgalGeom.get() );
  QString errorMsg;
  if ( sfcgal::errorHandler()->hasFailed( &errorMsg ) )
    QgsDebugError( QStringLiteral( "Unable to init sfcgal with qgs geom. Error: %1" ).arg( errorMsg ) );
  mWkbType = mQgsGeom->wkbType();

  return true;
}

QDomElement QgsSfcgalGeometry::asGml2( QDomDocument &doc, int precision, const QString &ns, AxisOrder axisOrder ) const
{
  return mQgsGeom->asGml2( doc,  precision, ns,  axisOrder );
}

QDomElement QgsSfcgalGeometry::asGml3( QDomDocument &doc, int precision, const QString &ns, AxisOrder axisOrder ) const
{
  return mQgsGeom->asGml3( doc,  precision, ns,  axisOrder );
}

QString QgsSfcgalGeometry::asKml( int precision ) const
{
  return mQgsGeom->asKml( precision );
}

void QgsSfcgalGeometry::transform( const QgsCoordinateTransform &, Qgis::TransformDirection, bool )
{
  // nope
}

void QgsSfcgalGeometry::transform( const QTransform &, double, double, double, double )
{
  // nope
}

void QgsSfcgalGeometry::draw( QPainter &p ) const
{
  mQgsGeom->draw( p );
}

QPainterPath QgsSfcgalGeometry::asQPainterPath() const
{
  return mQgsGeom->asQPainterPath();
}

int QgsSfcgalGeometry::vertexNumberFromVertexId( QgsVertexId id ) const
{
  return mQgsGeom->vertexNumberFromVertexId( id );
}

bool QgsSfcgalGeometry::nextVertex( QgsVertexId &id, QgsPoint &vertex ) const
{
  return mQgsGeom->nextVertex( id, vertex );
}

void QgsSfcgalGeometry::adjacentVertices( QgsVertexId vertex, QgsVertexId &previousVertex, QgsVertexId &nextVertex ) const
{
  mQgsGeom->adjacentVertices( vertex, previousVertex, nextVertex );
}

QgsCoordinateSequence QgsSfcgalGeometry::coordinateSequence() const
{
  return mQgsGeom->coordinateSequence();
}

QgsPoint QgsSfcgalGeometry::vertexAt( QgsVertexId id ) const
{
  return mQgsGeom->vertexAt( id );
}

double QgsSfcgalGeometry::closestSegment( const QgsPoint &pt, QgsPoint &segmentPt, QgsVertexId &vertexAfter, int *leftOf, double epsilon ) const
{
  return mQgsGeom->closestSegment( pt, segmentPt, vertexAfter, leftOf, epsilon );
}

bool QgsSfcgalGeometry::insertVertex( QgsVertexId, const QgsPoint & )
{
  // nope
  return false;
}

bool QgsSfcgalGeometry::moveVertex( QgsVertexId, const QgsPoint & )
{
  // nope
  return false;
}

bool QgsSfcgalGeometry::deleteVertex( QgsVertexId )
{
  // nope
  return false;
}

double QgsSfcgalGeometry::segmentLength( QgsVertexId startVertex ) const
{
  return mQgsGeom->segmentLength( startVertex );
}

QgsAbstractGeometry *QgsSfcgalGeometry::toCurveType() const
{
  return mQgsGeom->toCurveType();
}

QgsAbstractGeometry *QgsSfcgalGeometry::snappedToGrid( double hSpacing, double vSpacing, double dSpacing, double mSpacing, bool removeRedundantPoints ) const
{
  return mQgsGeom->snappedToGrid( hSpacing, vSpacing, dSpacing, mSpacing, removeRedundantPoints );
}

QgsAbstractGeometry *QgsSfcgalGeometry::simplifyByDistance( double tolerance ) const
{
  return mQgsGeom->simplifyByDistance( tolerance );
}

bool QgsSfcgalGeometry::removeDuplicateNodes( double, bool )
{
  // nope
  return false;
}

double QgsSfcgalGeometry::vertexAngle( QgsVertexId vertex ) const
{
  return mQgsGeom->vertexAngle( vertex );
}

int QgsSfcgalGeometry::vertexCount( int part, int ring ) const
{
  return mQgsGeom->vertexCount( part, ring );
}

int QgsSfcgalGeometry::ringCount( int ) const
{
  return mQgsGeom->partCount();
}

int QgsSfcgalGeometry::partCount() const
{
  return mQgsGeom->partCount();
}

bool QgsSfcgalGeometry::addZValue( double )
{
  // nope
  return false;
}

bool QgsSfcgalGeometry::addMValue( double )
{
  // nope
  return false;
}

bool QgsSfcgalGeometry::dropZValue()
{
  // nope
  return false;
}

bool QgsSfcgalGeometry::dropMValue()
{
  // nope
  return false;
}

void QgsSfcgalGeometry::swapXy()
{
  // nope
}

bool QgsSfcgalGeometry::isValid( QString &error, Qgis::GeometryValidityFlags flags ) const
{
  return mQgsGeom->isValid( error, flags );
}

bool QgsSfcgalGeometry::transform( QgsAbstractGeometryTransformer *, QgsFeedback * )
{
  // nope
  return false;
}

int QgsSfcgalGeometry::compareToSameClass( const QgsAbstractGeometry *other ) const
{
  return mQgsGeom->compareTo( other );
}

QgsSfcgalGeometry *QgsSfcgalGeometry::intersection( const QgsAbstractGeometry *geom, QString *errorMsg, const QgsGeometryParameters &parameters ) const
{
  auto engine = std::make_unique<QgsSfcgalEngine>( mSfcgalGeom );
  return dynamic_cast<QgsSfcgalGeometry *>( engine->intersection( geom, errorMsg, parameters ) );
}

bool QgsSfcgalGeometry::intersects( const QgsAbstractGeometry *geom, QString *errorMsg ) const
{
  auto engine = std::make_unique<QgsSfcgalEngine>( mSfcgalGeom );
  return engine->intersects( geom, errorMsg );
}

QgsSfcgalGeometry *QgsSfcgalGeometry::triangulate( QString *errorMsg ) const
{
  auto engine = std::make_unique<QgsSfcgalEngine>( mSfcgalGeom );
  return dynamic_cast<QgsSfcgalGeometry *>( engine->triangulate( errorMsg ) );
}

QgsSfcgalGeometry *QgsSfcgalGeometry::difference( const QgsAbstractGeometry *geom, QString *errorMsg, const QgsGeometryParameters &parameters ) const
{
  auto engine = std::make_unique<QgsSfcgalEngine>( mSfcgalGeom );
  return dynamic_cast<QgsSfcgalGeometry *>( engine->difference( geom, errorMsg, parameters ) );
}

